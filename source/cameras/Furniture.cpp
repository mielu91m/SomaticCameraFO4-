/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "cameras/Furniture.h"

#include "systems/Config.h"
#include "utils/Log.h"

namespace ImprovedCamera {

	CameraFurniture::CameraFurniture() :
		ICamera("Furniture", RE::CameraStates::kFurniture)
	{
		SetData();
	}

	void CameraFurniture::OnUpdate(std::uint8_t currentID, std::uint8_t previousID)
	{
		if (currentID != RE::CameraStates::kFurniture)
		{
			m_FurnitureState = State::kExit;
			return;
		}

		switch (m_FurnitureState)
		{
			case State::kExit:
			{
				m_FurnitureState = State::kEnter;
				break;
			}
			case State::kEnter:
			{
				m_FurnitureState = State::kIdle;
				break;
			}
		}

		SetData();
	}

	bool CameraFurniture::HasControl()
	{
		if (m_FurnitureState == State::kExit)
			return false;

		auto data = GetData();
		if (data.EventActive && !*data.EventActive)
			return false;

		return true;
	}

	CameraEvent CameraFurniture::GetEventID()
	{
		return CameraEvent::kFurniture;
	}

	void CameraFurniture::SetData()
	{
		auto config = m_Config;
		m_Data.EventActive = &config->Events().bFurniture;
		m_Data.FOV = &config->FOV().fFurniture;
		m_Data.NearDistance = &config->NearDistance().fFurniture;
	}

}

