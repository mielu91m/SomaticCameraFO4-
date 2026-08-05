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

		if (a_event.opening && isBlockingMenu && !isPipboyMenu && ic->IsPseudoFPPActive()) {
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
		else if (!a_event.opening && isBlockingMenu && !isPipboyMenu && pseudoWasActiveBeforeMenu) {
			pseudoWasActiveBeforeMenu = false;
			ic->SetPseudoFPPActive(true);
			// Signal PerFrameUpdate to re-push k3rdPerson on the next
			// safe frame, rather than pushing here during the VATS-exit
			// transition which can corrupt the camera state stack.
			ic->m_PseudoPendingK3rdPersonPush = true;
			ic->m_PseudoPushedK3rdPerson = false;
			ic->m_PseudoReenableFrameCount = 30;  // ~0.5s at 60fps
			LOG_INFO("Pseudo-FPP: re-enabled after menu {} (deferred re-activation)", a_event.menuName);
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

