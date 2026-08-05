/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

namespace Settings {

	struct ModuleData {
		std::string sFileName{};
		std::string sFileVersionMin{};
		std::string sFileVersionMax{};
		std::string sWindowName{};
		std::int32_t iCheckCompatibility = 1;
		std::int32_t iMenuMode = 0;
		std::int32_t iMenuTimeout = 30;
		std::int32_t iMenuKey = 0x2D;
		std::string sMenuFont{ "default" };
		float fMenuFontSize = 16.0f;
		std::string sProfileName{ "Default.ini" };
	};

	struct General {
		bool bEnableBody = 1;
		bool bEnableBodyConsole = 0;
		bool bEnableShadows = 0;
		bool bAdjustPlayerScale = 1;
		float fBodyHeightOffset = 0.0f;
		bool bEnableHead = 1;
		bool bEnableHeadCombat = 1;
		bool bEnableHeadMount = 1;
		bool bEnableHeadScripted = 1;
		bool bEnableThirdPersonArms = 0;
	};

	struct Hide {
		bool bWeapon = 0;
		bool bSitting = 0;
		bool bSleeping = 0;
		bool bJumping = 0;
		bool bSwimming = 0;
		bool bSneakRoll = 0;
		bool bAttack = 0;
		bool bPowerAttack = 0;
		bool bKillmove = 0;
	};

	struct Fixes {
		bool bFirstPersonOverhaul = 0;
	};

	struct RestrictAngles {
		float fSitting = 100.0f;
		float fSittingMaxLookingUp = 80.0f;
		float fSittingMaxLookingDown = 80.0f;
		float fScripted = 100.0f;
		float fScriptedPitch = 80.0f;
	};

	struct Events {
		bool bFirstPerson = 1;
		bool bFirstPersonCombat = 1;
		bool bFurniture = 1;
		bool bCrafting = 1;
		bool bRagdoll = 1;
		bool bDeath = 1;
		bool bMount = 1;
		bool bMountCombat = 1;
		bool bDialogue = 1;
		bool bScripted = 1;
		bool bThirdPerson = 1;
	};

	struct FOV {
		bool bEnableOverride = 0;
		float fFirstPerson = 80.0f;
		float fFirstPersonCombat = 80.0f;
		float fFurniture = 80.0f;
		float fCrafting = 80.0f;
		float fRagdoll = 80.0f;
		float fDeath = 80.0f;
		float fMount = 80.0f;
		float fMountCombat = 80.0f;
		float fDialogue = 80.0f;
		float fScripted = 80.0f;
		float fThirdPerson = 80.0f;
	};

	struct NearDistance {
		bool bEnableOverride = 0;
		float fFirstPersonDefault = 10.0f;
		float fPitchThreshold = 60.0f;
		float fFirstPerson = 10.0f;
		float fFirstPersonCombat = 10.0f;
		float fSitting = 10.0f;
		float fFurniture = 10.0f;
		float fCrafting = 10.0f;
		float fRagdoll = 10.0f;
		float fDeath = 10.0f;
		float fMount = 10.0f;
		float fMountCombat = 10.0f;
		float fDialogue = 10.0f;
		float fScripted = 10.0f;
		float fThirdPerson = 10.0f;
	};

	struct Headbob {
		bool bIdle = 0;
		bool bWalk = 1;
		bool bRun = 1;
		bool bSprint = 1;
		bool bCombat = 1;
		bool bSneak = 1;
		bool bSneakRoll = 0;
		float fRotationIdle = 0.0f;
		float fRotationWalk = 0.02f;
		float fRotationRun = 0.04f;
		float fRotationSprint = 0.06f;
		float fRotationCombat = 0.03f;
		float fRotationSneak = 0.01f;
		float fRotationSneakRoll = 0.04f;
	};

	struct Camera {
		float fFirstPersonPosX = 0.0f;
		float fFirstPersonPosY = 0.0f;
		float fFirstPersonPosZ = 0.0f;
		float fFirstPersonCombatPosX = 0.0f;
		float fFirstPersonCombatPosY = 0.0f;
		float fFirstPersonCombatPosZ = 0.0f;
		float fMountPosX = 0.0f;
		float fMountPosY = 0.0f;
		float fMountPosZ = 0.0f;
		float fMountCombatPosX = 0.0f;
		float fMountCombatPosY = 0.0f;
		float fMountCombatPosZ = 0.0f;
		float fScriptedPosX = 0.0f;
		float fScriptedPosY = 0.0f;
		float fScriptedPosZ = 0.0f;
	};

	struct PseudoFPP {
		float fHeightOffset = -5.0f;
		float fForwardOffset = 0.0f;
		std::int32_t iToggleKey = 115;
		std::int32_t iADSKey = 2;
		std::int32_t iGamepadTriggerThreshold = 30;
	};

	struct Logging {
		bool bAnimations = 0;
		bool bCameraDelta = 0;
		bool bMenus = 0;
	};

}

