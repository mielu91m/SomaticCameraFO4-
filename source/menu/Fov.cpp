/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "menu/Fov.h"

#include "plugin.h"

namespace Menu {

	bool MenuFOV::OnUpdate()
	{
		auto config = DLLMain::Plugin::Get()->Config();

		ImGui::Checkbox("Enable FOV Override", &config->m_FOV.bEnableOverride);
		ImGui::Separator();

		if (config->m_FOV.bEnableOverride)
		{
			ImGui::SliderFloat("First Person", &config->m_FOV.fFirstPerson, 1.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("First Person Combat", &config->m_FOV.fFirstPersonCombat, 1.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Furniture", &config->m_FOV.fFurniture, 1.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Crafting", &config->m_FOV.fCrafting, 1.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Ragdoll", &config->m_FOV.fRagdoll, 1.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Death", &config->m_FOV.fDeath, 1.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Mount", &config->m_FOV.fMount, 1.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Mount Combat", &config->m_FOV.fMountCombat, 1.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Dialogue", &config->m_FOV.fDialogue, 1.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Scripted", &config->m_FOV.fScripted, 1.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Third Person", &config->m_FOV.fThirdPerson, 1.0f, 180.0f, "%.0f");
		}

		return false;
	}

}

