/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

namespace Menu {
	class UIMenu;
	class MenuGeneral;
}

namespace Patch {

		class Hooks {

	public:
		Hooks() = default;
		~Hooks();

		static void InvalidatePseudoCameraCache();

	private:
		void Install();
		void Input();
		void Setup();
		void HookUpdateThirdPerson();
		void HookNiCameraUpdateWorldData();
		void HookPlayerCameraUpdate();
		void HookThirdPersonStateUpdate();
		void InstallSceneGraphHooks();

		static void Hook_UpdateThirdPerson(RE::PlayerCamera* camera, bool weaponDrawn);
		static void Hook_NiCamera_UpdateWorldData(RE::NiCamera* a_this, RE::NiUpdateData* a_data);
		static void Hook_PlayerCameraUpdate(RE::PlayerCamera* a_this);
		static void Hook_ThirdPersonStateUpdate(RE::ThirdPersonState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState);

	private:
		friend class Fallout4;
		friend class Menu::UIMenu;
		friend class Menu::MenuGeneral;
	};

}
