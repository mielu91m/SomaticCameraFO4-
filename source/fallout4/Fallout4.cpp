/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include <psapi.h>

#include "fallout4/Fallout4.h"

#include "fallout4/Hooks.h"
#include "fallout4/ImprovedCameraFO4.h"
#include "plugin.h"
#include "utils/Log.h"
#include "utils/Utils.h"
#include "version.h"

namespace Patch {

	Fallout4::Fallout4()
	{
		m_Name = "Fallout4";
		m_FullName = "Fallout4.exe";
		m_WindowName = "Fallout4";
		m_Path = Utils::GetCurrentDirectory();

		ExecutableInfo();
	}

	void Fallout4::OnLoad()
	{
		m_Hooks = std::make_unique<Hooks>();
		m_Camera = std::make_unique<ImprovedCamera::ImprovedCameraFO4>();

		m_Hooks->Install();
		m_OnLoaded = true;
	}

	void Fallout4::InstallInput()
	{
		if (m_OnLoaded)
			m_Hooks->Input();
	}

	void Fallout4::LoadGame()
	{
		m_Camera->ResetState(true);
	}

	void Fallout4::ExecutableInfo()
	{
		std::string fullFilePath = m_Path;
		if (!fullFilePath.empty() && fullFilePath.back() != '\\') {
			fullFilePath += '\\';
		}
		fullFilePath += m_FullName;

		auto fileVersion = REL::GetFileVersion(std::string_view(fullFilePath));
		if (!fileVersion) {
			fileVersion = REL::GetFileVersion(std::string_view(m_FullName));
		}
		if (fileVersion) {
			m_VersionMajor = (*fileVersion)[0];
			m_VersionMinor = (*fileVersion)[1];
			m_VersionRevision = (*fileVersion)[2];
			m_VersionBuild = (*fileVersion)[3];
		} else {
			LOG_WARN("Failed to read version from {}", fullFilePath.c_str());
		}

		HMODULE module = GetModuleHandleA(m_FullName.c_str());
		if (module)
		{
			MODULEINFO moduleInfo{};
			if (GetModuleInformation(GetCurrentProcess(), module, &moduleInfo, sizeof(moduleInfo)))
			{
				m_BaseAddress = reinterpret_cast<std::uintptr_t>(moduleInfo.lpBaseOfDll);
				m_ImageSize = moduleInfo.SizeOfImage;
			}
		}
	}

}

