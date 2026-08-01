/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "menu/IMenu.h"

namespace Interface {

	Menu::~Menu()
	{
		for (auto& menu : m_Menus)
		{
			if (menu)
				delete menu;
		}
		m_Menus.clear();
	}

	void Menu::Register(IMenu* a_menu)
	{
		if (!a_menu)
			return;
		m_Menus.push_back(a_menu);
	}

	void Menu::Unregister(IMenu* a_menu)
	{
		if (!a_menu)
			return;

		auto it = std::find(m_Menus.begin(), m_Menus.end(), a_menu);
		if (it != m_Menus.end())
		{
			m_Menus.erase(it);
			delete a_menu;
		}
	}

}

