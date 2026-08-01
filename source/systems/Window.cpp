/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "systems/Window.h"

#include "plugin.h"
#include "utils/Log.h"

#include <dwmapi.h>

namespace Systems {

	LRESULT CALLBACK Window::ApplicationMessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		auto pluginGraphics = DLLMain::Plugin::Get()->Graphics();

		if (!pluginGraphics->UI())
			return CallWindowProc(pluginGraphics->Window()->m_Properties->Wndproc, hWnd, msg, wParam, lParam);

		if (!pluginGraphics->UI()->IsUIDisplayed() && pluginGraphics->Window()->m_MenuMode == Window::UIDisplay::kOverlay)
		{
			switch (msg)
			{
				case WM_WINDOWPOSCHANGED:
				case WM_MOVE:
				case WM_SIZE:
				{
					RECT rectClient{}, rectWindow{};
					glm::ivec2 size = { 0, 0 }, position = { 0, 0 };

					if (GetClientRect(hWnd, &rectClient))
					{
						size = { rectClient.right - rectClient.left, rectClient.bottom - rectClient.top };
					}
					if (GetWindowRect(hWnd, &rectWindow))
					{
						position = { ((rectWindow.right - rectWindow.left) - size.x) / 2, (rectWindow.bottom - rectWindow.top) - size.y };
						position.y -= position.x;
						if (position.y < 0)
							position.y = 0;

						SetWindowPos(pluginGraphics->Window()->m_MenuHwnd, nullptr, rectWindow.left + position.x, rectWindow.top + position.y, size.x, size.y, SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
					}
					break;
				}
			}
		}
		return CallWindowProc(pluginGraphics->Window()->m_Properties->Wndproc, hWnd, msg, wParam, lParam);
	}

	Window::Window()
	{
		m_Initialized = true;
		m_Properties = &properties;

		auto pluginConfig = DLLMain::Plugin::Get()->Config();

		m_Properties->name = pluginConfig->ModuleData().sWindowName;
		LOG_INFO("Searching for {} window...", m_Properties->name.c_str());

		m_MenuMode = pluginConfig->ModuleData().iMenuMode;

		if (m_MenuMode > UIDisplay::kNone)
		{
			std::int32_t maxTimeout = pluginConfig->ModuleData().iMenuTimeout - 1;
			std::int32_t timeout = 0;

			if (maxTimeout < 0)
				maxTimeout = 0;
			if (maxTimeout > 180)
				maxTimeout = 180;

			while (!m_Properties->hWnd)
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));

				timeout++;
				if (timeout > maxTimeout)
					break;

				m_Properties->hWnd = FindWindow(nullptr, m_Properties->name.c_str());
			}

			if (m_Properties->hWnd)
			{
				LOG_TRACE("  Found:\t\t\t\t{} in {} secs", m_Properties->name.c_str(), timeout);
				LOG_TRACE("  Handle:\t\t\t\t0x{:016X}", (std::uint64_t)m_Properties->hWnd);

				RECT rectClient{}, rectWindow{};
				if (GetClientRect(m_Properties->hWnd, &rectClient))
				{
					m_Properties->size = { rectClient.right - rectClient.left, rectClient.bottom - rectClient.top };
				}
				if (GetWindowRect(m_Properties->hWnd, &rectWindow))
				{
					m_Properties->position = { ((rectWindow.right - rectWindow.left) - m_Properties->size.x) / 2,
						(rectWindow.bottom - rectWindow.top) - m_Properties->size.y };

					m_Properties->position.y -= m_Properties->position.x;

					if (m_Properties->position.y < 0)
						m_Properties->position.y = 0;
				}

				DisableProcessWindowsGhosting();
				m_Properties->Wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(m_Properties->hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ApplicationMessageHandler)));

				HMODULE hModule = nullptr;
				char moduleName[MAX_PATH]{};
				if (GetWindowModuleFileName(m_Properties->hWnd, moduleName, MAX_PATH) == 0)
				{
					if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCSTR>(m_Properties->Wndproc), &hModule))
					{
						if (GetModuleFileName(hModule, moduleName, MAX_PATH) == 0)
							moduleName[0] = '\0';
					}
				}

				LOG_TRACE("  WNDPROC:\t\t\t0x{:016X}", (std::uint64_t)m_Properties->Wndproc);
			}
			else
			{
				m_Initialized = false;
				LOG_ERROR("Failed to find {} window.", m_Properties->name.c_str());
			}
		}
	}

	Window::~Window()
	{
		if (m_MenuMode == UIDisplay::kOverlay)
		{
			m_WindowSystemRunning = false;
		}
	}

	bool Window::CreateOverlay()
	{
		auto pluginConfig = DLLMain::Plugin::Get()->Config();

		WNDCLASSEX wc{};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.lpfnWndProc = MessageHandler;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.lpszClassName = "SomaticCameraFO4_Overlay";
		wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
		RegisterClassEx(&wc);

		m_MenuHwnd = CreateWindowEx(
			WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
			"SomaticCameraFO4_Overlay",
			"SomaticCameraFO4_Overlay",
			WS_POPUP,
			m_Properties->position.x, m_Properties->position.y,
			m_Properties->size.x, m_Properties->size.y,
			nullptr, nullptr,
			wc.hInstance,
			nullptr);

		if (!m_MenuHwnd)
			return false;

		SetLayeredWindowAttributes(m_MenuHwnd, RGB(0, 0, 0), 255, LWA_COLORKEY | LWA_ALPHA);

		MARGINS margins{ -1, -1, -1, -1 };
		DwmExtendFrameIntoClientArea(m_MenuHwnd, &margins);

		SetWindowPos(m_MenuHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		ShowWindow(m_MenuHwnd, SW_SHOW);

		return true;
	}

	void Window::RunOverlay()
	{
		if (m_MenuMode != UIDisplay::kOverlay)
			return;

		if (!m_MenuHwnd)
		{
			if (!CreateOverlay())
			{
				LOG_ERROR("Failed to create overlay window.");
				return;
			}
		}

		SetWindowPos(m_MenuHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		MSG msg{};
		while (m_WindowSystemRunning)
		{
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			HWND foregroundWindow = GetForegroundWindow();
			if (foregroundWindow == m_Properties->hWnd || foregroundWindow == m_MenuHwnd)
			{
				SetWindowPos(m_MenuHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				ShowWindow(m_MenuHwnd, SW_SHOW);
			}
			else
			{
				ShowWindow(m_MenuHwnd, SW_HIDE);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		DestroyWindow(m_MenuHwnd);
	}

	LRESULT CALLBACK Window::MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		auto pluginGraphics = DLLMain::Plugin::Get()->Graphics();
		if (pluginGraphics->UI())
			pluginGraphics->UI()->WndProcHandler(hWnd, msg, wParam, lParam);

		switch (msg)
		{
			case WM_DESTROY:
				PostQuitMessage(0);
				break;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

}

