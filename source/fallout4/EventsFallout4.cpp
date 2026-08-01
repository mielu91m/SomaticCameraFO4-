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

		if (a_event.opening && isBlockingMenu && ic->IsPseudoFPPActive()) {
			pseudoWasActiveBeforeMenu = true;
			ic->SetPseudoFPPActive(false);
			// Set state inactive first (which invalidates cache and resets m_PseudoPushedK3rdPerson).
			// Then pop pseudo's k3rdPerson from the camera stack if the camera
			// is currently in it (i.e. we pushed it before the menu opened).
			auto playerCamera = RE::PlayerCamera::GetSingleton();
			if (playerCamera && playerCamera->QCameraEquals(RE::CameraState::k3rdPerson)) {
				playerCamera->PopState();
				LOG_INFO("Pseudo-FPP: popped k3rdPerson for menu {}", a_event.menuName);
			}
			LOG_INFO("Pseudo-FPP: disabled for menu {}", a_event.menuName);
		}
		else if (!a_event.opening && isBlockingMenu && pseudoWasActiveBeforeMenu) {
			pseudoWasActiveBeforeMenu = false;
			ic->SetPseudoFPPActive(true);
			// Signal PerFrameUpdate to push k3rdPerson on the next
			// safe frame, rather than pushing here during the VATS-exit
			// transition which can corrupt the camera state stack.
			ic->m_PseudoPendingK3rdPersonPush = true;
			ic->m_PseudoPushedK3rdPerson = false;
			LOG_INFO("Pseudo-FPP: re-enabled after menu (pending k3rdPerson push)");
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

