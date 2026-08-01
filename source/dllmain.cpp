/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "plugin.h"

#ifdef _DEBUG
#include <iostream>
#endif

static DLLMain::Plugin* g_plugin = nullptr;

BOOL APIENTRY DllMain(HMODULE hinstDLL, DWORD fdwReason, LPVOID)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls(hinstDLL);
#ifdef _DEBUG
			if (!IsDebuggerPresent())
				MessageBox(nullptr, TEXT("Click OK when you have attached your debugger or to carry on without it."), TEXT("Attach Debugger"), MB_ICONINFORMATION | MB_OK);

			AllocConsole();
			FILE* pConsole = nullptr;
			freopen_s(&pConsole, "CONOUT$", "w", stdout);
			freopen_s(&pConsole, "CONIN$", "w", stdin);
			freopen_s(&pConsole, "CONOUT$", "w", stderr);
			if (pConsole)
				fclose(pConsole);

			std::cout << "Debug Console Started:" << std::endl;
#endif
			g_plugin = new DLLMain::Plugin(hinstDLL);
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			delete g_plugin;
			break;
		}
	}
	return TRUE;
}

