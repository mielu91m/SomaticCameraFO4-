/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "systems/Logging.h"

#include "plugin.h"
#include "version.h"

namespace Systems {

	void Logging::Initialize()
	{
		if (m_Initialized)
			return;

		m_Initialized = true;

		auto plugin = DLLMain::Plugin::Get();
		std::string logPath = plugin->Path() + plugin->Name() + ".log";

		try
		{
			auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath, true);
			auto logger = std::make_shared<spdlog::logger>("log", fileSink);

			logger->set_level(spdlog::level::trace);
			logger->flush_on(spdlog::level::trace);

			spdlog::set_default_logger(logger);
			spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

			LOG_INFO("{} v{}", VERSION_PRODUCTNAME_DESCRIPTION_STR, VERSION_STR);
		}
		catch (const spdlog::spdlog_ex& ex)
		{
			MessageBox(nullptr, TEXT("Failed to initialize logging."), TEXT(VERSION_PRODUCTNAME_DESCRIPTION_STR), MB_ICONERROR | MB_OK);
		}
	}

}

