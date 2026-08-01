/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

namespace Menu {

	class UIMenu;
}

namespace Systems {

	class UI {

	public:
		UI() = default;
		virtual ~UI() = default;

		virtual bool Initialize() = 0;
		virtual void BeginFrame() = 0;
		virtual void OnUpdate() = 0;
		virtual void EndFrame() = 0;
		virtual bool IsUIDisplayed() = 0;
		virtual void WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
		virtual Menu::UIMenu* GetMenu() = 0;

		static UI* CreateMenu(HWND hWnd);
	};

}

