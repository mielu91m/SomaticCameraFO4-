/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "api/f4se_api.h"
#include "fallout4/Fallout4.h"
#include "systems/Config.h"
#include "systems/Graphics.h"
#include "systems/Logging.h"

namespace DLLMain {

	class Plugin {

	public:
		explicit Plugin(HMODULE a_module);
		~Plugin();

		Plugin(const Plugin&) = delete;
		Plugin& operator=(const Plugin&) = delete;
		Plugin(Plugin&&) = delete;
		Plugin& operator=(Plugin&&) = delete;

		static Plugin* Get() { return s_Instance; };

	public:
		const std::string& Name() const { return m_Name; };
		const std::string& Description() const { return m_Description; };
		const std::string& Path() const { return m_Path; };
		const std::uint32_t VersionMajor() const { return m_VersionMajor; };
		const std::uint32_t VersionMinor() const { return m_VersionMinor; };
		const std::uint32_t VersionRevision() const { return m_VersionRevision; };
		const std::uint32_t VersionBuild() const { return m_VersionBuild; };
		const bool IsGraphicsInitialized() const { return m_GraphicsInitialized; };

		Patch::Fallout4* Fallout4() const { return m_Fallout4.get(); };
		Systems::Config* Config() const { return m_Config.get(); };
		Systems::Graphics* Graphics() const { return m_Graphics.get(); };

	private:
		std::string m_Name{};
		std::string m_Description{};
		std::string m_Path{};
		std::uint32_t m_VersionMajor = 0;
		std::uint32_t m_VersionMinor = 0;
		std::uint32_t m_VersionRevision = 0;
		std::uint32_t m_VersionBuild = 0;
		bool m_GraphicsInitialized = false;

		Systems::Logging m_Logging;

		std::unique_ptr<Patch::Fallout4> m_Fallout4 = nullptr;
		std::unique_ptr<Systems::Config> m_Config = nullptr;
		std::unique_ptr<Systems::Graphics> m_Graphics = nullptr;

		bool m_Loaded = false;
		bool m_InitializeMenu = false;

	private:
		bool Load();
		void CreateMenu();
		bool CheckFallout4();
		void CheckCompatibility();

	private:
		static Plugin* s_Instance;

		friend void F4SE::F4SEMessageHandler(F4SE::MessagingInterface::Message* message);
		friend bool F4SE::F4SEPlugin_Load(const F4SE::LoadInterface* f4se);
	};

}

