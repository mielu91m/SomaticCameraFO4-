/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "cameras/ThirdPerson.h"

#include "fallout4/Helper.h"
#include "systems/Config.h"
#include "utils/Log.h"

namespace ImprovedCamera {

	CameraThirdPerson::CameraThirdPerson() :
		ICamera("ThirdPerson", RE::CameraStates::k3rdPerson)
	{
		SetData();
	}

	void CameraThirdPerson::OnUpdate(std::uint8_t currentID, std::uint8_t previousID)
	{
		if (currentID != RE::CameraStates::k3rdPerson)
		{
			m_ThirdPersonState = State::kExit;
			return;
		}

		if (!m_Player)
			return;

		switch (m_ThirdPersonState)
		{
			case State::kExit:
			{
				if (Helper::IsWeaponDrawn(m_Player))
					m_ThirdPersonState = State::kWeaponDrawnEnter;
				else if (Helper::CannotMoveAndLook())
					m_ThirdPersonState = State::kCraftingEnter;
				else
					m_ThirdPersonState = State::kEnter;
				break;
			}
			case State::kEnter:
				m_ThirdPersonState = State::kIdle;
				break;
			case State::kWeaponDrawnEnter:
				m_ThirdPersonState = State::kWeaponDrawnIdle;
				break;
			case State::kCraftingEnter:
				m_ThirdPersonState = State::kCraftingIdle;
				break;
			case State::kScriptedEnter:
				m_ThirdPersonState = State::kScriptedIdle;
				break;
		}

		SetData();
	}

	bool CameraThirdPerson::HasControl()
	{
		if (m_ThirdPersonState == State::kExit)
			return false;

		auto data = GetData();
		if (data.EventActive && !*data.EventActive)
			return false;

		switch (m_ThirdPersonState)
		{
			case State::kIdle:
			case State::kWeaponDrawnIdle:
			{
				if (Helper::CannotMoveAndLook())
					m_ThirdPersonState = State::kScriptedEnter;
				else if (Helper::IsWeaponDrawn(m_Player) && m_ThirdPersonState != State::kWeaponDrawnIdle)
					m_ThirdPersonState = State::kWeaponDrawnEnter;
				else if (!Helper::IsWeaponDrawn(m_Player) && m_ThirdPersonState == State::kWeaponDrawnIdle)
					m_ThirdPersonState = State::kEnter;
				break;
			}
			case State::kCraftingIdle:
			{
				if (!Helper::CannotMoveAndLook())
					m_ThirdPersonState = State::kExit;
				break;
			}
			case State::kScriptedIdle:
			{
				if (!Helper::CannotMoveAndLook())
					m_ThirdPersonState = State::kExit;
				break;
			}
		}

		return m_ThirdPersonState != State::kExit;
	}

	CameraEvent CameraThirdPerson::GetEventID()
	{
		switch (m_ThirdPersonState)
		{
			case State::kCraftingIdle:
			case State::kCraftingEnter:
				return CameraEvent::kCrafting;
			case State::kScriptedIdle:
			case State::kScriptedEnter:
				return CameraEvent::kScripted;
			case State::kWeaponDrawnIdle:
			case State::kWeaponDrawnEnter:
			default:
				return CameraEvent::kThirdPerson;
		}
	}

	void CameraThirdPerson::SetData()
	{
		auto config = m_Config;

		switch (m_ThirdPersonState)
		{
			case State::kCraftingIdle:
			case State::kCraftingEnter:
			{
				m_Data.EventActive = &config->Events().bCrafting;
				m_Data.FOV = &config->FOV().fCrafting;
				m_Data.NearDistance = &config->NearDistance().fCrafting;
				break;
			}
			case State::kScriptedIdle:
			case State::kScriptedEnter:
			{
				m_Data.EventActive = &config->Events().bScripted;
				m_Data.FOV = &config->FOV().fScripted;
				m_Data.NearDistance = &config->NearDistance().fScripted;
				break;
			}
			default:
			{
				m_Data.EventActive = &config->Events().bThirdPerson;
				m_Data.FOV = &config->FOV().fThirdPerson;
				m_Data.NearDistance = &config->NearDistance().fThirdPerson;
				break;
			}
		}
	}

}

