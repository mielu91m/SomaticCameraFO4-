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
		static bool IsVATSActive();

	private:
		void Install();
		void Input();
		void Setup();
void HookNiCameraUpdateWorldData();
		void HookPlayerCameraUpdate();
		void HookThirdPersonStateUpdate();
		void HookPipboyMode();
		void HookFurnitureMode();
		void HookCameraStateSet();
		void InstallSceneGraphHooks();

		static void Hook_NiCamera_UpdateWorldData(RE::NiCamera* a_this, RE::NiUpdateData* a_data);
		static void Hook_PlayerCameraUpdate(RE::PlayerCamera* a_this);
		static void Hook_ThirdPersonStateUpdate(RE::ThirdPersonState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState);
		static void Hook_StartPipboyMode(RE::PlayerCamera* a_this, bool a_forcePipboyModeCamera);
		static void Hook_StopPipboyMode(RE::PlayerCamera* a_this);
		static void Hook_StartFurnitureMode(RE::PlayerCamera* a_this, RE::TESObjectREFR* a_furniture);
		static RE::TESCameraState* Hook_PlayerCameraSetState(RE::PlayerCamera* a_this, RE::TESCameraState* a_newstate);

	private:
		friend class Fallout4;
		friend class Menu::UIMenu;
		friend class Menu::MenuGeneral;
	};

}
