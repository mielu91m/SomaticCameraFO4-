/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "cameras/Bleedout.h"

#include "fallout4/Helper.h"
#include "systems/Config.h"
#include "utils/Log.h"

namespace ImprovedCamera {

	CameraBleedout::CameraBleedout() :
		ICamera("Bleedout", RE::CameraStates::kBleedout)
	{
		SetData();
	}

	void CameraBleedout::OnUpdate(std::uint8_t currentID, std::uint8_t previousID)
	{
		if (currentID != RE::CameraStates::kBleedout)
		{
			m_BleedoutState = State::kExit;
			return;
		}

		switch (m_BleedoutState)
		{
			case State::kExit:
			{
				m_BleedoutState = State::kPlaying;
				break;
			}
		}

		SetData();
	}

	bool CameraBleedout::HasControl()
	{
		if (m_BleedoutState == State::kExit)
			return false;

		auto data = GetData();
		if (data.EventActive && !*data.EventActive)
			return false;

		return true;
	}

	CameraEvent CameraBleedout::GetEventID()
	{
		if (Helper::IsDead(m_Player))
			return CameraEvent::kDeath;
		return CameraEvent::kRagdoll;
	}

	void CameraBleedout::SetData()
	{
		auto config = m_Config;
		m_Data.EventActive = &config->Events().bRagdoll;
		m_Data.FOV = &config->FOV().fRagdoll;
		m_Data.NearDistance = &config->NearDistance().fRagdoll;
	}

}

