/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <F4SE/F4SE.h>

namespace F4SE {

	extern void F4SEMessageHandler(F4SE::MessagingInterface::Message* message);
	extern "C" __declspec(dllexport) bool F4SEPlugin_Load(const F4SE::LoadInterface* f4se);

}

