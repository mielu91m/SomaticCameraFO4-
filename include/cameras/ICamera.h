/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "cameras/Events.h"

namespace Systems {

	class Config;
}

namespace Interface {

	class ICamera {

	public:
		struct Data {
			const bool* EventActive;
			const float* FOV;
			const float* NearDistance;
		};

		ICamera(const std::string& a_name, std::uint32_t a_id);
		virtual ~ICamera() = default;

		virtual void OnUpdate(std::uint8_t currentID, std::uint8_t previousID) = 0;
		virtual bool HasControl() = 0;
		virtual CameraEvent GetEventID() = 0;
		virtual std::uint8_t GetStateID() = 0;
		virtual Data GetData() = 0;
		virtual void OnShutdown() { return; };

		const std::string& Name() const { return m_Name; };
		std::uint32_t ID() const { return m_ID; };

	protected:
		std::string m_Name{};
		std::uint32_t m_ID = 0;

		RE::PlayerCharacter* m_Player = nullptr;
		Systems::Config* m_Config = nullptr;
	};

	class Camera {

	public:
		Camera() = default;
		~Camera();

		Camera(const Camera&) = delete;
		Camera& operator=(const Camera&) = delete;

		void Register(ICamera* a_camera);
		void Unregister(ICamera* a_camera);

		auto begin() noexcept { return m_Cameras.begin(); }
		auto end() noexcept { return m_Cameras.end(); }
		auto rbegin() noexcept { return m_Cameras.rbegin(); }
		auto rend() noexcept { return m_Cameras.rend(); }

	private:
		std::vector<ICamera*> m_Cameras{};
		std::size_t m_CameraInsertIndex = 0;
	};

}

