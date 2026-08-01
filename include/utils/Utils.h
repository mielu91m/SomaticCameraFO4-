/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <string>
#include <vector>

namespace Utils
{
	std::string GetCurrentDirectory();

	std::string GetFileInfo(const std::string& a_path, const std::string& a_info);

	bool GetVersionFromString(const std::string& a_version, std::uint32_t a_dest[4]);

	bool PluginExists(const std::string& a_name, bool a_fullPath);

	std::string GetFileName(const std::string& a_path);

	std::string GetFileExtension(const std::string& a_path);
}

