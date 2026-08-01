/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "cameras/Transition.h"

#include "systems/Config.h"
#include "utils/Log.h"

namespace ImprovedCamera {

	CameraTransition::CameraTransition() :
		ICamera("Transition", RE::CameraStates::kPCTransition)
	{
		SetData();
	}

	void CameraTransition::OnUpdate(std::uint8_t currentID, std::uint8_t previousID)
	{
		if (currentID != RE::CameraStates::kPCTransition)
		{
			m_TransitionState = State::kExit;
			return;
		}

		switch (m_TransitionState)
		{
			case State::kExit:
			{
				m_TransitionState = State::kPlaying;
				break;
			}
		}

		SetData();
	}

	bool CameraTransition::HasControl()
	{
		if (m_TransitionState == State::kExit)
			return false;

		return true;
	}

	CameraEvent CameraTransition::GetEventID()
	{
		return CameraEvent::kScripted;
	}

	void CameraTransition::SetData()
	{
		auto config = m_Config;
		m_Data.EventActive = &config->Events().bScripted;
		m_Data.FOV = &config->FOV().fScripted;
		m_Data.NearDistance = &config->NearDistance().fScripted;
	}

}

