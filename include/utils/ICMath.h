/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <RE/Fallout.h>

namespace ICMath
{
	float DegToRad(float degrees);

	float RadToDeg(float radians);

	float Lerp(float a, float b, float t);

	RE::NiPoint3 Lerp(const RE::NiPoint3& a, const RE::NiPoint3& b, float t);

	float Clamp(float value, float min, float max);
}

