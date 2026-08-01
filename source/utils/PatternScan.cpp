/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"
#include "utils/PatternScan.h"

namespace Utils
{
	std::uintptr_t FindPattern(const std::string& a_module, const std::string& a_pattern)
	{
		auto module = ::GetModuleHandleA(a_module.c_str());
		if (!module) {
			return 0;
		}

		auto dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
			return 0;
		}

		auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(
			reinterpret_cast<const std::uint8_t*>(module) + dosHeader->e_lfanew);
		if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
			return 0;
		}

		auto size = ntHeaders->OptionalHeader.SizeOfImage;
		auto data = reinterpret_cast<const std::uint8_t*>(module);

		std::vector<std::pair<std::uint8_t, bool>> pattern;
		for (size_t i = 0; i < a_pattern.size();) {
			if (a_pattern[i] == ' ') {
				i++;
				continue;
			}
			if (a_pattern[i] == '?' && i + 1 < a_pattern.size() && a_pattern[i + 1] == '?') {
				pattern.emplace_back(0x00, true);
				i += 2;
			} else if (a_pattern[i] == '?') {
				pattern.emplace_back(0x00, true);
				i += 1;
			} else {
				auto byte = static_cast<std::uint8_t>(std::strtoul(a_pattern.c_str() + i, nullptr, 16));
				pattern.emplace_back(byte, false);
				i += 2;
			}
		}

		for (std::uintptr_t i = 0; i < size - pattern.size(); i++) {
			bool found = true;
			for (size_t j = 0; j < pattern.size(); j++) {
				if (pattern[j].second) {
					continue;
				}
				if (data[i + j] != pattern[j].first) {
					found = false;
					break;
				}
			}
			if (found) {
				return reinterpret_cast<std::uintptr_t>(module) + i;
			}
		}

		return 0;
	}
}

