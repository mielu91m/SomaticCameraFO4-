/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "menu/Hide.h"

#include "plugin.h"

namespace Menu {

	bool MenuHide::OnUpdate()
	{
		auto config = DLLMain::Plugin::Get()->Config();

		ImGui::Checkbox("Hide Weapon", &config->m_Hide.bWeapon);
		ImGui::Checkbox("Hide While Sitting", &config->m_Hide.bSitting);
		ImGui::Checkbox("Hide While Sleeping", &config->m_Hide.bSleeping);
		ImGui::Checkbox("Hide While Jumping", &config->m_Hide.bJumping);
		ImGui::Checkbox("Hide While Swimming", &config->m_Hide.bSwimming);
		ImGui::Checkbox("Hide While Sneak Rolling", &config->m_Hide.bSneakRoll);
		ImGui::Checkbox("Hide While Attacking", &config->m_Hide.bAttack);
		ImGui::Checkbox("Hide While Power Attacking", &config->m_Hide.bPowerAttack);
		ImGui::Checkbox("Hide During Killmove", &config->m_Hide.bKillmove);

		return false;
	}

}

