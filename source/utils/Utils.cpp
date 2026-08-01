/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"
#include "utils/Utils.h"

namespace Utils
{
	std::string GetCurrentDirectory()
	{
		std::string result;
		auto size = ::GetCurrentDirectoryA(0, nullptr);
		if (size > 0) {
			std::vector<char> buf(size);
			::GetCurrentDirectoryA(size, buf.data());
			result = buf.data();
		}
		return result;
	}

	std::string GetFileInfo(const std::string& a_path, const std::string& a_info)
	{
		std::string result;
		auto size = ::GetFileVersionInfoSizeA(a_path.c_str(), nullptr);
		if (size > 0) {
			std::vector<char> buf(size);
			if (::GetFileVersionInfoA(a_path.c_str(), 0, size, buf.data())) {
				char* ver = nullptr;
				UINT verLen = 0;
				if (::VerQueryValueA(buf.data(), a_info.c_str(), reinterpret_cast<void**>(&ver), &verLen)) {
					result = std::string(ver, verLen);
				}
			}
		}
		return result;
	}

	bool GetVersionFromString(const std::string& a_version, std::uint32_t a_dest[4])
	{
		std::istringstream ss(a_version);
		std::string token;
		int idx = 0;
		while (std::getline(ss, token, '.')) {
			if (idx >= 4) {
				return false;
			}
			char* end = nullptr;
			auto val = static_cast<std::uint32_t>(std::strtoul(token.c_str(), &end, 10));
			if (*end != '\0') {
				return false;
			}
			a_dest[idx++] = val;
		}
		return idx == 4;
	}

	bool PluginExists(const std::string& a_name, bool a_fullPath)
	{
		auto path = a_name;
		if (!a_fullPath) {
			auto dir = Utils::GetCurrentDirectory();
			path = dir + "\\" + a_name;
		}
		auto attrib = ::GetFileAttributesA(path.c_str());
		return attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY);
	}

	std::string GetFileName(const std::string& a_path)
	{
		auto pos = a_path.find_last_of("\\/");
		if (pos != std::string::npos) {
			return a_path.substr(pos + 1);
		}
		return a_path;
	}

	std::string GetFileExtension(const std::string& a_path)
	{
		auto pos = a_path.find_last_of('.');
		if (pos != std::string::npos) {
			return a_path.substr(pos + 1);
		}
		return {};
	}
}

