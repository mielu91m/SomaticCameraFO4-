/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

namespace ImprovedCamera {

	enum CameraEvent : std::uint32_t
	{
		kFirstPerson = 0,
		kFirstPersonCombat = 1,
		kFurniture = 2,
		kCrafting = 3,
		kRagdoll = 4,
		kDeath = 5,
		kMount = 6,
		kMountCombat = 7,
		kDialogue = 8,
		kScripted = 9,
		kThirdPerson = 10,
		kTotal = 11
	};

}

using ::ImprovedCamera::CameraEvent;

