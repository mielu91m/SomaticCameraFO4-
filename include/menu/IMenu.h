/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <imgui.h>

namespace Interface {

	class IMenu {

	public:
		IMenu(const std::string& a_name) : m_Name(a_name) {}
		virtual ~IMenu() = default;

		virtual void OnOpen() {};
		virtual void OnClose() {};
		virtual bool OnUpdate() = 0;

		const std::string& Name() const { return m_Name; };

	protected:
		std::string m_Name{};
	};

	class Menu {

	public:
		Menu() = default;
		~Menu();

		void Register(IMenu* a_menu);
		void Unregister(IMenu* a_menu);

		auto begin() noexcept { return m_Menus.begin(); }
		auto end() noexcept { return m_Menus.end(); }

	private:
		std::vector<IMenu*> m_Menus{};
	};

}

