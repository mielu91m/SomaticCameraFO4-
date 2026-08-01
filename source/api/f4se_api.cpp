/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "api/f4se_api.h"

#include "cameras/ICamera.h"
#include "fallout4/ImprovedCameraFO4.h"
#include "plugin.h"
#include "version.h"

namespace F4SE {

	void F4SEMessageHandler(F4SE::MessagingInterface::Message* message)
	{
		auto plugin = DLLMain::Plugin::Get();

		switch (message->type)
		{
			case F4SE::MessagingInterface::kPostPostLoad:
			{
				plugin->Fallout4()->Camera()->RequestAPIs();
				break;
			}
			case F4SE::MessagingInterface::kPostLoadGame:
			{
				plugin->Fallout4()->LoadGame();
				break;
			}
			case F4SE::MessagingInterface::kInputLoaded:
			{
				plugin->CreateMenu();
				break;
			}
			case F4SE::MessagingInterface::kGameDataReady:
			{
				plugin->Fallout4()->Camera()->DetectMods();

				if (plugin->Config()->ModuleData().iMenuMode > Systems::Window::UIDisplay::kNone)
				{
					plugin->Fallout4()->InstallInput();

					if (plugin->Graphics()->IsInitalized())
						plugin->m_GraphicsInitialized = true;
				}
				break;
			}
		}
	}

	bool F4SEPlugin_Load(const F4SE::LoadInterface* f4se)
	{
		auto plugin = DLLMain::Plugin::Get();
		plugin->m_Logging.Initialize();

		F4SE::Init(f4se);
		bool loaded = plugin->Load();

		if (loaded)
		{
			auto msgInterface = F4SE::GetMessagingInterface();
			msgInterface->RegisterListener(F4SEMessageHandler);
			plugin->Fallout4()->OnLoad();
		}
		return loaded;
	}

}

