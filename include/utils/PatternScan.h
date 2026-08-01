/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <cstdint>
#include <string>

namespace Utils
{
	std::uintptr_t FindPattern(const std::string& a_module, const std::string& a_pattern);
}

