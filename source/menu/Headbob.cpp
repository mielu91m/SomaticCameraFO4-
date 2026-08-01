/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "menu/Headbob.h"

#include "plugin.h"

namespace Menu {

	bool MenuHeadbob::OnUpdate()
	{
		auto config = DLLMain::Plugin::Get()->Config();

		if (ImGui::CollapsingHeader("Enable", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Idle", &config->m_Headbob.bIdle);
			ImGui::Checkbox("Walk", &config->m_Headbob.bWalk);
			ImGui::Checkbox("Run", &config->m_Headbob.bRun);
			ImGui::Checkbox("Sprint", &config->m_Headbob.bSprint);
			ImGui::Checkbox("Combat", &config->m_Headbob.bCombat);
			ImGui::Checkbox("Sneak", &config->m_Headbob.bSneak);
			ImGui::Checkbox("Sneak Roll", &config->m_Headbob.bSneakRoll);
		}

		if (ImGui::CollapsingHeader("Rotation", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("Idle", &config->m_Headbob.fRotationIdle, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat("Walk", &config->m_Headbob.fRotationWalk, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat("Run", &config->m_Headbob.fRotationRun, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat("Sprint", &config->m_Headbob.fRotationSprint, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat("Combat", &config->m_Headbob.fRotationCombat, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat("Sneak", &config->m_Headbob.fRotationSneak, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat("Sneak Roll", &config->m_Headbob.fRotationSneakRoll, 0.0f, 1.0f, "%.3f");
		}

		return false;
	}

}

