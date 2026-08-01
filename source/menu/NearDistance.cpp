/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "menu/NearDistance.h"

#include "plugin.h"

namespace Menu {

	bool MenuNearDistance::OnUpdate()
	{
		auto config = DLLMain::Plugin::Get()->Config();

		ImGui::Checkbox("Enable Near Distance Override", &config->m_NearDistance.bEnableOverride);
		ImGui::Separator();

		if (config->m_NearDistance.bEnableOverride)
		{
			ImGui::SliderFloat("Default", &config->m_NearDistance.fFirstPersonDefault, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("Pitch Threshold", &config->m_NearDistance.fPitchThreshold, 0.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("First Person", &config->m_NearDistance.fFirstPerson, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("First Person Combat", &config->m_NearDistance.fFirstPersonCombat, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("Sitting", &config->m_NearDistance.fSitting, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("Furniture", &config->m_NearDistance.fFurniture, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("Crafting", &config->m_NearDistance.fCrafting, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("Ragdoll", &config->m_NearDistance.fRagdoll, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("Death", &config->m_NearDistance.fDeath, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("Mount", &config->m_NearDistance.fMount, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("Mount Combat", &config->m_NearDistance.fMountCombat, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("Dialogue", &config->m_NearDistance.fDialogue, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("Scripted", &config->m_NearDistance.fScripted, 1.0f, 100.0f, "%.0f");
			ImGui::SliderFloat("Third Person", &config->m_NearDistance.fThirdPerson, 1.0f, 100.0f, "%.0f");
		}

		return false;
	}

}

