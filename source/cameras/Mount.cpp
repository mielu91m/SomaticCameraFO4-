/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "cameras/Mount.h"

#include "fallout4/Helper.h"
#include "systems/Config.h"
#include "utils/Log.h"

namespace ImprovedCamera {

	CameraMount::CameraMount() :
		ICamera("Mount", RE::CameraStates::kMount)
	{
		SetData();
	}

	void CameraMount::OnUpdate(std::uint8_t currentID, std::uint8_t previousID)
	{
		if (currentID != RE::CameraStates::kMount)
		{
			m_MountState = State::kExit;
			return;
		}

		if (!m_Player)
			return;

		switch (m_MountState)
		{
			case State::kExit:
			{
				m_MountState = State::kMounting;
				break;
			}
			case State::kMounting:
			{
				m_MountState = State::kMounted;
				break;
			}
			case State::kMounted:
			{
				if (Helper::IsWeaponDrawn(m_Player))
					m_MountState = State::kRidingWeaponDrawn;
				else
					m_MountState = State::kRiding;
				break;
			}
		}

		SetData();
	}

	bool CameraMount::HasControl()
	{
		if (m_MountState == State::kExit)
			return false;

		auto data = GetData();
		if (data.EventActive && !*data.EventActive)
			return false;

		switch (m_MountState)
		{
			case State::kRiding:
			case State::kRidingWeaponDrawn:
			{
				if (Helper::IsWeaponDrawn(m_Player) && m_MountState != State::kRidingWeaponDrawn)
					m_MountState = State::kRidingWeaponDrawn;
				else if (!Helper::IsWeaponDrawn(m_Player) && m_MountState == State::kRidingWeaponDrawn)
					m_MountState = State::kRiding;
				break;
			}
			case State::kDismounting:
			{
				m_MountState = State::kExit;
				break;
			}
		}

		return m_MountState != State::kExit;
	}

	CameraEvent CameraMount::GetEventID()
	{
		switch (m_MountState)
		{
			case State::kRidingWeaponDrawn:
				return CameraEvent::kMountCombat;
			case State::kRiding:
			case State::kMounting:
			case State::kMounted:
			default:
				return CameraEvent::kMount;
		}
	}

	void CameraMount::SetData()
	{
		auto config = m_Config;

		switch (m_MountState)
		{
			case State::kRidingWeaponDrawn:
			{
				m_Data.EventActive = &config->Events().bMountCombat;
				m_Data.FOV = &config->FOV().fMountCombat;
				m_Data.NearDistance = &config->NearDistance().fMountCombat;
				break;
			}
			default:
			{
				m_Data.EventActive = &config->Events().bMount;
				m_Data.FOV = &config->FOV().fMount;
				m_Data.NearDistance = &config->NearDistance().fMount;
				break;
			}
		}
	}

}

