/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "cameras/ICamera.h"

namespace ImprovedCamera {

	class CameraDialogue :
		public Interface::ICamera {

	public:
		CameraDialogue();
		~CameraDialogue() = default;

		struct State {
			enum Dialogue : std::uint8_t
			{
				kExit = 0,
				kPlaying = 1,
				kTotal = 2
			};
		};

		void OnUpdate(std::uint8_t currentID, std::uint8_t previousID) override;
		bool HasControl() override;
		CameraEvent GetEventID() override;
		std::uint8_t GetStateID() override { return m_DialogueState; }
		Interface::ICamera::Data GetData() override { return m_Data; }
		void OnShutdown() override { m_DialogueState = State::kExit; }

	private:
		void SetData();

		std::uint8_t m_DialogueState = State::kExit;
		Interface::ICamera::Data m_Data{};
	};

}

