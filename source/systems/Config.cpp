/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PCH.h"

#include "systems/Config.h"

#include "plugin.h"
#include "utils/Utils.h"

#define MINI_CASE_SENSITIVE
#include "mini/ini.h"

namespace Systems {

	Config::Config()
	{
		auto plugin = DLLMain::Plugin::Get();

		m_Name = plugin->Name() + ".ini";
		m_Path = plugin->Path() + plugin->Name() + "\\";

		m_FontPath = m_Path + "Fonts\\";
		m_ProfilePath = m_Path + "Profiles\\";
		m_FileName = plugin->Path() + m_Name;

		if (!ReadIni(m_FileName))
		{
			m_ModuleData.sFileName = "Fallout4.exe";
			m_ModuleData.sFileVersionMin = "1.0.0.0";
			m_ModuleData.sFileVersionMax = "1.11.999.0";
			m_ModuleData.sWindowName = "Fallout4";
			m_ModuleData.iCheckCompatibility = 1;
			m_ModuleData.iMenuMode = 0;
			m_ModuleData.iMenuTimeout = 30;
			m_ModuleData.iMenuKey = 0x2D;
			m_ModuleData.sMenuFont = "default";
			m_ModuleData.fMenuFontSize = 16.0f;
			m_ModuleData.sProfileName = "Default.ini";
			m_PreInitialized = true;
		}

		// legacy profile override: if a Profiles\*.ini still exists (from an
		// older install), its settings win. Otherwise everything is read from
		// the single ImprovedCameraFO4.ini next to the DLL.
		std::string profileName = m_ProfilePath + m_ModuleData.sProfileName;
		if (GetFileAttributesA(profileName.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			if (!ReadIni(profileName))
				LOG_WARN("Profile read failed: {}", profileName.c_str());
		}

		// F4MCM integration: if F4MCM is installed, read settings from
		// Data\MCM\Settings\SomaticCameraFO4.ini (overrides INI file).
		// Try absolute path first (derived from plugin path), then fallback to relative.
		std::string pluginPath = plugin->Path();
		std::string dataPath = pluginPath;
		size_t f4sePos = dataPath.rfind("F4SE\\Plugins\\");
		if (f4sePos != std::string::npos)
			dataPath = dataPath.substr(0, f4sePos);
		std::string mcmSettings = dataPath + "MCM\\Settings\\SomaticCameraFO4.ini";
		m_MCMSettingsPath = mcmSettings;
		LOG_INFO("Config: checking MCM settings at '{}'", mcmSettings);
		if (GetFileAttributesA(mcmSettings.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			if (!ReadIni(mcmSettings))
				LOG_WARN("MCM settings read failed: {}", mcmSettings.c_str());
			else
				LOG_INFO("Config: using MCM settings from '{}'", mcmSettings.c_str());
		}
		else {
			// Fallback: try relative path
			std::string mcmSettingsRel = "Data\\MCM\\Settings\\SomaticCameraFO4.ini";
			LOG_INFO("Config: MCM settings not found at '{}', trying relative path '{}'", mcmSettings, mcmSettingsRel);
			if (GetFileAttributesA(mcmSettingsRel.c_str()) != INVALID_FILE_ATTRIBUTES)
			{
				m_MCMSettingsPath = mcmSettingsRel;
				if (!ReadIni(mcmSettingsRel))
					LOG_WARN("MCM settings read failed (relative): {}", mcmSettingsRel.c_str());
				else
					LOG_INFO("Config: using MCM settings from '{}' (relative)", mcmSettingsRel.c_str());
			}
		}

		UpdateMCMSettingsWriteTime();
		m_MCMKeybindsPath = dataPath + "MCM\\Settings\\Keybinds.json";
		if (GetFileAttributesA(m_MCMKeybindsPath.c_str()) == INVALID_FILE_ATTRIBUTES)
		{
			// Fallback: try relative path
			std::string mcmKeybindsRel = "Data\\MCM\\Settings\\Keybinds.json";
			if (GetFileAttributesA(mcmKeybindsRel.c_str()) != INVALID_FILE_ATTRIBUTES)
			{
				m_MCMKeybindsPath = mcmKeybindsRel;
				LOG_INFO("Config: using MCM keybinds from relative path '{}'", mcmKeybindsRel);
			}
		}
		UpdateMCMKeybindsWriteTime();
		ReadMCMToggleKey();
		if (m_MCMToggleKey > 0)
			LOG_INFO("Config: MCM toggle key from Keybinds.json = {} (overrides INI iToggleKey)", m_MCMToggleKey);
		else
			LOG_INFO("Config: no MCM toggle key found, falling back to INI iToggleKey = {}", m_PseudoFPP.iToggleKey);

		LOG_INFO("Config: using '{}' (PSEUDOFPP height={:.4f} forward={:.4f})",
			m_FileName.c_str(), m_PseudoFPP.fHeightOffset, m_PseudoFPP.fForwardOffset);
	}

	bool Config::ReadIni(std::string& name)
	{
		try
		{
			mINI::INIFile file(name.c_str());
			mINI::INIStructure ini;
			file.read(ini);

			if (!m_PreInitialized)
			{
				m_ModuleData.sFileName = ini.get("MODULE DATA").get("FileName");
				m_ModuleData.sFileVersionMin = ini.get("MODULE DATA").get("FileVersionMin");
				m_ModuleData.sFileVersionMax = ini.get("MODULE DATA").get("FileVersionMax");
				m_ModuleData.sWindowName = ini.get("MODULE DATA").get("WindowName");
				m_ModuleData.iCheckCompatibility = std::stoi(ini.get("MODULE DATA").get("CheckCompatibility"));
				m_ModuleData.iMenuMode = std::stoi(ini.get("MODULE DATA").get("MenuMode"));
				m_ModuleData.iMenuTimeout = std::stoi(ini.get("MODULE DATA").get("MenuTimeout"));
				m_ModuleData.iMenuKey = std::stoi(ini.get("MODULE DATA").get("MenuKey"), NULL, 16);
				m_ModuleData.sMenuFont = ini.get("MODULE DATA").get("MenuFont");
				m_ModuleData.fMenuFontSize = std::stof(ini.get("MODULE DATA").get("MenuFontSize"));
				m_ModuleData.sProfileName = ini.get("MODULE DATA").get("ProfileName");

				m_PreInitialized = true;
			}

			auto getBool = [&ini](const std::string& sec, const std::string& key, bool def) {
				auto v = ini.get(sec).get(key);
				return v.empty() ? def : std::stoi(v) != 0;
			};
			auto getInt = [&ini](const std::string& sec, const std::string& key, int def) {
				auto v = ini.get(sec).get(key);
				return v.empty() ? def : std::stoi(v);
			};
			auto getFloat = [&ini](const std::string& sec, const std::string& key, float def) {
				auto v = ini.get(sec).get(key);
				return v.empty() ? def : std::stof(v);
			};

			m_General.bEnableBody = getBool("GENERAL", "bEnableBody", m_General.bEnableBody);
			m_General.bEnableBodyConsole = getBool("GENERAL", "bEnableBodyConsole", m_General.bEnableBodyConsole);
			m_General.bEnableShadows = getBool("GENERAL", "bEnableShadows", m_General.bEnableShadows);
			m_General.bAdjustPlayerScale = getBool("GENERAL", "bAdjustPlayerScale", m_General.bAdjustPlayerScale);
			m_General.fBodyHeightOffset = getFloat("GENERAL", "fBodyHeightOffset", m_General.fBodyHeightOffset);
			m_General.bEnableHead = getBool("GENERAL", "bEnableHead", m_General.bEnableHead);
			m_General.bEnableHeadCombat = getBool("GENERAL", "bEnableHeadCombat", m_General.bEnableHeadCombat);
			m_General.bEnableHeadMount = getBool("GENERAL", "bEnableHeadMount", m_General.bEnableHeadMount);
			m_General.bEnableHeadScripted = getBool("GENERAL", "bEnableHeadScripted", m_General.bEnableHeadScripted);
			m_General.bEnableThirdPersonArms = getBool("GENERAL", "bEnableThirdPersonArms", m_General.bEnableThirdPersonArms);

			m_Hide.bWeapon = getBool("HIDE", "bWeapon", m_Hide.bWeapon);
			m_Hide.bSitting = getBool("HIDE", "bSitting", m_Hide.bSitting);
			m_Hide.bSleeping = getBool("HIDE", "bSleeping", m_Hide.bSleeping);
			m_Hide.bJumping = getBool("HIDE", "bJumping", m_Hide.bJumping);
			m_Hide.bSwimming = getBool("HIDE", "bSwimming", m_Hide.bSwimming);
			m_Hide.bSneakRoll = getBool("HIDE", "bSneakRoll", m_Hide.bSneakRoll);
			m_Hide.bAttack = getBool("HIDE", "bAttack", m_Hide.bAttack);
			m_Hide.bPowerAttack = getBool("HIDE", "bPowerAttack", m_Hide.bPowerAttack);
			m_Hide.bKillmove = getBool("HIDE", "bKillmove", m_Hide.bKillmove);

			m_Fixes.bFirstPersonOverhaul = getBool("FIXES", "bFirstPersonOverhaul", m_Fixes.bFirstPersonOverhaul);

			m_RestrictAngles.fSitting = getFloat("RESTRICT ANGLES", "fSitting", m_RestrictAngles.fSitting);
			m_RestrictAngles.fSittingMaxLookingUp = getFloat("RESTRICT ANGLES", "fSittingMaxLookingUp", m_RestrictAngles.fSittingMaxLookingUp);
			m_RestrictAngles.fSittingMaxLookingDown = getFloat("RESTRICT ANGLES", "fSittingMaxLookingDown", m_RestrictAngles.fSittingMaxLookingDown);
			m_RestrictAngles.fScripted = getFloat("RESTRICT ANGLES", "fScripted", m_RestrictAngles.fScripted);
			m_RestrictAngles.fScriptedPitch = getFloat("RESTRICT ANGLES", "fScriptedPitch", m_RestrictAngles.fScriptedPitch);

			m_Events.bFirstPerson = getBool("EVENTS", "bFirstPerson", m_Events.bFirstPerson);
			m_Events.bFirstPersonCombat = getBool("EVENTS", "bFirstPersonCombat", m_Events.bFirstPersonCombat);
			m_Events.bFurniture = getBool("EVENTS", "bFurniture", m_Events.bFurniture);
			m_Events.bCrafting = getBool("EVENTS", "bCrafting", m_Events.bCrafting);
			m_Events.bRagdoll = getBool("EVENTS", "bRagdoll", m_Events.bRagdoll);
			m_Events.bDeath = getBool("EVENTS", "bDeath", m_Events.bDeath);
			m_Events.bMount = getBool("EVENTS", "bMount", m_Events.bMount);
			m_Events.bMountCombat = getBool("EVENTS", "bMountCombat", m_Events.bMountCombat);
			m_Events.bDialogue = getBool("EVENTS", "bDialogue", m_Events.bDialogue);
			m_Events.bScripted = getBool("EVENTS", "bScripted", m_Events.bScripted);
			m_Events.bThirdPerson = getBool("EVENTS", "bThirdPerson", m_Events.bThirdPerson);

			m_FOV.bEnableOverride = getBool("FOV", "bEnableOverride", m_FOV.bEnableOverride);
			m_FOV.fFirstPerson = getFloat("FOV", "fFirstPerson", m_FOV.fFirstPerson);
			m_FOV.fFirstPersonCombat = getFloat("FOV", "fFirstPersonCombat", m_FOV.fFirstPersonCombat);
			m_FOV.fFurniture = getFloat("FOV", "fFurniture", m_FOV.fFurniture);
			m_FOV.fCrafting = getFloat("FOV", "fCrafting", m_FOV.fCrafting);
			m_FOV.fRagdoll = getFloat("FOV", "fRagdoll", m_FOV.fRagdoll);
			m_FOV.fDeath = getFloat("FOV", "fDeath", m_FOV.fDeath);
			m_FOV.fMount = getFloat("FOV", "fMount", m_FOV.fMount);
			m_FOV.fMountCombat = getFloat("FOV", "fMountCombat", m_FOV.fMountCombat);
			m_FOV.fDialogue = getFloat("FOV", "fDialogue", m_FOV.fDialogue);
			m_FOV.fScripted = getFloat("FOV", "fScripted", m_FOV.fScripted);
			m_FOV.fThirdPerson = getFloat("FOV", "fThirdPerson", m_FOV.fThirdPerson);

			m_NearDistance.bEnableOverride = getBool("NEARDISTANCE", "bEnableOverride", m_NearDistance.bEnableOverride);
			m_NearDistance.fFirstPersonDefault = getFloat("NEARDISTANCE", "fFirstPersonDefault", m_NearDistance.fFirstPersonDefault);
			m_NearDistance.fPitchThreshold = getFloat("NEARDISTANCE", "fPitchThreshold", m_NearDistance.fPitchThreshold);
			m_NearDistance.fFirstPerson = getFloat("NEARDISTANCE", "fFirstPerson", m_NearDistance.fFirstPerson);
			m_NearDistance.fFirstPersonCombat = getFloat("NEARDISTANCE", "fFirstPersonCombat", m_NearDistance.fFirstPersonCombat);
			m_NearDistance.fSitting = getFloat("NEARDISTANCE", "fSitting", m_NearDistance.fSitting);
			m_NearDistance.fFurniture = getFloat("NEARDISTANCE", "fFurniture", m_NearDistance.fFurniture);
			m_NearDistance.fCrafting = getFloat("NEARDISTANCE", "fCrafting", m_NearDistance.fCrafting);
			m_NearDistance.fRagdoll = getFloat("NEARDISTANCE", "fRagdoll", m_NearDistance.fRagdoll);
			m_NearDistance.fDeath = getFloat("NEARDISTANCE", "fDeath", m_NearDistance.fDeath);
			m_NearDistance.fMount = getFloat("NEARDISTANCE", "fMount", m_NearDistance.fMount);
			m_NearDistance.fMountCombat = getFloat("NEARDISTANCE", "fMountCombat", m_NearDistance.fMountCombat);
			m_NearDistance.fDialogue = getFloat("NEARDISTANCE", "fDialogue", m_NearDistance.fDialogue);
			m_NearDistance.fScripted = getFloat("NEARDISTANCE", "fScripted", m_NearDistance.fScripted);
			m_NearDistance.fThirdPerson = getFloat("NEARDISTANCE", "fThirdPerson", m_NearDistance.fThirdPerson);

			m_Headbob.bIdle = getBool("HEADBOB", "bIdle", m_Headbob.bIdle);
			m_Headbob.bWalk = getBool("HEADBOB", "bWalk", m_Headbob.bWalk);
			m_Headbob.bRun = getBool("HEADBOB", "bRun", m_Headbob.bRun);
			m_Headbob.bSprint = getBool("HEADBOB", "bSprint", m_Headbob.bSprint);
			m_Headbob.bCombat = getBool("HEADBOB", "bCombat", m_Headbob.bCombat);
			m_Headbob.bSneak = getBool("HEADBOB", "bSneak", m_Headbob.bSneak);
			m_Headbob.bSneakRoll = getBool("HEADBOB", "bSneakRoll", m_Headbob.bSneakRoll);
			m_Headbob.fRotationIdle = getFloat("HEADBOB", "fRotationIdle", m_Headbob.fRotationIdle);
			m_Headbob.fRotationWalk = getFloat("HEADBOB", "fRotationWalk", m_Headbob.fRotationWalk);
			m_Headbob.fRotationRun = getFloat("HEADBOB", "fRotationRun", m_Headbob.fRotationRun);
			m_Headbob.fRotationSprint = getFloat("HEADBOB", "fRotationSprint", m_Headbob.fRotationSprint);
			m_Headbob.fRotationCombat = getFloat("HEADBOB", "fRotationCombat", m_Headbob.fRotationCombat);
			m_Headbob.fRotationSneak = getFloat("HEADBOB", "fRotationSneak", m_Headbob.fRotationSneak);
			m_Headbob.fRotationSneakRoll = getFloat("HEADBOB", "fRotationSneakRoll", m_Headbob.fRotationSneakRoll);

			m_Camera.fFirstPersonPosX = getFloat("CAMERA", "fFirstPersonPosX", m_Camera.fFirstPersonPosX);
			m_Camera.fFirstPersonPosY = getFloat("CAMERA", "fFirstPersonPosY", m_Camera.fFirstPersonPosY);
			m_Camera.fFirstPersonPosZ = getFloat("CAMERA", "fFirstPersonPosZ", m_Camera.fFirstPersonPosZ);
			m_Camera.fFirstPersonCombatPosX = getFloat("CAMERA", "fFirstPersonCombatPosX", m_Camera.fFirstPersonCombatPosX);
			m_Camera.fFirstPersonCombatPosY = getFloat("CAMERA", "fFirstPersonCombatPosY", m_Camera.fFirstPersonCombatPosY);
			m_Camera.fFirstPersonCombatPosZ = getFloat("CAMERA", "fFirstPersonCombatPosZ", m_Camera.fFirstPersonCombatPosZ);
			m_Camera.fMountPosX = getFloat("CAMERA", "fMountPosX", m_Camera.fMountPosX);
			m_Camera.fMountPosY = getFloat("CAMERA", "fMountPosY", m_Camera.fMountPosY);
			m_Camera.fMountPosZ = getFloat("CAMERA", "fMountPosZ", m_Camera.fMountPosZ);
			m_Camera.fMountCombatPosX = getFloat("CAMERA", "fMountCombatPosX", m_Camera.fMountCombatPosX);
			m_Camera.fMountCombatPosY = getFloat("CAMERA", "fMountCombatPosY", m_Camera.fMountCombatPosY);
			m_Camera.fMountCombatPosZ = getFloat("CAMERA", "fMountCombatPosZ", m_Camera.fMountCombatPosZ);
			m_Camera.fScriptedPosX = getFloat("CAMERA", "fScriptedPosX", m_Camera.fScriptedPosX);
			m_Camera.fScriptedPosY = getFloat("CAMERA", "fScriptedPosY", m_Camera.fScriptedPosY);
			m_Camera.fScriptedPosZ = getFloat("CAMERA", "fScriptedPosZ", m_Camera.fScriptedPosZ);

			m_PseudoFPP.fHeightOffset = getFloat("PSEUDOFPP", "fHeightOffset", m_PseudoFPP.fHeightOffset);
			m_PseudoFPP.fForwardOffset = getFloat("PSEUDOFPP", "fForwardOffset", m_PseudoFPP.fForwardOffset);
			m_PseudoFPP.iToggleKey = getInt("PSEUDOFPP", "iToggleKey", m_PseudoFPP.iToggleKey);
			m_PseudoFPP.iADSKey = getInt("PSEUDOFPP", "iADSKey", m_PseudoFPP.iADSKey);
			m_PseudoFPP.iGamepadTriggerThreshold = getInt("PSEUDOFPP", "iGamepadTriggerThreshold", m_PseudoFPP.iGamepadTriggerThreshold);
		}
		catch (...)
		{
			auto plugin = DLLMain::Plugin::Get();
			std::string errorInfo{};

			if (m_PreInitialized && !m_Initialized)
			{
				m_PreInitialized = false;
				errorInfo = "Invalid Profile: " + m_ModuleData.sProfileName;
				MessageBox(NULL, errorInfo.c_str(), plugin->Description().c_str(), MB_ICONERROR);
			}
			else if (!m_PreInitialized)
			{
				errorInfo = "Invalid: " + m_Name;
				MessageBox(NULL, errorInfo.c_str(), plugin->Description().c_str(), MB_ICONERROR);
			}
			return false;
		}
		return true;
	}

	void Config::WriteIni(std::string& name, bool updateMain)
	{
		if (updateMain)
		{
			mINI::INIFile file(m_FileName.c_str());
			mINI::INIStructure ini;
			file.read(ini);
			ini["MODULE DATA"]["ProfileName"] = m_ModuleData.sProfileName;
			file.write(ini);
		}
		else
		{
			std::string fileName = m_ProfilePath + name.c_str();

			mINI::INIFile file(fileName.c_str());
			mINI::INIStructure ini;
			file.read(ini);

			ini["GENERAL"]["bEnableBody"] = std::to_string(m_General.bEnableBody);
			ini["GENERAL"]["bEnableBodyConsole"] = std::to_string(m_General.bEnableBodyConsole);
			ini["GENERAL"]["bEnableShadows"] = std::to_string(m_General.bEnableShadows);
			ini["GENERAL"]["bAdjustPlayerScale"] = std::to_string(m_General.bAdjustPlayerScale);
			ini["GENERAL"]["fBodyHeightOffset"] = std::to_string(m_General.fBodyHeightOffset);
			ini["GENERAL"]["bEnableHead"] = std::to_string(m_General.bEnableHead);
			ini["GENERAL"]["bEnableHeadCombat"] = std::to_string(m_General.bEnableHeadCombat);
			ini["GENERAL"]["bEnableHeadMount"] = std::to_string(m_General.bEnableHeadMount);
			ini["GENERAL"]["bEnableHeadScripted"] = std::to_string(m_General.bEnableHeadScripted);
			ini["GENERAL"]["bEnableThirdPersonArms"] = std::to_string(m_General.bEnableThirdPersonArms);

			ini["HIDE"]["bWeapon"] = std::to_string(m_Hide.bWeapon);
			ini["HIDE"]["bSitting"] = std::to_string(m_Hide.bSitting);
			ini["HIDE"]["bSleeping"] = std::to_string(m_Hide.bSleeping);
			ini["HIDE"]["bJumping"] = std::to_string(m_Hide.bJumping);
			ini["HIDE"]["bSwimming"] = std::to_string(m_Hide.bSwimming);
			ini["HIDE"]["bSneakRoll"] = std::to_string(m_Hide.bSneakRoll);
			ini["HIDE"]["bAttack"] = std::to_string(m_Hide.bAttack);
			ini["HIDE"]["bPowerAttack"] = std::to_string(m_Hide.bPowerAttack);
			ini["HIDE"]["bKillmove"] = std::to_string(m_Hide.bKillmove);

			ini["FIXES"]["bFirstPersonOverhaul"] = std::to_string(m_Fixes.bFirstPersonOverhaul);

			ini["RESTRICT ANGLES"]["fSitting"] = std::to_string(m_RestrictAngles.fSitting);
			ini["RESTRICT ANGLES"]["fSittingMaxLookingUp"] = std::to_string(m_RestrictAngles.fSittingMaxLookingUp);
			ini["RESTRICT ANGLES"]["fSittingMaxLookingDown"] = std::to_string(m_RestrictAngles.fSittingMaxLookingDown);
			ini["RESTRICT ANGLES"]["fScripted"] = std::to_string(m_RestrictAngles.fScripted);
			ini["RESTRICT ANGLES"]["fScriptedPitch"] = std::to_string(m_RestrictAngles.fScriptedPitch);

			ini["EVENTS"]["bFirstPerson"] = std::to_string(m_Events.bFirstPerson);
			ini["EVENTS"]["bFirstPersonCombat"] = std::to_string(m_Events.bFirstPersonCombat);
			ini["EVENTS"]["bFurniture"] = std::to_string(m_Events.bFurniture);
			ini["EVENTS"]["bCrafting"] = std::to_string(m_Events.bCrafting);
			ini["EVENTS"]["bRagdoll"] = std::to_string(m_Events.bRagdoll);
			ini["EVENTS"]["bDeath"] = std::to_string(m_Events.bDeath);
			ini["EVENTS"]["bMount"] = std::to_string(m_Events.bMount);
			ini["EVENTS"]["bMountCombat"] = std::to_string(m_Events.bMountCombat);
			ini["EVENTS"]["bDialogue"] = std::to_string(m_Events.bDialogue);
			ini["EVENTS"]["bScripted"] = std::to_string(m_Events.bScripted);
			ini["EVENTS"]["bThirdPerson"] = std::to_string(m_Events.bThirdPerson);

			ini["FOV"]["bEnableOverride"] = std::to_string(m_FOV.bEnableOverride);
			ini["FOV"]["fFirstPerson"] = std::to_string(m_FOV.fFirstPerson);
			ini["FOV"]["fFirstPersonCombat"] = std::to_string(m_FOV.fFirstPersonCombat);
			ini["FOV"]["fFurniture"] = std::to_string(m_FOV.fFurniture);
			ini["FOV"]["fCrafting"] = std::to_string(m_FOV.fCrafting);
			ini["FOV"]["fRagdoll"] = std::to_string(m_FOV.fRagdoll);
			ini["FOV"]["fDeath"] = std::to_string(m_FOV.fDeath);
			ini["FOV"]["fMount"] = std::to_string(m_FOV.fMount);
			ini["FOV"]["fMountCombat"] = std::to_string(m_FOV.fMountCombat);
			ini["FOV"]["fDialogue"] = std::to_string(m_FOV.fDialogue);
			ini["FOV"]["fScripted"] = std::to_string(m_FOV.fScripted);
			ini["FOV"]["fThirdPerson"] = std::to_string(m_FOV.fThirdPerson);

			ini["NEARDISTANCE"]["bEnableOverride"] = std::to_string(m_NearDistance.bEnableOverride);
			ini["NEARDISTANCE"]["fFirstPersonDefault"] = std::to_string(m_NearDistance.fFirstPersonDefault);
			ini["NEARDISTANCE"]["fPitchThreshold"] = std::to_string(m_NearDistance.fPitchThreshold);
			ini["NEARDISTANCE"]["fFirstPerson"] = std::to_string(m_NearDistance.fFirstPerson);
			ini["NEARDISTANCE"]["fFirstPersonCombat"] = std::to_string(m_NearDistance.fFirstPersonCombat);
			ini["NEARDISTANCE"]["fSitting"] = std::to_string(m_NearDistance.fSitting);
			ini["NEARDISTANCE"]["fFurniture"] = std::to_string(m_NearDistance.fFurniture);
			ini["NEARDISTANCE"]["fCrafting"] = std::to_string(m_NearDistance.fCrafting);
			ini["NEARDISTANCE"]["fRagdoll"] = std::to_string(m_NearDistance.fRagdoll);
			ini["NEARDISTANCE"]["fDeath"] = std::to_string(m_NearDistance.fDeath);
			ini["NEARDISTANCE"]["fMount"] = std::to_string(m_NearDistance.fMount);
			ini["NEARDISTANCE"]["fMountCombat"] = std::to_string(m_NearDistance.fMountCombat);
			ini["NEARDISTANCE"]["fDialogue"] = std::to_string(m_NearDistance.fDialogue);
			ini["NEARDISTANCE"]["fScripted"] = std::to_string(m_NearDistance.fScripted);
			ini["NEARDISTANCE"]["fThirdPerson"] = std::to_string(m_NearDistance.fThirdPerson);

			ini["HEADBOB"]["bIdle"] = std::to_string(m_Headbob.bIdle);
			ini["HEADBOB"]["bWalk"] = std::to_string(m_Headbob.bWalk);
			ini["HEADBOB"]["bRun"] = std::to_string(m_Headbob.bRun);
			ini["HEADBOB"]["bSprint"] = std::to_string(m_Headbob.bSprint);
			ini["HEADBOB"]["bCombat"] = std::to_string(m_Headbob.bCombat);
			ini["HEADBOB"]["bSneak"] = std::to_string(m_Headbob.bSneak);
			ini["HEADBOB"]["bSneakRoll"] = std::to_string(m_Headbob.bSneakRoll);
			ini["HEADBOB"]["fRotationIdle"] = std::to_string(m_Headbob.fRotationIdle);
			ini["HEADBOB"]["fRotationWalk"] = std::to_string(m_Headbob.fRotationWalk);
			ini["HEADBOB"]["fRotationRun"] = std::to_string(m_Headbob.fRotationRun);
			ini["HEADBOB"]["fRotationSprint"] = std::to_string(m_Headbob.fRotationSprint);
			ini["HEADBOB"]["fRotationCombat"] = std::to_string(m_Headbob.fRotationCombat);
			ini["HEADBOB"]["fRotationSneak"] = std::to_string(m_Headbob.fRotationSneak);
			ini["HEADBOB"]["fRotationSneakRoll"] = std::to_string(m_Headbob.fRotationSneakRoll);

			ini["CAMERA"]["fFirstPersonPosX"] = std::to_string(m_Camera.fFirstPersonPosX);
			ini["CAMERA"]["fFirstPersonPosY"] = std::to_string(m_Camera.fFirstPersonPosY);
			ini["CAMERA"]["fFirstPersonPosZ"] = std::to_string(m_Camera.fFirstPersonPosZ);
			ini["CAMERA"]["fFirstPersonCombatPosX"] = std::to_string(m_Camera.fFirstPersonCombatPosX);
			ini["CAMERA"]["fFirstPersonCombatPosY"] = std::to_string(m_Camera.fFirstPersonCombatPosY);
			ini["CAMERA"]["fFirstPersonCombatPosZ"] = std::to_string(m_Camera.fFirstPersonCombatPosZ);
			ini["CAMERA"]["fMountPosX"] = std::to_string(m_Camera.fMountPosX);
			ini["CAMERA"]["fMountPosY"] = std::to_string(m_Camera.fMountPosY);
			ini["CAMERA"]["fMountPosZ"] = std::to_string(m_Camera.fMountPosZ);
			ini["CAMERA"]["fMountCombatPosX"] = std::to_string(m_Camera.fMountCombatPosX);
			ini["CAMERA"]["fMountCombatPosY"] = std::to_string(m_Camera.fMountCombatPosY);
			ini["CAMERA"]["fMountCombatPosZ"] = std::to_string(m_Camera.fMountCombatPosZ);
			ini["CAMERA"]["fScriptedPosX"] = std::to_string(m_Camera.fScriptedPosX);
			ini["CAMERA"]["fScriptedPosY"] = std::to_string(m_Camera.fScriptedPosY);
			ini["CAMERA"]["fScriptedPosZ"] = std::to_string(m_Camera.fScriptedPosZ);

			ini["PSEUDOFPP"]["fHeightOffset"] = std::to_string(m_PseudoFPP.fHeightOffset);
			ini["PSEUDOFPP"]["fForwardOffset"] = std::to_string(m_PseudoFPP.fForwardOffset);
			ini["PSEUDOFPP"]["iToggleKey"] = std::to_string(m_PseudoFPP.iToggleKey);
			ini["PSEUDOFPP"]["iADSKey"] = std::to_string(m_PseudoFPP.iADSKey);
			ini["PSEUDOFPP"]["iGamepadTriggerThreshold"] = std::to_string(m_PseudoFPP.iGamepadTriggerThreshold);

			file.write(ini);
		}
	}

	void Config::UpdateMCMSettingsWriteTime()
	{
		WIN32_FILE_ATTRIBUTE_DATA data{};
		if (!GetFileAttributesExA(m_MCMSettingsPath.c_str(), GetFileExInfoStandard, &data))
		{
			m_MCMSettingsLastWrite = 0;
			return;
		}

		m_MCMSettingsLastWrite = (static_cast<std::uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) |
			static_cast<std::uint64_t>(data.ftLastWriteTime.dwLowDateTime);
	}

	void Config::ReloadPseudoFPP()
	{
		try
		{
			mINI::INIFile file(m_MCMSettingsPath.c_str());
			mINI::INIStructure ini;
			if (!file.read(ini))
				return;

			auto getFloat = [&ini](const std::string& sec, const std::string& key, float def) {
				auto v = ini.get(sec).get(key);
				return v.empty() ? def : std::stof(v);
			};
			auto getInt = [&ini](const std::string& sec, const std::string& key, int def) {
				auto v = ini.get(sec).get(key);
				return v.empty() ? def : std::stoi(v);
			};

			m_PseudoFPP.fHeightOffset = getFloat("PSEUDOFPP", "fHeightOffset", m_PseudoFPP.fHeightOffset);
			m_PseudoFPP.fForwardOffset = getFloat("PSEUDOFPP", "fForwardOffset", m_PseudoFPP.fForwardOffset);
			m_PseudoFPP.iToggleKey = getInt("PSEUDOFPP", "iToggleKey", m_PseudoFPP.iToggleKey);
			m_PseudoFPP.iADSKey = getInt("PSEUDOFPP", "iADSKey", m_PseudoFPP.iADSKey);
			m_PseudoFPP.iGamepadTriggerThreshold = getInt("PSEUDOFPP", "iGamepadTriggerThreshold", m_PseudoFPP.iGamepadTriggerThreshold);
		}
		catch (...)
		{
			LOG_WARN("MCM settings reload failed: {}", m_MCMSettingsPath.c_str());
		}
	}

	void Config::ReloadMCMSettings()
	{
		if (m_MCMSettingsPath.empty())
			return;

		std::uint64_t newWriteTime = 0;
		WIN32_FILE_ATTRIBUTE_DATA data{};
		if (GetFileAttributesExA(m_MCMSettingsPath.c_str(), GetFileExInfoStandard, &data))
		{
			newWriteTime = (static_cast<std::uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) |
				static_cast<std::uint64_t>(data.ftLastWriteTime.dwLowDateTime);
		}

		if (newWriteTime == 0 || newWriteTime == m_MCMSettingsLastWrite)
			return;

		m_MCMSettingsLastWrite = newWriteTime;
		ReloadPseudoFPP();

		LOG_INFO("Config: MCM settings reloaded (PSEUDOFPP height={:.4f} forward={:.4f} toggleKey={})",
			m_PseudoFPP.fHeightOffset, m_PseudoFPP.fForwardOffset, m_PseudoFPP.iToggleKey);
	}

	void Config::UpdateMCMKeybindsWriteTime()
	{
		WIN32_FILE_ATTRIBUTE_DATA data{};
		if (!GetFileAttributesExA(m_MCMKeybindsPath.c_str(), GetFileExInfoStandard, &data))
		{
			m_MCMKeybindsLastWrite = 0;
			return;
		}

		m_MCMKeybindsLastWrite = (static_cast<std::uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) |
			static_cast<std::uint64_t>(data.ftLastWriteTime.dwLowDateTime);
	}

	std::int32_t Config::ReadMCMToggleKey()
	{
		// Data\MCM\Settings\Keybinds.json is written by F4MCM whenever the MCM
		// menu is closed with modified keybinds. Find the entry for our keybind
		// ("togglePseudo") and read its keycode into m_MCMToggleKey.
		std::ifstream file(m_MCMKeybindsPath);
		if (!file.is_open()) {
			LOG_INFO("Config: Keybinds.json not found at '{}'", m_MCMKeybindsPath.c_str());
			m_MCMToggleKey = 0;
			return 0;
		}

		try {
			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string json = buffer.str();

			// Locate the keybind object for our mod. Format:
			// {"keycode":115,"modifiers":0,"modName":"SomaticCameraFO4","id":"togglePseudo"}
			// Search for the id field, then read the keycode within the same object.
			const std::string idKey = "\"id\":\"togglePseudo\"";
			auto idPos = json.find(idKey);
			if (idPos == std::string::npos) {
				LOG_WARN("Config: 'togglePseudo' id not found in Keybinds.json");
				m_MCMToggleKey = 0;
				return 0;
			}

			// Find the enclosing object braces around the id field.
			auto openBrace = json.rfind('{', idPos);
			auto closeBrace = json.find('}', idPos);
			if (openBrace == std::string::npos || closeBrace == std::string::npos) {
				LOG_WARN("Config: malformed object braces around togglePseudo keybind");
				m_MCMToggleKey = 0;
				return 0;
			}

			std::string object = json.substr(openBrace, closeBrace - openBrace + 1);

			// Verify it is our mod's entry.
			if (object.find("\"modName\":\"SomaticCameraFO4\"") == std::string::npos) {
				LOG_WARN("Config: togglePseudo keybind entry has wrong modName");
				m_MCMToggleKey = 0;
				return 0;
			}

			const std::string keyCodeKey = "\"keycode\":";
			auto keyCodePos = object.find(keyCodeKey);
			if (keyCodePos == std::string::npos) {
				LOG_WARN("Config: 'keycode' field not found in togglePseudo keybind entry");
				m_MCMToggleKey = 0;
				return 0;
			}

			keyCodePos += keyCodeKey.size();
			auto valueEnd = object.find_first_not_of("0123456789", keyCodePos);
			if (valueEnd == keyCodePos) {
				LOG_WARN("Config: keycode value is empty or invalid in togglePseudo keybind");
				m_MCMToggleKey = 0;
				return 0;
			}

			std::int32_t keyCode = std::stoi(object.substr(keyCodePos, valueEnd - keyCodePos));
			m_MCMToggleKey = keyCode;
			LOG_INFO("Config: MCM toggle key from Keybinds.json = {}", keyCode);
			return keyCode;
		}
		catch (...)
		{
			LOG_WARN("Config: failed to parse MCM Keybinds.json at '{}'", m_MCMKeybindsPath.c_str());
			m_MCMToggleKey = 0;
			return 0;
		}
	}

	void Config::ReloadMCMKeybinds()
	{
		if (m_MCMKeybindsPath.empty())
			return;

		std::uint64_t newWriteTime = 0;
		WIN32_FILE_ATTRIBUTE_DATA data{};
		if (GetFileAttributesExA(m_MCMKeybindsPath.c_str(), GetFileExInfoStandard, &data))
		{
			newWriteTime = (static_cast<std::uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) |
				static_cast<std::uint64_t>(data.ftLastWriteTime.dwLowDateTime);
		}

		if (newWriteTime == 0 || newWriteTime == m_MCMKeybindsLastWrite)
			return;

		m_MCMKeybindsLastWrite = newWriteTime;
		ReadMCMToggleKey();
		LOG_INFO("Config: MCM keybinds reloaded from '{}' (toggleKey={})",
			m_MCMKeybindsPath.c_str(), m_MCMToggleKey);
	}

}

