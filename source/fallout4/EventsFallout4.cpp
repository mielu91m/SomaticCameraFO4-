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

		LOG_DEBUG("Registered event sinks.");
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
		// can handle the menu camera normally (workshop, dialogue, etc.)
		// Re-enable pseudo when the blocking menu closes.
		// Pip-Boy is handled separately by the StartPipboyMode /
		// StopPipboyMode hooks (see Hooks.cpp) which disable/enable pseudo
		// at the correct time — BEFORE the engine sets up the Pip-Boy
		// camera, not after (MenuOpenCloseEvent fires too late).
		static bool pseudoWasActiveBeforeMenu = false;
		const bool isPipboyMenu = a_event.menuName == "PipboyMenu";
		const bool isBlockingMenu =
			isPipboyMenu ||
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

		// Hard stand-down for EVERY blocking menu (including Pip-Boy): from
		// the moment a blocking menu opens pseudo must not fight the engine's
		// menu camera, even on the very first frame when the menu may not be
		// registered as open yet. The counter is counted down in
		// PerFrameUpdate; while > 0 every pseudo hook stands down.
		if (a_event.opening && isBlockingMenu && ic) {
			ic->m_PseudoMenuBlockFrames = 30;
		}

		// TerminalMenu is special: the player enters a terminal through
		// furniture, so pseudo has ALREADY been disabled by the
		// StartFurnitureMode hook before this event fires. It must stay
		// disabled for the WHOLE terminal session (menu + the camera
		// transitions into and out of the terminal's first-person view) and
		// be re-enabled only after the terminal has fully closed and the
		// player is back in a gameplay state. This branch is deliberately
		// NOT gated on IsPseudoFPPActive() — the old code gated it there and,
		// because pseudo was already off, the flag was never set, which let
		// PerFrameUpdate re-enable pseudo mid-terminal and soft-lock the
		// player (terminal window/animations captured, can't use or exit).
		if (a_event.opening && isBlockingMenu && a_event.menuName == "TerminalMenu" && ic) {
			ic->m_TerminalMenuIsOpen = true;
			if (ic->IsPseudoFPPActive()) {
				// If pseudo somehow got re-enabled before the menu registered,
				// force it off and mark it for re-enable after the session.
				ic->m_PseudoPendingFurnitureExit = true;
				ic->SetPseudoFPPActive(false);
			}
			LOG_INFO("Pseudo-FPP: TerminalMenu opening — terminal session active, pseudo disabled");
		}
		else if (a_event.opening && isBlockingMenu && !isPipboyMenu && ic->IsPseudoFPPActive()) {
			pseudoWasActiveBeforeMenu = true;
			ic->SetPseudoFPPActive(false);

			if (a_event.menuName == "VATSMenu") {
				// VATS: the engine pushes its own kVATS camera state on top of
				// pseudo's k3rdPerson and restores it on exit. Do NOT pop
				// k3rdPerson here - if the camera is already in kVATS the pop
				// is skipped anyway, and popping during the VATS exit/kill-cam
				// transition corrupts the state stack (player freezes).
				LOG_INFO("Pseudo-FPP: VATS opened - k3rdPerson left on stack");
			} else {
				// Other blocking menus (workshop, dialogue, etc.): just
				// disable pseudo. These menus run after MenuOpenCloseEvent,
				// but they don't have their own StartXxxMode() hook, so we
				// handle them here. Do NOT pop any camera states — the
				// engine manages its own camera stack for these menus.
				LOG_INFO("Pseudo-FPP: disabled for blocking menu {} (no stack manipulation)",
					a_event.menuName);
			}
		}
		else if (!a_event.opening && isBlockingMenu && a_event.menuName == "TerminalMenu" && ic) {
			// Terminal session is over. Keep pseudo disabled until the engine
			// has fully exited the terminal (stand-up animation / camera
			// restore) — PerFrameUpdate re-enables it once the camera is back
			// in a gameplay state. The longer grace period covers the exit
			// animation so pseudo doesn't fight it.
			ic->m_TerminalMenuIsOpen = false;
			ic->m_PseudoMenuBlockFrames = 60;
			LOG_INFO("Pseudo-FPP: TerminalMenu closed — pseudo stays disabled until furniture exit");
		}
		else if (!a_event.opening && isBlockingMenu && !isPipboyMenu && pseudoWasActiveBeforeMenu) {
			// Other blocking menus: re-enable pseudo
			pseudoWasActiveBeforeMenu = false;
			ic->SetPseudoFPPActive(true);
			// Signal PerFrameUpdate to re-push k3rdPerson on the next
			// safe frame, rather than pushing here during the VATS-exit
			// transition which can corrupt the camera state stack.
			ic->m_PseudoPendingK3rdPersonPush = true;
			ic->m_PseudoPushedK3rdPerson = false;
			ic->m_PseudoReenableFrameCount = 30;  // ~0.5s at 60fps
			// Fresh grace period after the menu closes so the engine's
			// camera restore settles before pseudo starts pinning the camera again.
			ic->m_PseudoMenuBlockFrames = 30;
			LOG_INFO("Pseudo-FPP: re-enabled after menu {} (deferred re-activation)", a_event.menuName);
		}
		else if (!a_event.opening && isBlockingMenu && ic && ic->m_PseudoMenuBlockFrames > 0) {
			// Blocking menu closed but pseudo was not re-enabled here
			// (e.g. Pip-Boy, which is re-enabled by the StopPipboyMode hook).
			// Refresh the stand-down grace period anyway so the engine's
			// menu-camera restore settles before any pseudo hook runs again.
			ic->m_PseudoMenuBlockFrames = 30;
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

