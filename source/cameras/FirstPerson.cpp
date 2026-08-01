/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "cameras/FirstPerson.h"

#include "fallout4/Helper.h"
#include "systems/Config.h"
#include "utils/Log.h"

namespace ImprovedCamera {

	CameraFirstPerson::CameraFirstPerson() :
		ICamera("FirstPerson", RE::CameraStates::kFirstPerson)
	{
		SetData();
	}

	void CameraFirstPerson::OnUpdate(std::uint8_t currentID, std::uint8_t previousID)
	{
		if (currentID != RE::CameraStates::kFirstPerson)
		{
			m_FirstPersonState = State::kExit;
			return;
		}

		if (!m_Player)
			return;

		switch (m_FirstPersonState)
		{
			case State::kExit:
			{
				if (Helper::IsSitting(m_Player))
					m_FirstPersonState = State::kSittingEnter;
				else if (Helper::IsWeaponDrawn(m_Player))
					m_FirstPersonState = State::kWeaponDrawnEnter;
				else if (Helper::CannotMoveAndLook())
					m_FirstPersonState = State::kScriptedEnter;
				else
					m_FirstPersonState = State::kEnter;
				break;
			}
			case State::kEnter:
				m_FirstPersonState = State::kIdle;
				break;
			case State::kWeaponDrawnEnter:
				m_FirstPersonState = State::kWeaponDrawnIdle;
				break;
			case State::kSittingEnter:
				m_FirstPersonState = State::kSittingIdle;
				break;
			case State::kScriptedEnter:
				m_FirstPersonState = State::kScriptedIdle;
				break;
		}

		SetData();
	}

	bool CameraFirstPerson::HasControl()
	{
		if (m_FirstPersonState == State::kExit)
			return false;

		auto data = GetData();
		if (data.EventActive && !*data.EventActive)
			return false;

		switch (m_FirstPersonState)
		{
			case State::kIdle:
			case State::kWeaponDrawnIdle:
			{
				if (Helper::IsSitting(m_Player))
					m_FirstPersonState = State::kSittingEnter;
				else if (Helper::CannotMoveAndLook())
					m_FirstPersonState = State::kScriptedEnter;
				else if (Helper::IsWeaponDrawn(m_Player) && m_FirstPersonState != State::kWeaponDrawnIdle)
					m_FirstPersonState = State::kWeaponDrawnEnter;
				else if (!Helper::IsWeaponDrawn(m_Player) && m_FirstPersonState == State::kWeaponDrawnIdle)
					m_FirstPersonState = State::kEnter;
				break;
			}
			case State::kSittingIdle:
			{
				if (!Helper::IsSitting(m_Player))
					m_FirstPersonState = State::kExit;
				break;
			}
			case State::kScriptedIdle:
			{
				if (!Helper::CannotMoveAndLook())
					m_FirstPersonState = State::kExit;
				break;
			}
		}

		return m_FirstPersonState != State::kExit;
	}

	CameraEvent CameraFirstPerson::GetEventID()
	{
		switch (m_FirstPersonState)
		{
			case State::kWeaponDrawnIdle:
			case State::kWeaponDrawnEnter:
				return CameraEvent::kFirstPersonCombat;
			default:
				return CameraEvent::kFirstPerson;
		}
	}

	void CameraFirstPerson::SetData()
	{
		auto config = m_Config;

		switch (m_FirstPersonState)
		{
			case State::kWeaponDrawnIdle:
			case State::kWeaponDrawnEnter:
			{
				m_Data.EventActive = &config->Events().bFirstPersonCombat;
				m_Data.FOV = &config->FOV().fFirstPersonCombat;
				m_Data.NearDistance = &config->NearDistance().fFirstPersonCombat;
				break;
			}
			default:
			{
				m_Data.EventActive = &config->Events().bFirstPerson;
				m_Data.FOV = &config->FOV().fFirstPerson;
				m_Data.NearDistance = &config->NearDistance().fFirstPerson;
				break;
			}
		}
	}

}

