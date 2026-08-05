/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "fallout4/Hooks.h"

#include "fallout4/EventsFallout4.h"
#include "fallout4/Helper.h"
#include "fallout4/ImprovedCameraFO4.h"
#include "plugin.h"
#include "systems/UI.h"
#include "utils/Log.h"

#include <MinHook.h>

namespace Address::Hook
{
	using FuncType = void(__thiscall*)(RE::PlayerCamera*, bool);

	inline REL::Relocation<FuncType> ProcessInput;
	inline REL::Relocation<FuncType> UpdateCamera;
	inline REL::Relocation<FuncType> UpdateFirstPerson;
	inline REL::Relocation<FuncType> ForceFirstPerson;
	inline REL::Relocation<FuncType> ForceThirdPerson;
	inline REL::Relocation<FuncType> TogglePOV;
	inline REL::Relocation<FuncType> NiCameraUpdate;
	inline REL::Relocation<FuncType> Ragdoll;
	inline REL::Relocation<FuncType> KillActor;
	inline REL::Relocation<FuncType> NearDistanceIndoorsFix;

	inline void (*NiCamera_UpdateWorldData_Original)(RE::NiCamera*, RE::NiUpdateData*) = nullptr;

	using PlayerCameraUpdateFunc = void(__thiscall*)(RE::PlayerCamera*);
	inline PlayerCameraUpdateFunc PlayerCameraUpdate_Original = nullptr;

	using ThirdPersonStateUpdateFunc = void(__thiscall*)(RE::ThirdPersonState*, RE::BSTSmartPointer<RE::TESCameraState>&);
	inline ThirdPersonStateUpdateFunc ThirdPersonStateUpdate_Original = nullptr;

	using StartPipboyModeFunc = void(__thiscall*)(RE::PlayerCamera*, bool);
	inline StartPipboyModeFunc StartPipboyMode_Original = nullptr;

	using StopPipboyModeFunc = void(__thiscall*)(RE::PlayerCamera*);
	inline StopPipboyModeFunc StopPipboyMode_Original = nullptr;
}

namespace Patch {

	using namespace RE;

	static ImprovedCamera::ImprovedCameraFO4* GetIC()
	{
		return DLLMain::Plugin::Get()->Fallout4()->Camera();
	}

	namespace PseudoFPP
	{
		// --- camera cache ---
		inline RE::NiCamera* g_NiCamera = nullptr;
		inline RE::NiAVObject* g_CameraRoot = nullptr;

		// --- scene graph hook originals ---
		inline void (*UpdateWorldData_Orig)(RE::NiAVObject*, RE::NiUpdateData*) = nullptr;
		inline void (*UpdateTransformAndBounds_Orig)(RE::NiAVObject*, RE::NiUpdateData*) = nullptr;
		inline void (*UpdateTransforms_Orig)(RE::NiAVObject*, RE::NiUpdateData*) = nullptr;
		inline void (*NiNodeUWD_Orig)(RE::NiAVObject*, RE::NiUpdateData*) = nullptr;
		inline void (*NiNodeUTB_Orig)(RE::NiAVObject*, RE::NiUpdateData*) = nullptr;
		inline void (*NiNodeUT_Orig)(RE::NiAVObject*, RE::NiUpdateData*) = nullptr;
		inline bool g_SceneGraphHooksInstalled = false;
		inline bool g_NiNodeHooksInstalled = false;

		// Frames to keep the scene-graph hooks standing down after the camera
		// leaves kVATS/kAnimated (kill-cam, power-armor mount/dismount, etc.)
		// so the engine's tail animation finishes before we pin again.
		inline int g_VATSCoolDownFrames = 0;

		static bool IsActive()
		{
			auto ic = GetIC();
			return ic && ic->IsPseudoFPPActive();
		}

		bool IsBlockingMenuOpen();

		// True while the engine's VATS playback / kill cam is running. The
		// scene-graph hooks must stand down during it - the cinematic moves the
		// camera around while the camera state stays on k3rdPerson, so pinning
		// the camera to the head every frame fights the cinematic and crashes.
		bool IsVATSKillCamActive();
		bool IsVATSKillCamActiveDiag();

		static RE::NiAVObject* GetHeadNode()
		{
			auto player = RE::PlayerCharacter::GetSingleton();
			if (!player || !player->currentProcess || !player->currentProcess->middleHigh)
				return nullptr;
			return player->currentProcess->middleHigh->headNode;
		}

		static RE::NiPoint3 GetHeadPos()
		{
			auto headNode = GetHeadNode();
			if (!headNode)
				return {};
			return headNode->GetWorldTranslate();
		}

		static RE::NiMatrix3 ComputeLocalRotateFromWorld(RE::NiAVObject* a_node)
		{
			if (!a_node || !a_node->parent)
				return a_node ? a_node->world.rotate : RE::NiMatrix3{};
			const RE::NiMatrix3 parentInv = a_node->parent->world.rotate.Transpose();
			return a_node->world.rotate * parentInv;
		}

		static RE::NiPoint3 TransformWorldToLocal(const RE::NiMatrix3& a_rot, const RE::NiPoint3& a_vec)
		{
			// local = transpose(world rotate) * worldOffset
			RE::NiPoint3 out{};
			out.x = a_vec.x * a_rot.entry[0][0] + a_vec.y * a_rot.entry[1][0] + a_vec.z * a_rot.entry[2][0];
			out.y = a_vec.x * a_rot.entry[0][1] + a_vec.y * a_rot.entry[1][1] + a_vec.z * a_rot.entry[2][1];
			out.z = a_vec.x * a_rot.entry[0][2] + a_vec.y * a_rot.entry[1][2] + a_vec.z * a_rot.entry[2][2];
			return out;
		}

		static RE::NiPoint3 ComputeNodeLocalFromWorld(RE::NiAVObject* a_node, const RE::NiPoint3& a_worldPos)
		{
			if (!a_node || !a_node->parent)
				return a_worldPos;
			return TransformWorldToLocal(a_node->parent->world.rotate, a_worldPos - a_node->parent->world.translate);
		}

		static RE::NiCamera* FindNiCamera(RE::TESCamera* a_tesCam)
		{
			if (!a_tesCam || !a_tesCam->cameraRoot)
				return nullptr;

			auto* root = a_tesCam->cameraRoot.get();
			if (!root)
				return nullptr;

			for (auto& child : root->children) {
				if (!child)
					continue;
				if (auto* cam = netimmerse_cast<RE::NiCamera*>(child.get()))
					return cam;
				if (auto* childNode = netimmerse_cast<RE::NiNode*>(child.get())) {
					for (auto& grandChild : childNode->children) {
						if (grandChild) {
							if (auto* cam = netimmerse_cast<RE::NiCamera*>(grandChild.get()))
								return cam;
						}
					}
				}
			}

			return nullptr;
		}

		static void ForceCameraToHead()
		{
			if (!IsActive())
				return;

			if (IsBlockingMenuOpen() || IsVATSKillCamActiveDiag())
				return;

			auto* camera = RE::PlayerCamera::GetSingleton();
			if (!camera)
				return;

			// pseudo overrides third-person-style camera states
			// (k3rdPerson, kIronSights) plus furniture and power armor
			// transition (kFurniture, kPCTransition)
			if (!camera->QCameraEquals(RE::CameraState::k3rdPerson) &&
				!camera->QCameraEquals(RE::CameraState::kIronSights) &&
				!camera->QCameraEquals(RE::CameraState::kFurniture) &&
				!camera->QCameraEquals(RE::CameraState::kPCTransition))
				return;

			auto* tesCam = static_cast<RE::TESCamera*>(camera);
			auto* cr = tesCam->cameraRoot.get();
			if (!cr)
				return;

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player)
				return;

			RE::NiPoint3 headPos = GetHeadPos();
			if (headPos == RE::NiPoint3{})
				return;

			auto* niCam = g_NiCamera ? g_NiCamera : FindNiCamera(tesCam);
			if (niCam)
				g_NiCamera = niCam;
			g_CameraRoot = cr;

			// user-adjustable offsets: vertical correction + camera push-out
			// along the current view direction (rotation is left to the engine,
			// so this follows mouse look)
			auto* config = DLLMain::Plugin::Get()->Config();
			const float heightOffset = config ? config->PseudoFPP().fHeightOffset : 0.0f;
			const float forwardOffset = config ? config->PseudoFPP().fForwardOffset : 0.0f;

			RE::NiPoint3 rawHead = headPos;

			headPos.z -= heightOffset;

			if (forwardOffset != 0.0f) {
				// planar forward derived from the camera yaw (mirrors SF's
				// GetYawAxes); a raw rotation-matrix column is the wrong axis
				// and makes the offset point sideways/behind the facing
				const float camYaw = std::atan2(cr->world.rotate.entry[1][0], cr->world.rotate.entry[0][0]);
				RE::NiPoint3 fwd{
					std::sin(camYaw),
					std::cos(camYaw),
					0.0f
				};
				headPos += fwd * forwardOffset;
			}

			// pin camera root to head position - only translation is touched so
			// mouse look / engine rotation keeps working
			cr->local.translate = ComputeNodeLocalFromWorld(cr, headPos);
			cr->previousWorld.translate = headPos;
			cr->world.translate = headPos;

		// pin the actual NiCamera to the head; local stays at origin so it
		// tracks cr exactly
		if (niCam) {
			niCam->local.translate = {};
			niCam->previousWorld.translate = headPos;
			niCam->world.translate = headPos;
		}

		static int diagCounter = 0;
			if ((diagCounter++ % 120) == 0) {
				LOG_INFO("PseudoFPP: cfg h={:.4f} f={:.4f} rawHead=({:.2f},{:.2f},{:.2f}) cam=({:.2f},{:.2f},{:.2f})",
					heightOffset, forwardOffset,
					rawHead.x, rawHead.y, rawHead.z,
					cr->world.translate.x, cr->world.translate.y, cr->world.translate.z);
			}
		}

		bool IsBlockingMenuOpen()
		{
			auto ui = RE::UI::GetSingleton();
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

		bool IsVATSKillCamActive()
		{
			// Stand down while VATS playback runs (shot playback and the death
			// kill-cam). Note: vats->cameraContext is non-null even outside a
			// VATS session, so it can NOT be used as a session signal.
			auto* vats = RE::VATS::GetSingleton();
			if (vats && vats->mode.any(RE::VATS::VATS_MODE_ENUM::kPlayback))
				return true;
			// The engine may switch the camera into kVATS before the VATS
			// singleton reports the session - cover that window too.
			auto* camera = RE::PlayerCamera::GetSingleton();
			const bool inAnimatedState =
				camera &&
				(camera->QCameraEquals(RE::CameraState::kVATS) ||
				 camera->QCameraEquals(RE::CameraState::kAnimated));
			if (inAnimatedState) {
				// Reset the cooldown on every frame inside an animated/VATS
				// state so the grace period starts from the END of the
				// sequence (power-armor mount/dismount, kill-cam, etc.).
				g_VATSCoolDownFrames = 150;  // ~2.5s at 60fps
				return true;
			}

			// Cooldown: after a kVATS/kAnimated sequence ends (VATS kill-cam,
			// power-armor station mount/dismount, furniture, cinematic) the
			// engine keeps animating the camera for a while even though it has
			// already left kVATS/kAnimated. Pinning the camera to the head
			// during that tail animation fights the cinematic and can crash,
			// so keep the hooks standing down for a short grace period.
			if (g_VATSCoolDownFrames > 0) {
				g_VATSCoolDownFrames--;
				return true;
			}
			return false;
		}

		// Diagnostic: log VATS state transitions so we can see whether the
		// kill-cam is actually caught by the guard above.
		bool IsVATSKillCamActiveDiag()
		{
			static bool lastState = false;
			const bool now = IsVATSKillCamActive();
			if (now != lastState) {
				lastState = now;
				auto* vats = RE::VATS::GetSingleton();
				auto* camera = RE::PlayerCamera::GetSingleton();
				LOG_INFO("VATS state: active={} mode={:08X} camVATS={} camAnimated={}",
					now ? 1 : 0,
					vats ? static_cast<std::uint32_t>(vats->mode.underlying()) : 0,
					camera ? (camera->QCameraEquals(RE::CameraState::kVATS) ? 1 : 0) : 0,
					camera ? (camera->QCameraEquals(RE::CameraState::kAnimated) ? 1 : 0) : 0);
			}
			return now;
		}

		static void InvalidateCache()
		{
			g_NiCamera = nullptr;
			g_CameraRoot = nullptr;
			g_NiNodeHooksInstalled = false;
		}

		static void RestorePseudoRig(RE::NiAVObject* a_this)
		{
			if (!IsActive() || IsBlockingMenuOpen() || IsVATSKillCamActiveDiag())
				return;

			// fast path: cache populated - resolve by pointer comparison only.
			// Only probe VATS/camera state for actual camera nodes, never for
			// every scene-graph node (that is the hottest path in the game).
			if (g_NiCamera || g_CameraRoot) {
				if (a_this == g_NiCamera || a_this == g_CameraRoot) {
					if (IsVATSKillCamActiveDiag())
						return;
					ForceCameraToHead();
				}
				return;
			}

			// slow path: resolve the camera nodes once, then re-check
			auto* camera = RE::PlayerCamera::GetSingleton();
			if (!camera)
				return;
			auto* tesCam = static_cast<RE::TESCamera*>(camera);
			g_CameraRoot = tesCam->cameraRoot.get();
			g_NiCamera = FindNiCamera(tesCam);
			if (a_this == g_NiCamera || a_this == g_CameraRoot) {
				if (IsVATSKillCamActiveDiag())
					return;
				ForceCameraToHead();
			}
		}

		// --- scene graph detours ---
		static void DetourUpdateWorldData(RE::NiAVObject* a_this, RE::NiUpdateData* a_data)
		{
			UpdateWorldData_Orig(a_this, a_data);
			RestorePseudoRig(a_this);
		}

		static void DetourUpdateTransformAndBounds(RE::NiAVObject* a_this, RE::NiUpdateData* a_data)
		{
			UpdateTransformAndBounds_Orig(a_this, a_data);
			RestorePseudoRig(a_this);
		}

		static void DetourUpdateTransforms(RE::NiAVObject* a_this, RE::NiUpdateData* a_data)
		{
			UpdateTransforms_Orig(a_this, a_data);
			RestorePseudoRig(a_this);
		}

		static void DetourNiNodeUWD(RE::NiAVObject* a_this, RE::NiUpdateData* a_data)
		{
			NiNodeUWD_Orig(a_this, a_data);
			RestorePseudoRig(a_this);
		}

		static void DetourNiNodeUTB(RE::NiAVObject* a_this, RE::NiUpdateData* a_data)
		{
			NiNodeUTB_Orig(a_this, a_data);
			RestorePseudoRig(a_this);
		}

		static void DetourNiNodeUT(RE::NiAVObject* a_this, RE::NiUpdateData* a_data)
		{
			NiNodeUT_Orig(a_this, a_data);
			RestorePseudoRig(a_this);
		}

		static void InstallVtableHook(std::uintptr_t a_vtabAddr, std::size_t a_slot, void* a_detour, void** a_orig)
		{
			void* funcAddr = *reinterpret_cast<void**>(a_vtabAddr + a_slot * 8);
			if (!funcAddr)
				return;
			if (MH_CreateHook(funcAddr, a_detour, a_orig) == MH_OK)
				MH_EnableHook(funcAddr);
		}

		// base NiAVObject scene-graph hooks - catches camera root / NiCamera
		// updates so the engine can't overwrite the pinned head position after
		// the state updates run
		static void InstallSceneGraphHooks()
		{
			if (g_SceneGraphHooksInstalled)
				return;
			auto avObjVtabAddr = RE::VTABLE::NiAVObject[0].address();
			InstallVtableHook(avObjVtabAddr, 0x34, reinterpret_cast<void*>(&DetourUpdateWorldData), reinterpret_cast<void**>(&UpdateWorldData_Orig));
			InstallVtableHook(avObjVtabAddr, 0x35, reinterpret_cast<void*>(&DetourUpdateTransformAndBounds), reinterpret_cast<void**>(&UpdateTransformAndBounds_Orig));
			InstallVtableHook(avObjVtabAddr, 0x36, reinterpret_cast<void*>(&DetourUpdateTransforms), reinterpret_cast<void**>(&UpdateTransforms_Orig));
			g_SceneGraphHooksInstalled = true;
			LOG_INFO("NiAVObject scene-graph hooks installed");
		}

		// camera root (NiNode) hooks - only hooked if the camera root's vtable
		// slots actually differ from the base NiAVObject ones (avoid double
		// hooking the same function)
		static void InstallNiNodeHooks(RE::NiNode* a_cameraRoot)
		{
			if (g_NiNodeHooksInstalled || !a_cameraRoot)
				return;
			void* nodeVtab = *reinterpret_cast<void**>(a_cameraRoot);
			if (!nodeVtab)
				return;
			std::uintptr_t nodeVtabAddr = reinterpret_cast<std::uintptr_t>(nodeVtab);
			auto avObjVtabAddr = RE::VTABLE::NiAVObject[0].address();

			void* uwAddr = *reinterpret_cast<void**>(nodeVtabAddr + 0x34 * 8);
			void* avUwAddr = *reinterpret_cast<void**>(avObjVtabAddr + 0x34 * 8);
			if (uwAddr && uwAddr != avUwAddr) {
				if (MH_CreateHook(uwAddr, reinterpret_cast<void*>(&DetourNiNodeUWD), reinterpret_cast<void**>(&NiNodeUWD_Orig)) == MH_OK)
					MH_EnableHook(uwAddr);
			}
			else {
				NiNodeUWD_Orig = UpdateWorldData_Orig;
			}

			void* utbAddr = *reinterpret_cast<void**>(nodeVtabAddr + 0x35 * 8);
			void* avUtbAddr = *reinterpret_cast<void**>(avObjVtabAddr + 0x35 * 8);
			if (utbAddr && utbAddr != avUtbAddr) {
				if (MH_CreateHook(utbAddr, reinterpret_cast<void*>(&DetourNiNodeUTB), reinterpret_cast<void**>(&NiNodeUTB_Orig)) == MH_OK)
					MH_EnableHook(utbAddr);
			}
			else {
				NiNodeUTB_Orig = UpdateTransformAndBounds_Orig;
			}

			void* utAddr = *reinterpret_cast<void**>(nodeVtabAddr + 0x36 * 8);
			void* avUtAddr = *reinterpret_cast<void**>(avObjVtabAddr + 0x36 * 8);
			if (utAddr && utAddr != avUtAddr) {
				if (MH_CreateHook(utAddr, reinterpret_cast<void*>(&DetourNiNodeUT), reinterpret_cast<void**>(&NiNodeUT_Orig)) == MH_OK)
					MH_EnableHook(utAddr);
			}
			else {
				NiNodeUT_Orig = UpdateTransforms_Orig;
			}

			g_NiNodeHooksInstalled = true;
			LOG_INFO("NiNode (camera root) hooks installed");
		}
	}

#pragma push_macro("near")
#undef near

	struct NiCamera_Update
	{
		static void thunk(RE::NiCamera* a_this, RE::NiUpdateData* a_data)
		{
			float nearDist = a_this->viewFrustum.near;
			float newNear = GetIC()->UpdateNearDistance(nearDist);

			if (newNear != nearDist)
				a_this->viewFrustum.near = newNear;

			func(a_this, a_data);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

#pragma pop_macro("near")

	Hooks::~Hooks()
	{
		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
	}

	void Hooks::Install()
	{
		Setup();

		LOG_INFO("Installing hooks...");

		HookPlayerCameraUpdate();
		HookNiCameraUpdateWorldData();
		HookThirdPersonStateUpdate();
		HookPipboyMode();
		PseudoFPP::InstallSceneGraphHooks();

		// Register the MenuOpenCloseEvent handler so pseudo is disabled for
		// blocking menus (Pip-Boy, VATS, etc.) and re-enabled afterwards.
		Events::Observer::Get()->Register();

		LOG_INFO("Finished installing hooks.");
	}

	void Hooks::Input()
	{
		LOG_INFO("ProcessInput hook placeholder (UI).");
	}

	void Hooks::Setup()
	{
		LOG_INFO("Resolving hook addresses...");
		LOG_INFO("Finished resolving hook addresses.");
	}

	void Hooks::HookPlayerCameraUpdate()
	{
		REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE::PlayerCamera[0] };

		if (!vtable) {
			LOG_WARN("PlayerCamera::Update hook: failed to resolve vtable");
			return;
		}

		auto original = vtable.write_vfunc(0x03, reinterpret_cast<std::uintptr_t>(Patch::Hooks::Hook_PlayerCameraUpdate));
		Address::Hook::PlayerCameraUpdate_Original = reinterpret_cast<Address::Hook::PlayerCameraUpdateFunc>(original);

		LOG_INFO("PlayerCamera::Update hook installed successfully");
	}

	void Hooks::Hook_PlayerCameraUpdate(RE::PlayerCamera* a_this)
	{
		auto ic = GetIC();
		const bool pseudoActive = ic && ic->IsPseudoFPPActive();

		// let the engine run its normal camera update first, then re-pin the
		// pseudo head position so the engine's own TPP math can't win
		Address::Hook::PlayerCameraUpdate_Original(a_this);

		if (!pseudoActive || PseudoFPP::IsBlockingMenuOpen() || PseudoFPP::IsVATSKillCamActiveDiag())
			return;

		auto* tesCam = static_cast<RE::TESCamera*>(a_this);
		if (!tesCam)
			return;
		auto* cr = tesCam->cameraRoot.get();
		if (!cr)
			return;

		PseudoFPP::InstallNiNodeHooks(cr);

		PseudoFPP::ForceCameraToHead();
	}

	void Hooks::HookThirdPersonStateUpdate()
	{
		REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE::ThirdPersonState[0] };

		if (!vtable) {
			LOG_WARN("ThirdPersonState::Update hook: failed to resolve vtable");
			return;
		}

		auto original = vtable.write_vfunc(0x0B, reinterpret_cast<std::uintptr_t>(Patch::Hooks::Hook_ThirdPersonStateUpdate));
		Address::Hook::ThirdPersonStateUpdate_Original = reinterpret_cast<Address::Hook::ThirdPersonStateUpdateFunc>(original);

		LOG_INFO("ThirdPersonState::Update hook installed successfully");
	}

	void Hooks::Hook_ThirdPersonStateUpdate(RE::ThirdPersonState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState)
	{
		// BEFORE calling the original: if Pip-Boy is active, prevent the
		// engine from transitioning away from the Pip-Boy camera state.
		// When a weapon is drawn the engine's ThirdPersonState::Update
		// unconditionally sets a_nextState to kFirstPerson, which would
		// overwrite the Pip-Boy camera every frame ("constantly overwritten").
		// Clear a_nextState so the camera stays in its current (Pip-Boy) state.
		auto* camera = RE::PlayerCamera::GetSingleton();
		if (camera && camera->pipboyMode) {
			a_nextState = nullptr;
			return;
		}

		Address::Hook::ThirdPersonStateUpdate_Original(a_this, a_nextState);

		auto ic = GetIC();
		if (!ic || !ic->IsPseudoFPPActive() || PseudoFPP::IsBlockingMenuOpen() || PseudoFPP::IsVATSKillCamActiveDiag())
			return;

		if (a_nextState && (a_nextState->id == RE::CameraState::kFirstPerson || a_nextState->id == RE::CameraState::kVATS))
			return;

		PseudoFPP::ForceCameraToHead();
	}

	void Hooks::HookPipboyMode()
	{
		// Hook StartPipboyMode and StopPipboyMode to disable/enable pseudo
		// at the correct time — BEFORE the engine sets up the Pip-Boy camera
		// (not after, as MenuOpenCloseEvent does). This prevents pseudo's
		// scene-graph/camera hooks from interfering with the Pip-Boy camera
		// state setup, which is the root cause of the weapon-drawn Pip-Boy
		// conflict.
		void* startAddr = reinterpret_cast<void*>(REL::ID(RE::ID::PlayerCamera::StartPipboyMode).address());
		if (!startAddr) {
			LOG_WARN("StartPipboyMode hook: failed to resolve address");
			return;
		}
		if (MH_CreateHook(startAddr, reinterpret_cast<void*>(&Hook_StartPipboyMode),
			reinterpret_cast<void**>(&Address::Hook::StartPipboyMode_Original)) != MH_OK) {
			LOG_WARN("StartPipboyMode hook: failed to create hook");
			return;
		}
		MH_EnableHook(startAddr);
		LOG_INFO("StartPipboyMode hook installed successfully");

		void* stopAddr = reinterpret_cast<void*>(REL::ID(RE::ID::PlayerCamera::StopPipboyMode).address());
		if (!stopAddr) {
			LOG_WARN("StopPipboyMode hook: failed to resolve address");
			return;
		}
		if (MH_CreateHook(stopAddr, reinterpret_cast<void*>(&Hook_StopPipboyMode),
			reinterpret_cast<void**>(&Address::Hook::StopPipboyMode_Original)) != MH_OK) {
			LOG_WARN("StopPipboyMode hook: failed to create hook");
			return;
		}
		MH_EnableHook(stopAddr);
		LOG_INFO("StopPipboyMode hook installed successfully");
	}

	void Hooks::Hook_StartPipboyMode(RE::PlayerCamera* a_this, bool a_forcePipboyModeCamera)
	{
		// Disable pseudo BEFORE the engine sets up the Pip-Boy camera, so
		// no pseudo hooks (ForceCameraToHead, scene-graph hooks) interfere
		// with StartPipboyMode()'s camera state transition.
		auto ic = GetIC();
		const bool pseudoWasActive = ic && ic->IsPseudoFPPActive();
		if (pseudoWasActive) {
			ic->m_PseudoActiveBeforePipboy = true;
			LOG_INFO("Pseudo-FPP: StartPipboyMode -> disabling pseudo (a_forcePipboyModeCamera={})", a_forcePipboyModeCamera ? 1 : 0);
			ic->SetPseudoFPPActive(false);
		} else {
			ic->m_PseudoActiveBeforePipboy = false;
		}

		// If a weapon is drawn, the camera is currently in k3rdPerson
		// (pseudo's state). The engine's StartPipboyMode expects to
		// transition from kFirstPerson (weapon-drawn first-person) to
		// Pip-Boy, which properly sheathes the weapon and shows the
		// Pip-Boy at the correct distance. If we leave the camera in
		// k3rdPerson, the Pip-Boy appears tiny (rendered from third-person
		// distance). Switch to kFirstPerson first, BEFORE the engine's
		// pipboy transition takes over.
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player && Helper::IsWeaponDrawn(player)) {
			RE::PlayerCamera* pc = RE::PlayerCamera::GetSingleton();
			if (pc && pc->GetState(RE::CameraState::kFirstPerson))
				pc->SetState(pc->GetState(RE::CameraState::kFirstPerson).get());
			LOG_INFO("Pseudo-FPP: StartPipboyMode -> set kFirstPerson (weapon drawn) for pipboy transition");
		}

		Address::Hook::StartPipboyMode_Original(a_this, a_forcePipboyModeCamera);
	}

	void Hooks::Hook_StopPipboyMode(RE::PlayerCamera* a_this)
	{
		Address::Hook::StopPipboyMode_Original(a_this);

		// Re-enable pseudo only if it was active when Pip-Boy opened.
		// The re-enable is deferred via m_PseudoPendingK3rdPersonPush so
		// k3rdPerson is pushed at a safe frame in PerFrameUpdate, not here.
		auto ic = GetIC();
		if (ic && ic->m_PseudoActiveBeforePipboy && !ic->IsPseudoFPPActive()) {
			LOG_INFO("Pseudo-FPP: StopPipboyMode -> re-enabling pseudo");
			ic->SetPseudoFPPActive(true);
			ic->m_PseudoPendingK3rdPersonPush = true;
			ic->m_PseudoPushedK3rdPerson = false;
			ic->m_PseudoReenableFrameCount = 30;  // ~0.5s at 60fps
		}
		if (ic)
			ic->m_PseudoActiveBeforePipboy = false;
	}

	void Hooks::HookNiCameraUpdateWorldData()
	{
		REL::Relocation<std::uintptr_t> vtable{ REL::ID(1305073) };

		if (!vtable) {
			LOG_WARN("NiCamera UpdateWorldData hook: failed to resolve vtable (ID 1305073)");
			return;
		}

		auto original = vtable.write_vfunc(0x34, reinterpret_cast<std::uintptr_t>(Patch::Hooks::Hook_NiCamera_UpdateWorldData));
		Address::Hook::NiCamera_UpdateWorldData_Original = reinterpret_cast<decltype(Address::Hook::NiCamera_UpdateWorldData_Original)>(original);

		LOG_INFO("NiCamera UpdateWorldData hook installed successfully");
	}

	void Hooks::Hook_NiCamera_UpdateWorldData(RE::NiCamera* a_this, RE::NiUpdateData* a_data)
	{
		Address::Hook::NiCamera_UpdateWorldData_Original(a_this, a_data);

		// re-pin after the NiCamera recomputed its own world transform
		PseudoFPP::RestorePseudoRig(a_this);
	}

	void Hooks::InvalidatePseudoCameraCache()
	{
		PseudoFPP::InvalidateCache();
	}

	bool Hooks::IsVATSActive()
	{
		return PseudoFPP::IsVATSKillCamActive();
	}

}
