/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "cameras/ICamera.h"

namespace ImprovedCamera {

	class CameraThirdPerson :
		public Interface::ICamera {

	public:
		CameraThirdPerson();
		~CameraThirdPerson() = default;

		struct State {
			enum ThirdPerson : std::uint8_t
			{
				kExit = 0,
				kEnter = 1,
				kIdle = 2,
				kWeaponDrawnEnter = 3,
				kWeaponDrawnIdle = 4,
				kCraftingEnter = 5,
				kCraftingIdle = 6,
				kScriptedEnter = 7,
				kScriptedIdle = 8,
				kTotal = 9
			};
		};

		void OnUpdate(std::uint8_t currentID, std::uint8_t previousID) override;
		bool HasControl() override;
		CameraEvent GetEventID() override;
		std::uint8_t GetStateID() override { return m_ThirdPersonState; }
		Interface::ICamera::Data GetData() override { return m_Data; }
		void OnShutdown() override { m_ThirdPersonState = State::kExit; }

	private:
		void SetData();

		std::uint8_t m_ThirdPersonState = State::kExit;
		Interface::ICamera::Data m_Data{};
	};

}

