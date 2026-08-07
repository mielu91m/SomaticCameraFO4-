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

	bool ImprovedCameraFO4::IsPlayerInFurniture() const
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !player->currentProcess || !player->currentProcess->middleHigh)
			return false;

		// True as long as the player is occupying ANY furniture object, even
		// while the camera has already left kFurniture for the terminal's
		// first-person view or is playing the stand-up exit animation.
		// currentFurniture is the furniture the actor is moving to/using,
		// occupiedFurniture is the one they are physically sitting on.
		auto& currentFurniture = player->currentProcess->middleHigh->currentFurniture;
		if (currentFurniture && currentFurniture.get())
			return true;
		auto& occupiedFurniture = player->currentProcess->middleHigh->occupiedFurniture;
		if (occupiedFurniture && occupiedFurniture.get())
			return true;
		return false;
	}

	void ImprovedCameraFO4::PopPseudoK3rdPerson()
	{
		auto playerCamera = RE::PlayerCamera::GetSingleton();
		if (!playerCamera)
			return;

		// Remove pseudo's k3rdPerson from the camera return stack.
		// The engine may have pushed additional k3rdPerson copies on top
		// (e.g. weapon-drawn camera pushes kFirstPerson, which re-pushes
		// the current k3rdPerson). A single PopState is not enough.
		// Also pop kIronSights entries: when a weapon is drawn the engine
		// may push kIronSights on top of pseudo's k3rdPerson, and we
		// need to clear those too so the Pip-Boy gets a clean state.
		// Phase 1: pop every k3rdPerson or kIronSights that is the top
		// of the return stack (engine duplicates). Phase 2: if current
		// is still k3rdPerson or kIronSights (pseudo's own push or
		// engine's weapon-drawn state), pop that too.
		int popped = 0;
		while (popped < 16 && !playerCamera->tempReturnStates.empty()) {
			auto id = playerCamera->tempReturnStates.back()->id;
			if (id != RE::CameraState::k3rdPerson && id != RE::CameraState::kIronSights)
				break;
			playerCamera->PopState();
			popped++;
		}
		while (popped < 16 &&
			(playerCamera->QCameraEquals(RE::CameraState::k3rdPerson) ||
			 playerCamera->QCameraEquals(RE::CameraState::kIronSights))) {
			playerCamera->PopState();
			popped++;
		}
		if (popped > 0)
			LOG_INFO("Pseudo-FPP: popped {}x k3rdPerson/kIronSights from return stack", popped);
	}

	void ImprovedCameraFO4::RestorePseudoK3rdPerson()
	{
		auto playerCamera = RE::PlayerCamera::GetSingleton();
		if (!playerCamera)
			return;

		// The engine can drop the pseudo camera into real FPP (weapon-drawn
		// camera, ADS end) by either SetState(kFirstPerson) or
		// PushState(kFirstPerson). In the PushState case it pushed pseudo's
		// k3rdPerson onto the return stack, so a SetState back to k3rdPerson
		// would leave a stale duplicate k3rdPerson on the stack (which then
		// breaks Pip-Boy). Pop it back off instead so the stack stays
		// balanced: currentState = k3rdPerson, top of return stack = k3rdPerson.
		// Also handle kIronSights: when a weapon is drawn and the player is
		// in ADS, the engine may push kIronSights on the stack. Pop it so
		// the camera properly returns to pseudo's k3rdPerson.
		//
		// CRITICAL: pop ALL stale entries, not just one. When a weapon is
		// drawn every frame the engine pushes kFirstPerson (pushing current
		// k3rdPerson onto the return stack), and RestorePseudoK3rdPerson
		// only pops one per frame — stale entries accumulate and corrupt
		// the stack, which prevents the engine from transitioning to Pip-Boy
		// camera later (hands/weapon overlay blocks Pip-Boy UI).
		bool pushed = false;
		while (!playerCamera->tempReturnStates.empty()) {
			auto id = playerCamera->tempReturnStates.back()->id;
			if (id == RE::CameraState::k3rdPerson || id == RE::CameraState::kIronSights) {
				playerCamera->PopState();
				pushed = true;
			} else {
				break;
			}
		}
		if (pushed) {
			LOG_INFO("Pseudo-FPP: restored k3rdPerson via PopState (popped stale FPP/ADS entries)");
		} else {
			playerCamera->SetState(playerCamera->GetState(RE::CameraState::k3rdPerson).get());
			LOG_INFO("Pseudo-FPP: restored k3rdPerson via SetState");
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

		// Refresh F4MCM-saved settings periodically so MCM tweaks take effect
		// without a game restart. Throttled to every ~60 frames (1s at 60fps).
		static int mcmReloadFrame = 0;
		if (m_pluginConfig && ++mcmReloadFrame >= 60)
		{
			mcmReloadFrame = 0;
			m_pluginConfig->ReloadMCMSettings();
			m_pluginConfig->ReloadMCMKeybinds();
		}

		// Refresh menu stand-down counter once per frame.
		// While > 0 every pseudo hook behaves as if a blocking menu is open,
		// so the engine can freely run the menu camera (Pip-Boy / terminal
		// transition to kFirstPerson, etc.).
		if (m_PseudoMenuBlockFrames > 0)
			m_PseudoMenuBlockFrames--;

		// Fail-safe furniture/terminal detection (ordering- and hook-failure
		// agnostic). The StartFurnitureMode/SetState hooks below should already
		// disable pseudo on furniture entry, but if they fail to install (game
		// version mismatch → unresolved REL::ID) or PlayerCamera::Update pins the
		// camera on the same frame the player activates a terminal, pseudo can
		// still be active here. Force it off the instant we detect the player is
		// occupying any furniture object (currentFurniture/occupiedFurniture),
		// which is set the moment the sit animation begins — this is the robust
		// signal that pseudo must step aside so the engine can run the terminal
		// entry/exit animation and the TerminalMenu can open. Re-enable happens
		// in the furniture-exit block below once the player fully leaves.
		if (m_PseudoFPPActive && !IsBlockingMenuOpen() && IsPlayerInFurniture()) {
			LOG_INFO("Pseudo-FPP: furniture/terminal detected (IsPlayerInFurniture) — disabling pseudo (fail-safe)");
			m_PseudoPendingFurnitureExit = true;
			m_PseudoMenuBlockFrames = 120;
			// Pop pseudo's pushed k3rdPerson so the engine owns the camera state
			// stack while entering furniture (matches StartFurnitureMode hook).
			if (m_PseudoPushedK3rdPerson) {
				PopPseudoK3rdPerson();
				m_PseudoPushedK3rdPerson = false;
			}
			SetPseudoFPPActive(false);
		}

		// Re-enable pseudo after a furniture / terminal session ends. pseudo
		// was disabled by the SetState/StartFurnitureMode hooks when the player
		// entered furniture (terminals, chairs). It must stay disabled through
		// the WHOLE session: while a TerminalMenu is open the camera leaves
		// kFurniture for the terminal's first-person view, so gating on
		// !m_TerminalMenuIsOpen prevents pseudo from re-engaging mid-terminal
		// (which captures the terminal window/animations and soft-locks the
		// player — can't use or exit the terminal). The m_PseudoMenuBlockFrames
		// gate additionally keeps pseudo off during the stand-down window set
		// on entry (StartFurnitureMode) and on TerminalMenu close, so it cannot
		// re-engage while the engine is still running the terminal entry/exit
		// camera transition. Once the terminal has fully closed AND the camera
		// is out of kFurniture/kPCTransition AND the stand-down has elapsed,
		// re-enable.
		if (!m_PseudoFPPActive && m_PseudoPendingFurnitureExit && !m_TerminalMenuIsOpen &&
			m_PseudoMenuBlockFrames <= 0) {
			const bool stillInFurniture =
				playerCamera->QCameraEquals(RE::CameraState::kFurniture) ||
				playerCamera->QCameraEquals(RE::CameraState::kPCTransition);
			// Also keep pseudo off while the player still occupies the
			// furniture object itself (IsPlayerInFurniture). This covers the
			// races a pure camera-state check misses: on ENTER the camera may
			// briefly be kFirstPerson before TerminalMenu registers while the
			// furniture handle is already alive, and on EXIT the camera
			// restores to gameplay BEFORE the stand-up animation finishes.
			// When the player is fully out, count down a short grace window
			// instead of re-engaging on the very next frame.
			const bool stillInFurnitureObj = IsPlayerInFurniture();
			if (stillInFurniture || stillInFurnitureObj) {
				// Keep the exit grace armed so the engine's exit animation
				// gets room to finish once the player actually leaves.
				m_PseudoFurnitureExitGraceFrames = 90;
			}
			else {
				// Fully out of furniture — count down the grace, then re-enable.
				if (m_PseudoFurnitureExitGraceFrames > 0)
					m_PseudoFurnitureExitGraceFrames--;
				if (m_PseudoFurnitureExitGraceFrames <= 0) {
					LOG_INFO("Pseudo-FPP: re-enabling after furniture exit");
					m_PseudoPendingFurnitureExit = false;
					m_PseudoFPPActive = true;
					m_PseudoPendingK3rdPersonPush = true;
					m_PseudoPushedK3rdPerson = false;
					m_PseudoReenableFrameCount = 30;
					// Don't shorten a longer stand-down (e.g. the 60-frame grace
					// set when TerminalMenu closed) below the default 30 frames.
					if (m_PseudoMenuBlockFrames < 30)
						m_PseudoMenuBlockFrames = 30;
				}
			}
		}

		// toggle key (default VK_F4 = 115), read from config; the F4MCM hotkey
		// control (Keybinds.json) takes precedence over the iToggleKey setting.
		const int toggleKey = m_pluginConfig->ToggleKey();
		bool currentKeyState = (GetAsyncKeyState(toggleKey) & 0x8000) != 0;
		static bool lastKeyState = false;
		if (currentKeyState && !lastKeyState && !IsBlockingMenuOpen() && !m_TerminalMenuIsOpen) {
			m_PseudoFPPActive = !m_PseudoFPPActive;
			LOG_INFO("Pseudo-FPP {} (key {} toggle)", m_PseudoFPPActive ? "enabled" : "disabled", toggleKey);

			if (m_PseudoFPPActive) {
				const bool isFurnitureToggle = playerCamera->QCameraEquals(RE::CameraState::kFurniture);
				const bool isTransitionToggle = playerCamera->QCameraEquals(RE::CameraState::kPCTransition);
				const bool isThirdPersonNow = playerCamera->QCameraEquals(RE::CameraState::k3rdPerson);
				if (!isFurnitureToggle && !isTransitionToggle && !isThirdPersonNow) {
					playerCamera->PushState(RE::CameraState::k3rdPerson);
					m_PseudoPushedK3rdPerson = true;
					LOG_INFO("Pseudo-FPP: forced third-person camera state");
				}
				else {
					m_PseudoPushedK3rdPerson = false;
					LOG_INFO("Pseudo-FPP: enabled in {} camera state", isFurnitureToggle ? "furniture" : isTransitionToggle ? "transition" : "third-person");
				}
			}
			else {
				if (m_PseudoPushedK3rdPerson) {
					// Pop EVERY k3rdPerson pseudo pushed. A weapon drawn while
					// pseudo is active makes the engine push its own kFirstPerson
					// on top of pseudo's k3rdPerson, duplicating k3rdPerson on
					// the return stack, so a single PopState would leave the
					// camera stuck in third person after toggling off.
					PopPseudoK3rdPerson();
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
	// Block pushing if terminal menu is open to avoid interfering with
	// terminal camera control.
	if (m_PseudoFPPActive && m_PseudoPendingK3rdPersonPush && m_PseudoMenuBlockFrames <= 0 &&
		!IsBlockingMenuOpen() && !m_TerminalMenuIsOpen && !Patch::Hooks::IsVATSActive()) {
		const bool isFurniturePending = playerCamera->QCameraEquals(RE::CameraState::kFurniture);
		const bool isTransitionPending = playerCamera->QCameraEquals(RE::CameraState::kPCTransition);
		const bool isKillCamActive =
			playerCamera->QCameraEquals(RE::CameraState::kVATS) ||
			playerCamera->QCameraEquals(RE::CameraState::kAnimated);
		const bool isStableGameplayState =
			playerCamera->QCameraEquals(RE::CameraState::k3rdPerson) ||
			playerCamera->QCameraEquals(RE::CameraState::kFirstPerson) ||
			playerCamera->QCameraEquals(RE::CameraState::kIronSights);

		if (!isFurniturePending && !isTransitionPending && !isKillCamActive && isStableGameplayState) {
			if (m_PseudoReenableFrameCount > 0)
				m_PseudoReenableFrameCount--;
			if (m_PseudoReenableFrameCount <= 0) {
				// Idempotent push. Only push k3rdPerson if it is not already on
				// the camera's return stack - after a kill cam the engine may
				// have already popped pseudo's k3rdPerson and restored the base
				// state, so a blind PopState would pop the BASE state and
				// corrupt the stack (player freezes). Never PopState here.
				const bool isThirdPersonNow = playerCamera->QCameraEquals(RE::CameraState::k3rdPerson);
				bool k3rdOnStack = false;
				for (const auto& statePtr : playerCamera->tempReturnStates) {
					if (statePtr && statePtr->id == RE::CameraState::k3rdPerson) {
						k3rdOnStack = true;
						break;
					}
				}
				if (!isThirdPersonNow && !k3rdOnStack) {
					playerCamera->PushState(RE::CameraState::k3rdPerson);
					m_PseudoPushedK3rdPerson = true;
					LOG_INFO("Pseudo-FPP: pending push k3rdPerson (post-menu recovery)");
				} else {
					// Already in k3rdPerson (player's own base state) or k3rdPerson
					// already on the return stack - pseudo did NOT push it, so the
					// menu handler must not pop it later.
					m_PseudoPushedK3rdPerson = false;
					LOG_INFO("Pseudo-FPP: pending push skipped (isTPP={}, k3rdOnStack={})", isThirdPersonNow, k3rdOnStack);
				}
				m_PseudoPendingK3rdPersonPush = false;
			}
		} else {
			// still transitioning (VATS/kill cam) - keep waiting, reset the
			// delay so we wait a full delay after the transition settles
			m_PseudoReenableFrameCount = 30;
		}
	}

		// --- ADS (Starfield-style) ---
		static bool headHidden = false;
		const bool isFurnitureADS = playerCamera->QCameraEquals(RE::CameraState::kFurniture);
		const bool isTransitionADS = playerCamera->QCameraEquals(RE::CameraState::kPCTransition);
		const bool isFurnitureNow = isFurnitureADS;
		const bool isTransitionNow = isTransitionADS;
		if (m_PseudoFPPActive && !isFurnitureADS && !isTransitionADS &&
			m_PseudoMenuBlockFrames <= 0 && !(playerCamera->pipboyMode) &&
			!IsBlockingMenuOpen() && !m_TerminalMenuIsOpen && !Patch::Hooks::IsVATSActive()) {
			auto* player = RE::PlayerCharacter::GetSingleton();

			// --- Hide the player's head while the rig pins the camera to it ---
			RE::NiAVObject* headNode = nullptr;
			if (player && player->currentProcess && player->currentProcess->middleHigh)
				headNode = player->currentProcess->middleHigh->headNode;
			if (headNode)
				headNode->SetAppCulled(true);
			headHidden = true;

			{
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
				// The pseudo camera may switch to real FPP ONLY while the aim
				// button/trigger is physically held. Anything else (weapon
				// draw, POV toggle, scripts) must keep the pseudo camera.
				const bool isAimingNow = (isADSKeyDown || isGamepadAimingNow) && (isIronSightsNow || isTPP || isFPP);

				if (isAimingNow) {
					// Switch from the pseudo camera to real FPP for aiming.
					// Once the engine has taken over into kIronSights (the real
					// ADS zoom) leave it alone - yanking it back to kFirstPerson
					// every frame would make the iron-sights view flicker.
					if (!isFPP && !isIronSightsNow) {
						playerCamera->SetState(playerCamera->GetState(RE::CameraState::kFirstPerson).get());
						LOG_INFO("Pseudo-FPP: ADS -> FPP");
					}
				}
				else {
					// Not aiming: the pseudo camera is the only allowed camera.
					// If we ended up in real FPP / iron sights (ADS end, or the
					// engine / POV toggle dropped us there) return to the pseudo
					// k3rdPerson. The engine's own weapon-drawn kFirstPerson
					// request is already blocked in ThirdPersonState::Update, so
					// this only fires on genuine state leaks, not every frame.
					if (isFPP || isIronSightsNow) {
						RestorePseudoK3rdPerson();
						LOG_INFO("Pseudo-FPP: restored pseudo camera (isFPP={}, wasIronSights={})", isFPP ? 1 : 0, isIronSightsNow ? 1 : 0);
					}
				}
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

