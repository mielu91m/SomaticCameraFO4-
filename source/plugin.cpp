/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "plugin.h"

#include "utils/Utils.h"
#include "version.h"

namespace DLLMain {

	Plugin* Plugin::s_Instance = nullptr;

	Plugin::Plugin(HMODULE a_module)
	{
		if (s_Instance)
		{
			MessageBox(nullptr, TEXT("An unexpected error occurred within plugin initialization."), TEXT(VERSION_PRODUCTNAME_DESCRIPTION_STR), MB_ICONERROR | MB_OK);
			s_Instance = nullptr;
			return;
		}

		s_Instance = this;

		m_Name = VERSION_PRODUCTNAME_STR;
		m_Description = VERSION_PRODUCTNAME_DESCRIPTION_STR;
		m_VersionMajor = VERSION_MAJOR;
		m_VersionMinor = VERSION_MINOR;
		m_VersionRevision = VERSION_REVISION;
		m_VersionBuild = VERSION_BUILD;

		char modulePath[MAX_PATH]{};
		const DWORD moduleSize = GetModuleFileNameA(a_module, modulePath, MAX_PATH);

		if (moduleSize && moduleSize < MAX_PATH)
		{
			std::string fullPath(modulePath);
			const std::size_t pos = fullPath.find_last_of("\\/");
			if (pos != std::string::npos)
				m_Path = fullPath.substr(0, pos + 1);
		}

		m_Config = std::make_unique<Systems::Config>();
		m_Fallout4 = std::make_unique<Patch::Fallout4>();
	}

	Plugin::~Plugin()
	{
#ifdef _DEBUG
		FreeConsole();
#endif
	}

	bool Plugin::Load()
	{
		if (m_Loaded)
			return true;

		m_Loaded = true;

		if (!m_Config->m_PreInitialized)
		{
			LOG_ERROR("Failed to load {}", m_Config->FileName().c_str());
			return false;
		}

		m_Config->m_Initialized = true;
		LOG_DEBUG("Profile:\t\t\t\t{}", m_Config->ModuleData().sProfileName.c_str());

		const auto& pf = m_Config->PseudoFPP();
		LOG_INFO("Config: using '{}' (PSEUDOFPP height={:.4f} forward={:.4f} toggleKey={} adsKey={})",
			m_Config->FileName().c_str(), pf.fHeightOffset, pf.fForwardOffset, pf.iToggleKey, pf.iADSKey);

		if (!CheckFallout4())
			return false;

		if (m_Config->ModuleData().iCheckCompatibility)
			CheckCompatibility();

		return true;
	}

	void Plugin::CreateMenu()
	{
		if (m_InitializeMenu)
			return;

		m_InitializeMenu = true;

		if (m_Config->ModuleData().iMenuMode > Systems::Window::UIDisplay::kNone)
		{
			m_Graphics = std::make_unique<Systems::Graphics>(m_Config->ModuleData().iMenuMode);
		}
	}

	bool Plugin::CheckFallout4()
	{
		LOG_INFO("Checking {} information...", m_Fallout4->FullName().c_str());

		std::uint32_t fileVersionMin[4]{};
		std::uint32_t fileVersionMax[4]{};

		Utils::GetVersionFromString(m_Config->ModuleData().sFileVersionMin, fileVersionMin);
		Utils::GetVersionFromString(m_Config->ModuleData().sFileVersionMax, fileVersionMax);

		if (m_Fallout4->VersionMajor() >= fileVersionMin[0] && m_Fallout4->VersionMajor() <= fileVersionMax[0] &&
			m_Fallout4->VersionMinor() >= fileVersionMin[1] && m_Fallout4->VersionMinor() <= fileVersionMax[1] &&
			m_Fallout4->VersionRevision() >= fileVersionMin[2] && m_Fallout4->VersionRevision() <= fileVersionMax[2] &&
			m_Fallout4->VersionBuild() >= fileVersionMin[3] && m_Fallout4->VersionBuild() <= fileVersionMax[3])
		{
			LOG_TRACE("  Version:\t\t\t\tv{}.{}.{}.{}", m_Fallout4->VersionMajor(), m_Fallout4->VersionMinor(), m_Fallout4->VersionRevision(), m_Fallout4->VersionBuild());
			LOG_TRACE("  Base Address:\t\t\t0x{:016X}", m_Fallout4->BaseAddress());
			LOG_TRACE("  Image Size:\t\t\t0x{:016X}", m_Fallout4->ImageSize());
		}
		else
		{
			LOG_CRITICAL("{}: v{}.{}.{}.{} not supported.", m_Fallout4->Name().c_str(), m_Fallout4->VersionMajor(), m_Fallout4->VersionMinor(), m_Fallout4->VersionRevision(), m_Fallout4->VersionBuild());
			return false;
		}
		return true;
	}

	void Plugin::CheckCompatibility()
	{
		std::string fullFilePath{};
		std::string productName{};
		std::string fileVersion{};

		LOG_INFO("Checking for compatibility issues...");

		bool ReShade = Utils::PluginExists("dxgi.dll", true);

		if (ReShade)
		{
			fullFilePath = m_Fallout4->Path() + "dxgi.dll";
			productName = Utils::GetFileInfo(fullFilePath, "ProductName");

			if (productName.compare("ReShade") != 0)
				ReShade = false;
		}

		LOG_INFO("Finished checking for compatibility issues.");
	}

}

