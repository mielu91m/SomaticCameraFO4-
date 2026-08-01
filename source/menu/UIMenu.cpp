/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "menu/UIMenu.h"

#include "menu/General.h"
#include "menu/Camera.h"
#include "menu/Fov.h"
#include "menu/Hide.h"
#include "menu/EventsMenu.h"
#include "menu/NearDistance.h"
#include "menu/RestrictAngles.h"
#include "menu/Headbob.h"
#include "plugin.h"
#include "utils/Log.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Menu {

	UIMenu::UIMenu(HWND hWnd) :
		m_HWnd(hWnd)
	{
	}

	UIMenu::~UIMenu()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	bool UIMenu::Initialize()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		auto pluginConfig = DLLMain::Plugin::Get()->Config();
		if (pluginConfig->ModuleData().sMenuFont != "default")
		{
			std::string fontPath = pluginConfig->FontPath() + pluginConfig->ModuleData().sMenuFont;
			io.Fonts->AddFontFromFileTTF(fontPath.c_str(), pluginConfig->ModuleData().fMenuFontSize);
		}

		ImGui::StyleColorsDark();

		if (!ImGui_ImplWin32_Init(m_HWnd))
			return false;

		auto pluginGraphics = DLLMain::Plugin::Get()->Graphics();
		if (!ImGui_ImplDX11_Init(pluginGraphics->m_Device.Get(), pluginGraphics->m_DeviceContext.Get()))
			return false;

		m_MenuManager.Register(new MenuGeneral());
		m_MenuManager.Register(new MenuCamera());
		m_MenuManager.Register(new MenuFOV());
		m_MenuManager.Register(new MenuHide());
		m_MenuManager.Register(new MenuEvents());
		m_MenuManager.Register(new MenuNearDistance());
		m_MenuManager.Register(new MenuRestrictAngles());
		m_MenuManager.Register(new MenuHeadbob());

		m_Initialized = true;
		LOG_INFO("ImGui menu initialized.");
		return true;
	}

	void UIMenu::BeginFrame()
	{
		if (!m_Initialized)
			return;

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void UIMenu::OnUpdate()
	{
		if (!m_Initialized)
			return;

		auto pluginConfig = DLLMain::Plugin::Get()->Config();
		auto io = ImGui::GetIO();

		if (pluginConfig->ModuleData().iMenuKey > 0)
		{
			ImGuiKey menuKey = static_cast<ImGuiKey>(pluginConfig->ModuleData().iMenuKey);
			if (ImGui::IsKeyDown(menuKey) && !io.KeyCtrl && !io.KeyShift && !io.KeyAlt)
			{
				static bool lastState = false;
				bool currentState = ImGui::IsKeyDown(menuKey);
				if (currentState && !lastState)
					m_DisplayMenu = !m_DisplayMenu;
				lastState = currentState;
			}
		}

		if (!m_DisplayMenu)
			return;

		ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Improved Camera FO4", &m_DisplayMenu, ImGuiWindowFlags_MenuBar))
		{
			ImGui::End();
			return;
		}

		if (ImGui::BeginMenuBar())
		{
			for (auto& menu : m_MenuManager)
			{
				if (ImGui::BeginMenu(menu->Name().c_str()))
				{
					menu->OnUpdate();
					ImGui::EndMenu();
				}
			}
			ImGui::EndMenuBar();
		}

		ImGui::End();
	}

	void UIMenu::EndFrame()
	{
		if (!m_Initialized)
			return;

		ImGui::Render();
	}

	bool UIMenu::IsUIDisplayed()
	{
		return m_DisplayMenu;
	}

	void UIMenu::WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
	}

}

