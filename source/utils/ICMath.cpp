/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"
#include "utils/ICMath.h"

namespace ICMath
{
	float DegToRad(float degrees)
	{
		constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
		return degrees * DEG_TO_RAD;
	}

	float RadToDeg(float radians)
	{
		constexpr float RAD_TO_DEG = 180.0f / 3.14159265358979323846f;
		return radians * RAD_TO_DEG;
	}

	float Lerp(float a, float b, float t)
	{
		return a + (b - a) * t;
	}

	RE::NiPoint3 Lerp(const RE::NiPoint3& a, const RE::NiPoint3& b, float t)
	{
		return RE::NiPoint3(
			Lerp(a.x, b.x, t),
			Lerp(a.y, b.y, t),
			Lerp(a.z, b.z, t));
	}

	float Clamp(float value, float min, float max)
	{
		if (value < min) {
			return min;
		}
		if (value > max) {
			return max;
		}
		return value;
	}
}

