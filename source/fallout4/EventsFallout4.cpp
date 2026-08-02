/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "fallout4/EventsFallout4.h"

#include "fallout4/ImprovedCameraFO4.h"
#include "plugin.h"
#include "utils/Log.h"

namespace Events {

	Observer* Observer::Get()
	{
		static Observer instance;
		return &instance;
	}

void Observer::Register()
	{
		auto ui = RE::UI::GetSingleton();
		if (ui)
			ui->RegisterSink<RE::MenuOpenCloseEvent>(this);

		auto loadGameSource = RE::TESLoadGameEvent::GetEventSource();
		if (loadGameSource)
			loadGameSource->RegisterSink(this);

		LOG_DEBUG("Registered event sinks.");
	}

	RE::BSEventNotifyControl Observer::ProcessEvent(const RE::TESLoadGameEvent& a_event, RE::BSTEventSource<RE::TESLoadGameEvent>*)
	{
		// Game loaded - reset camera state to avoid corrupted stack
		LOG_INFO("Game loaded - resetting camera state");
		ResetCameraState();
		return RE::BSEventNotifyControl::kContinue;
	}

	void Observer::ResetCameraState()
	{
		auto plugin = DLLMain::Plugin::Get();
		auto ic = plugin->Fallout4()->Camera();
		if (ic) {
			ic->SetPseudoFPPActive(false);
			ic->m_PseudoPushedK3rdPerson = false;
			ic->m_PseudoPendingK3rdPersonPush = false;
			LOG_INFO("Camera state reset on load");
		}
	}

	void Observer::CheckSPIM()
	{
		LOG_DEBUG("Checking for ShowPlayerInMenus...");
	}

RE::BSEventNotifyControl Observer::ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		auto plugin = DLLMain::Plugin::Get();
		auto ic = plugin->Fallout4()->Camera();
		auto config = plugin->Config();

		// --- Disable pseudo camera for any blocking menu so the engine
		// can handle the menu camera normally (Pip-Boy, workshop, dialogue, etc.)
		// Re-enable pseudo when the blocking menu closes.
		// For Pip-Boy specifically, also manage the camera stack so the
		// engine gets a clean kFirstPerson without pseudo's k3rdPerson on top.
		static bool pseudoWasActiveBeforeMenu = false;
		const bool isBlockingMenu =
			a_event.menuName == "PipboyMenu" ||
			a_event.menuName == "WorkshopMenu" ||
			a_event.menuName == "DialogueMenu" ||
			a_event.menuName == "ContainerMenu" ||
			a_event.menuName == "BarterMenu" ||
			a_event.menuName == "CraftingMenu" ||
			a_event.menuName == "TerminalMenu" ||
			a_event.menuName == "FavoritesMenu" ||
			a_event.menuName == "MessageBoxMenu" ||
			a_event.menuName == "SleepWaitMenu" ||
			a_event.menuName == "BookMenu" ||
			a_event.menuName == "LockpickingMenu" ||
			a_event.menuName == "VATSMenu" ||
			a_event.menuName == "Console" ||
			a_event.menuName == "LoadingMenu" ||
			a_event.menuName == "MainMenu";

		const bool isVATS = a_event.menuName == "VATSMenu";

		if (a_event.opening && isBlockingMenu && ic->IsPseudoFPPActive()) {
			// VATS safety: if previous VATS cycle left flag stuck, clear it
			if (isVATS && pseudoWasActiveBeforeMenu) {
				LOG_WARN("Pseudo-FPP: VATS opened but flag was stuck - force reset");
				pseudoWasActiveBeforeMenu = false;
			}
			pseudoWasActiveBeforeMenu = true;

			if (isVATS) {
				// VATS: do NOT pop k3rdPerson. The engine manages its own camera
				// during VATS (pushes kVATS on top) and restores k3rdPerson on
				// exit. Popping here unbalances the stack and the engine's kill
				// camera may crash trying to restore the popped state.
				const bool hadPushed = ic->m_PseudoPushedK3rdPerson;
				ic->SetPseudoFPPActive(false);
				ic->m_PseudoPushedK3rdPerson = hadPushed;
				LOG_INFO("Pseudo-FPP: VATS opened - kept k3rdPerson on stack (no pop)");
			} else {
				// Other blocking menus: disable pseudo, then pop pseudo's
				// k3rdPerson if the camera is currently in it.
				ic->SetPseudoFPPActive(false);
				auto playerCamera = RE::PlayerCamera::GetSingleton();
				if (playerCamera && playerCamera->QCameraEquals(RE::CameraState::k3rdPerson)) {
					playerCamera->PopState();
					LOG_INFO("Pseudo-FPP: popped k3rdPerson for menu {}", a_event.menuName);
				}
			}
			LOG_INFO("Pseudo-FPP: disabled for menu {}", a_event.menuName);
		}
		else if (!a_event.opening && isBlockingMenu && pseudoWasActiveBeforeMenu) {
			pseudoWasActiveBeforeMenu = false;

			// Defer pseudo re-activation until the engine has fully finished
			// the menu/kill-camera exit transition. Re-enabling immediately here
			// lets the scene-graph hooks run while camera nodes are still being
			// swapped, which crashes on VATS kill cam. PerFrameUpdate performs
			// the actual re-activation once the delay has elapsed.
			ic->m_PseudoPendingK3rdPersonPush = true;
			ic->m_PseudoPushedK3rdPerson = false;
			Patch::Hooks::InvalidatePseudoCameraCache();
			ic->m_PseudoReenableFrameCount = 30;  // ~0.5s at 60fps
			LOG_INFO("Pseudo-FPP: menu {} closed - deferred re-activation", a_event.menuName);
		}

		if (a_event.menuName == "Console")
		{
			if (a_event.opening && config->General().bEnableBodyConsole)
			{
				auto player = RE::PlayerCharacter::GetSingleton();
				if (player)
				{
					auto thirdPersonNode = player->Get3D(false);
					if (thirdPersonNode)
						thirdPersonNode->local.scale = 1.0f;
				}
			}
			else if (!a_event.opening && config->General().bEnableBodyConsole)
			{
				auto player = RE::PlayerCharacter::GetSingleton();
				if (player)
				{
					auto thirdPersonNode = player->Get3D(false);
					if (thirdPersonNode)
						thirdPersonNode->local.scale = config->General().bEnableBody ? 1.0f : 0.001f;
				}
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	void Observer::ResetArms()
	{
		LOG_DEBUG("ResetArms called");
	}

}

