/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include <Xinput.h>

	#include "fallout4/ImprovedCameraFO4.h"
	#include "fallout4/Hooks.h"

#include "cameras/Bleedout.h"
#include "cameras/Dialogue.h"
#include "cameras/FirstPerson.h"
#include "cameras/Furniture.h"
#include "cameras/Mount.h"
#include "cameras/ThirdPerson.h"
#include "cameras/Transition.h"

#include "fallout4/EventsFallout4.h"
#include "fallout4/Helper.h"
#include "menu/UIMenu.h"
#include "plugin.h"
#include "utils/ICMath.h"
#include "utils/Log.h"

namespace ImprovedCamera {

	// Left trigger analog aiming on gamepad (controller 0). GetAsyncKeyState
	// cannot see this - XInput has to be polled separately.
	static bool IsGamepadAiming(int threshold)
	{
		XINPUT_STATE state{};
		if (XInputGetState(0, &state) != ERROR_SUCCESS)
			return false;
		return state.Gamepad.bLeftTrigger > static_cast<BYTE>(threshold);
	}

	// Menus that take over the camera (Pip-Boy, terminals, dialogue, crafting,
	// etc.). While one is open the engine needs full camera control, so the
	// pseudo rig (and its "force FPP back to TPP" safety net) must stand down
	// or it fights the menu camera - e.g. the Pip-Boy view gets ripped back to
	// third person and "disappears".
	static bool IsBlockingMenuOpen()
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui)
			return false;
		static const RE::BSFixedString names[] = {
			"PipboyMenu", "TerminalMenu", "DialogueMenu", "ContainerMenu",
			"BarterMenu", "CraftingMenu", "WorkshopMenu", "FavoritesMenu",
			"MessageBoxMenu", "SleepWaitMenu", "BookMenu", "LockpickingMenu",
			"VATSMenu", "Console", "LoadingMenu", "MainMenu"
		};
		for (const auto& n : names) {
			if (ui->GetMenuOpen(n))
				return true;
		}
		return false;
	}

	ImprovedCameraFO4::ImprovedCameraFO4()
	{
		m_pluginConfig = DLLMain::Plugin::Get()->Config();
		SetupCameraData();
	}

	void ImprovedCameraFO4::SetPseudoFPPActive(bool a_active)
	{
		if (m_PseudoFPPActive == a_active)
			return;

		m_PseudoFPPActive = a_active;

		if (!a_active) {
			// Invalidate cached camera node pointers so the scene-graph hooks
			// don't dereference stale NiCamera/NiAVObject pointers left behind
			// by VATS/menu state transitions.
			Patch::Hooks::InvalidatePseudoCameraCache();
			m_PseudoPushedK3rdPerson = false;
			m_PseudoPendingK3rdPersonPush = false;
		}
	}

	void ImprovedCameraFO4::SetupCameraData()
	{
		m_Camera.Register(new CameraBleedout());
		m_Camera.Register(new CameraFurniture());
		m_Camera.Register(new CameraTransition());
		m_Camera.Register(new CameraDialogue());
		m_Camera.Register(new CameraThirdPerson());
		m_Camera.Register(new CameraMount());
		m_Camera.Register(new CameraFirstPerson());
	}

	void ImprovedCameraFO4::UpdateCamera(std::uint8_t currentID, std::uint8_t previousID, float deltaTime)
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		auto playerCamera = RE::PlayerCamera::GetSingleton();
		if (!player || !playerCamera)
			return;

		m_DeltaTime = deltaTime;
		m_PreviousCameraID = previousID;
		m_CurrentCameraID = currentID;

		m_IsFakeCamera = false;
		m_IsFirstPerson = false;

		auto currentState = playerCamera->GetCameraCurrentState();
		if (!currentState)
			return;

		std::uint8_t cameraStateID = static_cast<std::uint8_t>(currentState->id.get());

		m_ICamera = nullptr;

		for (auto it = m_Camera.rbegin(); it != m_Camera.rend(); ++it)
		{
			auto camera = *it;
			if (!camera)
				continue;

			camera->OnUpdate(cameraStateID, 0);

			if (camera->HasControl())
			{
				m_ICamera = camera;
				m_CameraEventID = camera->GetEventID();
				break;
			}
		}

		if (!m_ICamera)
			return;

		bool isFirstPersonCamera = cameraStateID == RE::CameraStates::kFirstPerson;
		bool isIronSights = cameraStateID == RE::CameraStates::kIronSights;
		bool isThirdPerson = cameraStateID == RE::CameraStates::k3rdPerson;
		bool isMount = cameraStateID == RE::CameraStates::kMount;
		bool isFurniture = cameraStateID == RE::CameraStates::kFurniture;
		bool isBleedout = cameraStateID == RE::CameraStates::kBleedout;
		bool isTransition = cameraStateID == RE::CameraStates::kPCTransition;
		bool isVATS = cameraStateID == RE::CameraStates::kVATS;
		bool isDialogue = cameraStateID == RE::CameraStates::kDialogue;

		m_IsFirstPerson = isFirstPersonCamera || isIronSights;

		auto thirdPersonNode = player->Get3D(false);
		if (thirdPersonNode)
		{
			bool showBody = m_IsFirstPerson && m_pluginConfig->General().bEnableBody;
			if (isMount || isFurniture || isBleedout || isVATS || isDialogue || isTransition)
				showBody = true;

			thirdPersonNode->local.scale = showBody ? 1.0f : 0.001f;
		}

		if (isFirstPersonCamera && m_ICamera && m_pluginConfig->General().bEnableBody)
		{
			m_IsFakeCamera = true;
			playerCamera->zoomInput = 0.0f;
		}

		if (isIronSights)
		{
			m_IsFakeCamera = true;
			m_IsFirstPerson = true;
		}

		UpdateFOV(playerCamera);

		if (m_IsFirstPerson || m_IsFakeCamera)
			TranslateCamera();
	}

	void ImprovedCameraFO4::UpdateFirstPerson()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return;

		auto firstPersonNode = player->Get3D(true);
		auto thirdPersonNode = player->Get3D(false);

		if (!firstPersonNode || !thirdPersonNode)
			return;

		constexpr float defaultPlayerHeight = 128.0f;
		float playerScale = 1.0f;
		if (m_pluginConfig->General().bAdjustPlayerScale && playerScale > 0.0f)
			firstPersonNode->local.scale = playerScale;

		bool useThirdPersonArms = m_pluginConfig->General().bEnableThirdPersonArms;

		if (firstPersonNode->IsNode())
		{
			auto firstPersonNiNode = static_cast<RE::NiNode*>(firstPersonNode);
			auto armsNode = Helper::FindNode(firstPersonNiNode, "Arms");
			if (armsNode)
				armsNode->local.scale = useThirdPersonArms ? 0.001f : 1.0f;
		}

		UpdateSkeleton(m_IsFirstPerson);
		DisplayShadows(m_IsFirstPerson && m_pluginConfig->General().bEnableShadows);

		if (m_IsFirstPerson || m_IsFakeCamera)
		{
			HeadRotation();
			TranslateFirstPersonModel();
			TranslateThirdPersonModel();
		}
	}

	void ImprovedCameraFO4::UpdateHeadTracking()
	{
		if (!m_IsFirstPerson || Helper::CannotMoveAndLook())
			return;

		if (m_ICamera)
		{
			auto data = m_ICamera->GetData();
			if (data.EventActive && !*data.EventActive)
				return;
		}

		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return;

		auto thirdPersonNode = player->Get3D(false);
		if (!thirdPersonNode)
			return;

		auto headNode = Helper::GetHeadNode(thirdPersonNode);
		if (!headNode)
			return;

		if (!headNode->IsNode())
			return;
	}

	bool ImprovedCameraFO4::ProcessInput(const RE::InputEvent* const* a_event)
	{
		if (!a_event || !*a_event)
			return false;

		auto ui = DLLMain::Plugin::Get()->Graphics();
		if (!ui || !ui->UI())
			return false;

		auto menu = ui->UI()->GetMenu();
		if (!menu)
			return false;

		if (!menu->IsUIDisplayed())
			return false;

		for (auto event = *a_event; event; event = event->next)
		{
			if (event->eventType == RE::INPUT_EVENT_TYPE::kButton)
			{
				auto buttonEvent = event->As<RE::ButtonEvent>();
				if (!buttonEvent)
					continue;

				if (buttonEvent->QJustPressed())
				{
					if (buttonEvent->QUserEvent() == "Journal")
					{
						auto uiSingleton = RE::UI::GetSingleton();
						if (uiSingleton && uiSingleton->GetMenuOpen("PauseMenu"))
						{
							return false;
						}
					}
				}
			}
		}
		return true;
	}

	void ImprovedCameraFO4::UpdateFOV(RE::PlayerCamera* camera)
	{
		auto config = m_pluginConfig->FOV();
		if (!config.bEnableOverride)
			return;

		float fov = 0.0f;

		switch (m_CameraEventID)
		{
			case CameraEvent::kFirstPerson: fov = config.fFirstPerson; break;
			case CameraEvent::kFirstPersonCombat: fov = config.fFirstPersonCombat; break;
			case CameraEvent::kFurniture: fov = config.fFurniture; break;
			case CameraEvent::kCrafting: fov = config.fCrafting; break;
			case CameraEvent::kRagdoll: fov = config.fRagdoll; break;
			case CameraEvent::kDeath: fov = config.fDeath; break;
			case CameraEvent::kMount: fov = config.fMount; break;
			case CameraEvent::kMountCombat: fov = config.fMountCombat; break;
			case CameraEvent::kDialogue: fov = config.fDialogue; break;
			case CameraEvent::kScripted: fov = config.fScripted; break;
			case CameraEvent::kThirdPerson: fov = config.fThirdPerson; break;
			default: break;
		}

		if (fov > 0.0f)
			camera->worldFOV = fov;
	}

	void ImprovedCameraFO4::UpdateSkeleton(bool show)
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return;

		auto thirdPersonNode = player->Get3D(false);
		if (!thirdPersonNode)
			return;

		if (!thirdPersonNode->IsNode())
			return;

		auto thirdPersonNiNode = static_cast<RE::NiNode*>(thirdPersonNode);

		auto configGeneral = m_pluginConfig->General();
		auto configHide = m_pluginConfig->Hide();

		bool headVisible = configGeneral.bEnableHead;
		if (m_CameraEventID == CameraEvent::kFirstPersonCombat && configGeneral.bEnableHeadCombat)
			headVisible = true;

		auto headNode = Helper::GetHeadNode(thirdPersonNiNode);
		if (headNode)
			headNode->local.scale = headVisible ? 1.0f : 0.001f;

		auto weaponNode = Helper::FindNode(thirdPersonNiNode, "WEAPON");
		if (weaponNode && configHide.bWeapon)
			weaponNode->local.scale = 0.001f;
	}

	void ImprovedCameraFO4::DisplayShadows(bool show)
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return;

		auto thirdPersonNode = player->Get3D(false);
		if (!thirdPersonNode)
			return;

		auto& flags = thirdPersonNode->flags;
		if (show)
			flags.flags |= (1ull << 40);
		else
			flags.flags &= ~(1ull << 40);
	}

	float ImprovedCameraFO4::UpdateNearDistance(float fNear)
	{
		auto config = m_pluginConfig->NearDistance();
		if (!config.bEnableOverride)
			return fNear;

		float newNear = fNear;

		switch (m_CameraEventID)
		{
			case CameraEvent::kFirstPerson: newNear = config.fFirstPerson; break;
			case CameraEvent::kFirstPersonCombat: newNear = config.fFirstPersonCombat; break;
			case CameraEvent::kFurniture: newNear = config.fFurniture; break;
			case CameraEvent::kCrafting: newNear = config.fCrafting; break;
			case CameraEvent::kRagdoll: newNear = config.fRagdoll; break;
			case CameraEvent::kDeath: newNear = config.fDeath; break;
			case CameraEvent::kMount: newNear = config.fMount; break;
			case CameraEvent::kMountCombat: newNear = config.fMountCombat; break;
			case CameraEvent::kDialogue: newNear = config.fDialogue; break;
			case CameraEvent::kScripted: newNear = config.fScripted; break;
			case CameraEvent::kThirdPerson: newNear = config.fThirdPerson; break;
			default: break;
		}

		if (newNear <= 0.0f)
			newNear = fNear;

		return newNear;
	}

	void ImprovedCameraFO4::TranslateCamera()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		auto playerCamera = RE::PlayerCamera::GetSingleton();
		if (!player || !playerCamera)
			return;

		auto thirdPersonNode = player->Get3D(false);
		if (!thirdPersonNode)
			return;

		if (!thirdPersonNode->IsNode())
			return;

		auto thirdPersonNiNode = static_cast<RE::NiNode*>(thirdPersonNode);

		auto headNode = Helper::GetHeadNode(thirdPersonNiNode);
		if (!headNode)
			return;

		auto cameraNode = Helper::FindNode(thirdPersonNiNode, "Camera");
		if (!cameraNode)
			return;

		RE::NiUpdateData updateData{};
		headNode->Update(updateData);

		RE::NiPoint3 headPos = headNode->world.translate;
		RE::NiPoint3 cameraOffset = { 0.0f, 0.0f, 0.0f };

		ScalePoint(&cameraOffset, 1.0f);

		RE::NiPoint3 targetPos = headPos + cameraOffset;

		float lerpFactor = 0.1f;
		targetPos.x = cameraNode->world.translate.x + (targetPos.x - cameraNode->world.translate.x) * lerpFactor;
		targetPos.y = cameraNode->world.translate.y + (targetPos.y - cameraNode->world.translate.y) * lerpFactor;
		targetPos.z = cameraNode->world.translate.z + (targetPos.z - cameraNode->world.translate.z) * lerpFactor;

		cameraNode->world.translate = targetPos;
		if (cameraNode->IsNode())
			static_cast<RE::NiNode*>(cameraNode)->local.translate = targetPos;
	}

	void ImprovedCameraFO4::TranslateFirstPersonModel()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return;

		auto firstPersonNode = player->Get3D(true);
		auto thirdPersonNode = player->Get3D(false);
		if (!firstPersonNode || !thirdPersonNode)
			return;

		auto headNode = Helper::GetHeadNode(thirdPersonNode);
		if (!headNode)
			return;

		firstPersonNode->world.translate = headNode->world.translate;
	}

	void ImprovedCameraFO4::TranslateThirdPersonModel()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return;

		auto thirdPersonNode = player->Get3D(false);
		if (!thirdPersonNode)
			return;

		if (!thirdPersonNode->IsNode())
			return;

		auto thirdPersonNiNode = static_cast<RE::NiNode*>(thirdPersonNode);

		auto cameraNode = Helper::FindNode(thirdPersonNiNode, "Camera");
		if (!cameraNode)
			return;

		AdjustModelPosition(m_thirdpersonLocalTranslate, false);

		thirdPersonNode->local.translate = m_thirdpersonLocalTranslate;
		thirdPersonNode->world.translate = m_thirdpersonLocalTranslate;
	}

	void ImprovedCameraFO4::AdjustModelPosition(RE::NiPoint3& position, bool headbob)
	{
		position = { 0.0f, 0.0f, 0.0f };
	}

	bool ImprovedCameraFO4::HeadRotation()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return false;

		auto playerCamera = RE::PlayerCamera::GetSingleton();
		if (!playerCamera)
			return false;

		auto thirdPersonNode = player->Get3D(false);
		if (!thirdPersonNode)
			return false;

		if (!thirdPersonNode->IsNode())
			return false;

		auto thirdPersonNiNode = static_cast<RE::NiNode*>(thirdPersonNode);

		auto headNode = Helper::GetHeadNode(thirdPersonNiNode);
		if (!headNode)
			return false;

		if (!headNode->IsNode())
			return false;

		auto headNiNode = static_cast<RE::NiNode*>(headNode);

		if (Helper::CannotMoveAndLook())
			return false;

		if (Helper::IsSitting(player))
			return false;

		auto cameraState = playerCamera->GetCameraCurrentState();
		if (!cameraState)
			return false;

		RE::NiQuaternion headRot;
		cameraState->GetRotation(headRot);

		float x2 = headRot.x + headRot.x;
		float y2 = headRot.y + headRot.y;
		float z2 = headRot.z + headRot.z;
		float xx = headRot.x * x2;
		float xy = headRot.x * y2;
		float xz = headRot.x * z2;
		float yy = headRot.y * y2;
		float yz = headRot.y * z2;
		float zz = headRot.z * z2;
		float wx = headRot.w * x2;
		float wy = headRot.w * y2;
		float wz = headRot.w * z2;

		headNiNode->local.rotate = RE::NiMatrix3{
			1.0f - (yy + zz), xy + wz, xz - wy, 0.0f,
			xy - wz, 1.0f - (xx + zz), yz + wx, 0.0f,
			xz + wy, yz - wx, 1.0f - (xx + yy), 0.0f
		};

		return true;
	}

	void ImprovedCameraFO4::ScalePoint(RE::NiPoint3* point, float scale)
	{
		if (!point)
			return;

		switch (m_CameraEventID)
		{
			case CameraEvent::kFirstPerson:
			case CameraEvent::kFirstPersonCombat:
			{
				auto config = m_pluginConfig->Camera();
				point->x = config.fFirstPersonPosX * scale;
				point->y = config.fFirstPersonPosY * scale;
				point->z = config.fFirstPersonPosZ * scale;
				break;
			}
			case CameraEvent::kMount:
			case CameraEvent::kMountCombat:
			{
				auto config = m_pluginConfig->Camera();
				point->x = config.fMountPosX * scale;
				point->y = config.fMountPosY * scale;
				point->z = config.fMountPosZ * scale;
				break;
			}
			case CameraEvent::kScripted:
			{
				auto config = m_pluginConfig->Camera();
				point->x = config.fScriptedPosX * scale;
				point->y = config.fScriptedPosY * scale;
				point->z = config.fScriptedPosZ * scale;
				break;
			}
			case CameraEvent::kThirdPerson:
			default:
				break;
		}
	}

	void ImprovedCameraFO4::ForceFirstPerson()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return;

		LOG_DEBUG("ForceFirstPerson called");
	}

	void ImprovedCameraFO4::ForceThirdPerson()
	{
		LOG_DEBUG("ForceThirdPerson called");
	}

	void ImprovedCameraFO4::TogglePOV()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		auto playerCamera = RE::PlayerCamera::GetSingleton();
		if (!player || !playerCamera)
			return;

		LOG_DEBUG("TogglePOV called");
	}

	void ImprovedCameraFO4::ResetState(bool forced)
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return;

		m_PreviousCameraID = 255;
		m_CurrentCameraID = 255;
		m_IsFirstPerson = false;
		m_IsThirdPersonForced = false;
		m_IsFakeCamera = false;
		m_iRagdollFrame = 0;

		for (auto& camera : m_Camera)
		{
			if (camera)
				camera->OnShutdown();
		}

		LOG_DEBUG("Camera state reset");
	}

	void ImprovedCameraFO4::Ragdoll(RE::Actor* actor)
	{
		if (!actor || actor != RE::PlayerCharacter::GetSingleton())
			return;

		m_iRagdollFrame = 0;
		LOG_DEBUG("Ragdoll detected");
	}

	void ImprovedCameraFO4::ResetPlayerNodes()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return;

		LOG_DEBUG("Player nodes reset");
	}

	void ImprovedCameraFO4::RequestAPIs()
	{
		LOG_DEBUG("Requesting external APIs...");
	}

	void ImprovedCameraFO4::DetectMods()
	{
		LOG_INFO("DetectMods called, registering PerFrameUpdate...");
		F4SE::GetTaskInterface()->AddTaskPermanent([this]() { PerFrameUpdate(); });
		LOG_INFO("PerFrameUpdate registered successfully");
	}

	void ImprovedCameraFO4::PerFrameUpdate()
	{
		auto playerCamera = RE::PlayerCamera::GetSingleton();
		if (!playerCamera)
			return;

		// toggle key (default VK_F4 = 115), read from config
		const int toggleKey = m_pluginConfig->PseudoFPP().iToggleKey;
		bool currentKeyState = (GetAsyncKeyState(toggleKey) & 0x8000) != 0;
		static bool lastKeyState = false;
		if (currentKeyState && !lastKeyState) {
			m_PseudoFPPActive = !m_PseudoFPPActive;
			LOG_INFO("Pseudo-FPP {} (key {} toggle)", m_PseudoFPPActive ? "enabled" : "disabled", toggleKey);

			if (m_PseudoFPPActive) {
				const bool isFurnitureNow = playerCamera->QCameraEquals(RE::CameraState::kFurniture);
				const bool isTransitionNow = playerCamera->QCameraEquals(RE::CameraState::kPCTransition);
				const bool isThirdPersonNow = playerCamera->QCameraEquals(RE::CameraState::k3rdPerson);
				if (!isFurnitureNow && !isTransitionNow && !isThirdPersonNow) {
					playerCamera->PushState(RE::CameraState::k3rdPerson);
					m_PseudoPushedK3rdPerson = true;
					LOG_INFO("Pseudo-FPP: forced third-person camera state");
				}
				else {
					m_PseudoPushedK3rdPerson = false;
					LOG_INFO("Pseudo-FPP: enabled in {} camera state", isFurnitureNow ? "furniture" : isTransitionNow ? "transition" : "third-person");
				}
			}
			else {
				if (m_PseudoPushedK3rdPerson) {
					playerCamera->PopState();
					LOG_INFO("Pseudo-FPP: restored previous camera state");
				}
				else {
					LOG_INFO("Pseudo-FPP: disabled (was in furniture/transition, no state change)");
				}
				m_PseudoPushedK3rdPerson = false;
			}
		}
		lastKeyState = currentKeyState;

	// Handle pending k3rdPerson push from menu/VATS re-entry.
	// The MenuOpenCloseEvent handler sets this flag when re-enabling
	// pseudo after VATS closes, but defers the actual PushState to here
	// so we push at a safe point (after the engine's VATS-exit
	// transition has stabilized) rather than mid-transition.
	if (m_PseudoFPPActive && m_PseudoPendingK3rdPersonPush &&
		!IsBlockingMenuOpen()) {
		const bool isFurnitureNow = playerCamera->QCameraEquals(RE::CameraState::kFurniture);
		const bool isTransitionNow = playerCamera->QCameraEquals(RE::CameraState::kPCTransition);
		LOG_INFO("Pseudo-FPP: pending push check - isFPP={}, isTPP={}, isFurniture={}, isTransition={}",
			playerCamera->QCameraEquals(RE::CameraState::kFirstPerson),
			playerCamera->QCameraEquals(RE::CameraState::k3rdPerson),
			isFurnitureNow, isTransitionNow);
		if (!isFurnitureNow && !isTransitionNow) {
			// Pop whatever the engine left us in (e.g. kFirstPerson/kIronSights
			// post-VATS) to ensure pseudo's k3rdPerson is cleanly on top of the stack.
			playerCamera->PopState();
			playerCamera->PushState(RE::CameraState::k3rdPerson);
			m_PseudoPushedK3rdPerson = true;
			LOG_INFO("Pseudo-FPP: pending push k3rdPerson (post-menu recovery)");
		}
		m_PseudoPendingK3rdPersonPush = false;
	}

	// --- ADS (Starfield-style) ---
	static bool wasAiming = false;
	static bool headHidden = false;
	if (m_PseudoFPPActive && !IsBlockingMenuOpen()) {
		auto* player = RE::PlayerCharacter::GetSingleton();

		// --- Hide the player's head while the rig pins the camera to it ---
		RE::NiAVObject* headNode = nullptr;
		if (player && player->currentProcess && player->currentProcess->middleHigh)
			headNode = player->currentProcess->middleHigh->headNode;
		if (headNode)
			headNode->SetAppCulled(true);
		headHidden = true;

		const bool isFurnitureNow = playerCamera->QCameraEquals(RE::CameraState::kFurniture);
		const bool isTransitionNow = playerCamera->QCameraEquals(RE::CameraState::kPCTransition);

		if (!isFurnitureNow && !isTransitionNow) {
			// Poll the physical aim button/trigger instead of relying on the
			// engine's kIronSights camera state. Aiming while the game is in
			// third person (which is what the pseudo camera is under the hood)
			// does NOT change CameraState to kIronSights - the engine just
			// zooms while staying in kThirdPerson. The physical-button check
			// catches that, and real-FPP aiming (which DOES flip the state to
			// kIronSights) is caught by isIronSightsNow. Mirrors ImprovedCameraSF.
			const int adsKey = m_pluginConfig->PseudoFPP().iADSKey;
			const bool isADSKeyDown = (GetAsyncKeyState(adsKey) & 0x8000) != 0;
			const bool isGamepadAimingNow = IsGamepadAiming(m_pluginConfig->PseudoFPP().iGamepadTriggerThreshold);

			const bool isTPP = playerCamera->QCameraEquals(RE::CameraState::k3rdPerson);
			const bool isFPP = playerCamera->QCameraEquals(RE::CameraState::kFirstPerson);
			const bool isIronSightsNow = playerCamera->QCameraEquals(RE::CameraState::kIronSights);
			const bool isAimingNow = (isADSKeyDown || isGamepadAimingNow) && (isIronSightsNow || isTPP || isFPP);

			if (isAimingNow) {
				if (!isFPP) {
					playerCamera->SetState(playerCamera->GetState(RE::CameraState::kFirstPerson).get());
					LOG_INFO("Pseudo-FPP: ADS -> FPP");
				}
			}
			else if (wasAiming) {
				if (isFPP) {
					playerCamera->SetState(playerCamera->GetState(RE::CameraState::k3rdPerson).get());
					LOG_INFO("Pseudo-FPP: ADS end -> TPP (restore pseudo)");
				}
			}
			wasAiming = isAimingNow;
		}

		// --- Body follows camera yaw ---
		// When the camera turns more than ~90 deg away from the body's
		// facing, rotate the body toward it slowly (max 6 deg/frame) so the
		// character turns to match where you're looking instead of the body
		// staying frozen while the head pivots. Mirrors ImprovedCameraSF.
		if (player && !isFurnitureNow && !isTransitionNow) {
			auto* tesCam = static_cast<RE::TESCamera*>(playerCamera);
			auto* cr = tesCam->cameraRoot.get();
			if (cr) {
				constexpr float kMaxYawRad = 90.0f * 0.01745329252f;
				constexpr float kMaxStepRad = 6.0f * 0.01745329252f;
				float camYaw = std::atan2(cr->world.rotate.entry[1][0], cr->world.rotate.entry[0][0]);
				float bodyYaw = player->data.angle.z;
				float diff = camYaw - bodyYaw;
				while (diff > 3.14159265f)
					diff -= 6.28318531f;
				while (diff < -3.14159265f)
					diff += 6.28318531f;
				if (std::fabs(diff) > kMaxYawRad) {
					float clamped = std::clamp(diff, -kMaxYawRad, kMaxYawRad);
					float correction = diff - clamped;
					float step = std::clamp(correction, -kMaxStepRad, kMaxStepRad);
					if (std::fabs(step) > 0.0001f)
						player->data.angle.z = bodyYaw + step;
				}
			}
		}
	}
	else {
		wasAiming = false;
		if (headHidden) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			RE::NiAVObject* headNode = nullptr;
			if (player && player->currentProcess && player->currentProcess->middleHigh)
				headNode = player->currentProcess->middleHigh->headNode;
			if (headNode)
				headNode->SetAppCulled(false);
			headHidden = false;
		}
	}
	}

}

