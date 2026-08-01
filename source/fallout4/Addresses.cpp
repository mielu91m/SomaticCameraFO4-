/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

namespace Address {

	namespace Function {

		void Get3D(void* a_this)
		{
			using func_t = void (*)(void*);
			static REL::Relocation<func_t> func{ REL::ID(0) };
			return func(a_this);
		}

		void ResetNodes(void* a_this)
		{
			using func_t = void (*)(void*);
			static REL::Relocation<func_t> func{ REL::ID(0) };
			return func(a_this);
		}
	}

	namespace Variable {

		float* fDefaultWorldFOV = nullptr;
		float* fMinCurrentZoom = nullptr;
		float* fSittingMaxLookingDown = nullptr;
		float* fMountedMaxLookingUp = nullptr;
		float* fMountedMaxLookingDown = nullptr;
	}

}

