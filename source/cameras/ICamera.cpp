/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "cameras/ICamera.h"

#include "plugin.h"

namespace Interface {

	ICamera::ICamera(const std::string& a_name, std::uint32_t a_id) :
		m_Name(a_name),
		m_ID(a_id)
	{
		m_Player = RE::PlayerCharacter::GetSingleton();
		m_Config = DLLMain::Plugin::Get()->Config();
	}

	Camera::~Camera()
	{
		for (auto& camera : m_Cameras)
		{
			if (camera)
			{
				camera->OnShutdown();
				delete camera;
			}
		}
		m_Cameras.clear();
	}

	void Camera::Register(ICamera* a_camera)
	{
		if (!a_camera)
			return;

		auto it = m_Cameras.begin();
		std::advance(it, m_CameraInsertIndex);
		m_Cameras.insert(it, a_camera);
		m_CameraInsertIndex++;
	}

	void Camera::Unregister(ICamera* a_camera)
	{
		if (!a_camera)
			return;

		auto end = m_Cameras.begin();
		std::advance(end, m_CameraInsertIndex);

		auto it = std::find(m_Cameras.begin(), end, a_camera);
		if (it != end)
		{
			(*it)->OnShutdown();
			m_Cameras.erase(it);
			m_CameraInsertIndex--;
		}
	}

}

