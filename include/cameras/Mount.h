/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "cameras/ICamera.h"

namespace ImprovedCamera {

	class CameraMount :
		public Interface::ICamera {

	public:
		CameraMount();
		~CameraMount() = default;

		struct State {
			enum Mount : std::uint8_t
			{
				kExit = 0,
				kMounting = 1,
				kMounted = 2,
				kRiding = 3,
				kRidingWeaponDrawn = 4,
				kDismounting = 5,
				kTotal = 6
			};
		};

		void OnUpdate(std::uint8_t currentID, std::uint8_t previousID) override;
		bool HasControl() override;
		CameraEvent GetEventID() override;
		std::uint8_t GetStateID() override { return m_MountState; }
		Interface::ICamera::Data GetData() override { return m_Data; }
		void OnShutdown() override { m_MountState = State::kExit; }

	private:
		void SetData();

		std::uint8_t m_MountState = State::kExit;
		Interface::ICamera::Data m_Data{};
	};

}

