/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "cameras/Dialogue.h"

#include "systems/Config.h"
#include "utils/Log.h"

namespace ImprovedCamera {

	CameraDialogue::CameraDialogue() :
		ICamera("Dialogue", RE::CameraStates::kDialogue)
	{
		SetData();
	}

	void CameraDialogue::OnUpdate(std::uint8_t currentID, std::uint8_t previousID)
	{
		if (currentID != RE::CameraStates::kDialogue)
		{
			m_DialogueState = State::kExit;
			return;
		}

		switch (m_DialogueState)
		{
			case State::kExit:
			{
				m_DialogueState = State::kPlaying;
				break;
			}
		}

		SetData();
	}

	bool CameraDialogue::HasControl()
	{
		if (m_DialogueState == State::kExit)
			return false;

		auto data = GetData();
		if (data.EventActive && !*data.EventActive)
			return false;

		return true;
	}

	CameraEvent CameraDialogue::GetEventID()
	{
		return CameraEvent::kDialogue;
	}

	void CameraDialogue::SetData()
	{
		auto config = m_Config;
		m_Data.EventActive = &config->Events().bDialogue;
		m_Data.FOV = &config->FOV().fDialogue;
		m_Data.NearDistance = &config->NearDistance().fDialogue;
	}

}

