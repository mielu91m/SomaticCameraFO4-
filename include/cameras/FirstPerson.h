/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "cameras/ICamera.h"

namespace ImprovedCamera {

	class CameraFirstPerson :
		public Interface::ICamera {

	public:
		CameraFirstPerson();
		~CameraFirstPerson() = default;

		struct State {
			enum FirstPerson : std::uint8_t
			{
				kExit = 0,
				kEnter = 1,
				kIdle = 2,
				kWeaponDrawnEnter = 3,
				kWeaponDrawnIdle = 4,
				kScriptedEnter = 5,
				kScriptedIdle = 6,
				kSittingEnter = 7,
				kSittingIdle = 8,
				kTotal = 9
			};
		};

		void OnUpdate(std::uint8_t currentID, std::uint8_t previousID) override;
		bool HasControl() override;
		CameraEvent GetEventID() override;
		std::uint8_t GetStateID() override { return m_FirstPersonState; }
		Interface::ICamera::Data GetData() override { return m_Data; }
		void OnShutdown() override { m_FirstPersonState = State::kExit; }

	private:
		void SetData();

		std::uint8_t m_FirstPersonState = State::kExit;
		Interface::ICamera::Data m_Data{};
	};

}

