/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "menu/RestrictAngles.h"

#include "plugin.h"

namespace Menu {

	bool MenuRestrictAngles::OnUpdate()
	{
		auto config = DLLMain::Plugin::Get()->Config();

		if (ImGui::CollapsingHeader("Sitting", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("Angle", &config->m_RestrictAngles.fSitting, 0.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Max Looking Up", &config->m_RestrictAngles.fSittingMaxLookingUp, 0.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Max Looking Down", &config->m_RestrictAngles.fSittingMaxLookingDown, 0.0f, 180.0f, "%.0f");
		}
		if (ImGui::CollapsingHeader("Scripted", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("Angle", &config->m_RestrictAngles.fScripted, 0.0f, 180.0f, "%.0f");
			ImGui::SliderFloat("Pitch", &config->m_RestrictAngles.fScriptedPitch, 0.0f, 180.0f, "%.0f");
		}

		return false;
	}

}

