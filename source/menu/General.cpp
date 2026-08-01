/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "menu/General.h"

#include "plugin.h"

namespace Menu {

	bool MenuGeneral::OnUpdate()
	{
		auto config = DLLMain::Plugin::Get()->Config();

		ImGui::Checkbox("Enable Body", &config->m_General.bEnableBody);
		ImGui::Checkbox("Enable Body Console", &config->m_General.bEnableBodyConsole);
		ImGui::Checkbox("Enable Shadows", &config->m_General.bEnableShadows);
		ImGui::Checkbox("Adjust Player Scale", &config->m_General.bAdjustPlayerScale);
		ImGui::SliderFloat("Body Height Offset", &config->m_General.fBodyHeightOffset, -50.0f, 50.0f, "%.1f");
		ImGui::Separator();
		ImGui::Checkbox("Enable Head", &config->m_General.bEnableHead);
		ImGui::Checkbox("Enable Head Combat", &config->m_General.bEnableHeadCombat);
		ImGui::Checkbox("Enable Head Mount", &config->m_General.bEnableHeadMount);
		ImGui::Checkbox("Enable Head Scripted", &config->m_General.bEnableHeadScripted);
		ImGui::Separator();
		ImGui::Checkbox("Enable Third Person Arms", &config->m_General.bEnableThirdPersonArms);

		return false;
	}

}

