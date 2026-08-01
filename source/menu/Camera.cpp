/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "menu/Camera.h"

#include "plugin.h"

namespace Menu {

	bool MenuCamera::OnUpdate()
	{
		auto config = DLLMain::Plugin::Get()->Config();

		if (ImGui::CollapsingHeader("First Person", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("Position X", &config->m_Camera.fFirstPersonPosX, -50.0f, 50.0f, "%.1f");
			ImGui::SliderFloat("Position Y", &config->m_Camera.fFirstPersonPosY, -50.0f, 50.0f, "%.1f");
			ImGui::SliderFloat("Position Z", &config->m_Camera.fFirstPersonPosZ, -50.0f, 50.0f, "%.1f");
		}
		if (ImGui::CollapsingHeader("First Person Combat", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("Position X", &config->m_Camera.fFirstPersonCombatPosX, -50.0f, 50.0f, "%.1f");
			ImGui::SliderFloat("Position Y", &config->m_Camera.fFirstPersonCombatPosY, -50.0f, 50.0f, "%.1f");
			ImGui::SliderFloat("Position Z", &config->m_Camera.fFirstPersonCombatPosZ, -50.0f, 50.0f, "%.1f");
		}
		if (ImGui::CollapsingHeader("Mount"))
		{
			ImGui::SliderFloat("Position X", &config->m_Camera.fMountPosX, -50.0f, 50.0f, "%.1f");
			ImGui::SliderFloat("Position Y", &config->m_Camera.fMountPosY, -50.0f, 50.0f, "%.1f");
			ImGui::SliderFloat("Position Z", &config->m_Camera.fMountPosZ, -50.0f, 50.0f, "%.1f");
		}
		if (ImGui::CollapsingHeader("Mount Combat"))
		{
			ImGui::SliderFloat("Position X", &config->m_Camera.fMountCombatPosX, -50.0f, 50.0f, "%.1f");
			ImGui::SliderFloat("Position Y", &config->m_Camera.fMountCombatPosY, -50.0f, 50.0f, "%.1f");
			ImGui::SliderFloat("Position Z", &config->m_Camera.fMountCombatPosZ, -50.0f, 50.0f, "%.1f");
		}
		if (ImGui::CollapsingHeader("Scripted"))
		{
			ImGui::SliderFloat("Position X", &config->m_Camera.fScriptedPosX, -50.0f, 50.0f, "%.1f");
			ImGui::SliderFloat("Position Y", &config->m_Camera.fScriptedPosY, -50.0f, 50.0f, "%.1f");
			ImGui::SliderFloat("Position Z", &config->m_Camera.fScriptedPosZ, -50.0f, 50.0f, "%.1f");
		}

		return false;
	}

}

