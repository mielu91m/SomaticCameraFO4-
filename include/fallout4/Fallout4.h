/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "fallout4/Hooks.h"
#include "fallout4/ImprovedCameraFO4.h"

namespace Menu {

	class UIMenu;
	class MenuGeneral;
}

namespace Patch {

	class Fallout4 {

	public:
		Fallout4();
		~Fallout4() = default;

	public:
		const std::string& FullName() const { return m_FullName; };
		const std::string& Name() const { return m_Name; };
		const std::string& WindowName() const { return m_WindowName; };
		const std::string& Path() const { return m_Path; };

		std::uint32_t VersionMajor() const { return m_VersionMajor; };
		std::uint32_t VersionMinor() const { return m_VersionMinor; };
		std::uint32_t VersionRevision() const { return m_VersionRevision; };
		std::uint32_t VersionBuild() const { return m_VersionBuild; };
		std::uintptr_t BaseAddress() const { return m_BaseAddress; };
		std::uintptr_t ImageSize() const { return m_ImageSize; };

		Patch::Hooks* GetHooks() const { return m_Hooks.get(); };
		ImprovedCamera::ImprovedCameraFO4* Camera() const { return m_Camera.get(); };

	public:
		void OnLoad();
		void InstallInput();
		void LoadGame();
		void ExecutableInfo();

	private:
		std::string m_Name{};
		std::string m_FullName{};
		std::string m_WindowName{};
		std::string m_Path{};
		std::uint32_t m_VersionMajor = 0;
		std::uint32_t m_VersionMinor = 0;
		std::uint32_t m_VersionRevision = 0;
		std::uint32_t m_VersionBuild = 0;
		std::uintptr_t m_BaseAddress = 0;
		std::uintptr_t m_ImageSize = 0;

		std::unique_ptr<Hooks> m_Hooks = nullptr;
		std::unique_ptr<ImprovedCamera::ImprovedCameraFO4> m_Camera = nullptr;

		bool m_OnLoaded = false;
		bool m_InstalledHooks = false;

	};

}

