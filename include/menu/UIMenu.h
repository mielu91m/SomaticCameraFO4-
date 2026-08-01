/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "menu/IMenu.h"
#include "systems/UI.h"

namespace Menu {

	class UIMenu :
		public Systems::UI {

	public:
		UIMenu(HWND hWnd);
		~UIMenu();

		bool Initialize() override;
		void BeginFrame() override;
		void OnUpdate() override;
		void EndFrame() override;
		bool IsUIDisplayed() override;
		void WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
		Menu::UIMenu* GetMenu() override { return this; }

	private:
		bool m_Initialized = false;
		bool m_DisplayMenu = false;
		HWND m_HWnd = nullptr;
		Interface::Menu m_MenuManager{};
	};

}

