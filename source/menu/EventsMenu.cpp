/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "menu/EventsMenu.h"

#include "plugin.h"

namespace Menu {

	bool MenuEvents::OnUpdate()
	{
		auto config = DLLMain::Plugin::Get()->Config();

		ImGui::Checkbox("First Person", &config->m_Events.bFirstPerson);
		ImGui::Checkbox("First Person Combat", &config->m_Events.bFirstPersonCombat);
		ImGui::Checkbox("Furniture", &config->m_Events.bFurniture);
		ImGui::Checkbox("Crafting", &config->m_Events.bCrafting);
		ImGui::Checkbox("Ragdoll", &config->m_Events.bRagdoll);
		ImGui::Checkbox("Death", &config->m_Events.bDeath);
		ImGui::Checkbox("Mount", &config->m_Events.bMount);
		ImGui::Checkbox("Mount Combat", &config->m_Events.bMountCombat);
		ImGui::Checkbox("Dialogue", &config->m_Events.bDialogue);
		ImGui::Checkbox("Scripted", &config->m_Events.bScripted);
		ImGui::Checkbox("Third Person", &config->m_Events.bThirdPerson);

		return false;
	}

}

