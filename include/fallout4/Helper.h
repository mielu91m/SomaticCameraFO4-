/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "RE/Fallout.h"

namespace RE {

	enum class EQUIPPED_ITEMTYPE_IDS
	{
		kFist = 0,
		kSword = 1,
		kDagger = 2,
		kAxe = 3,
		kMace = 4,
		kGreatsword = 5,
		kWarhammer = 6,
		kBow = 7,
		kStaff = 8,
		kMagic = 9,
		kShield = 10,
		kTorch = 11,
		kCrossbow = 12
	};
}

namespace Helper {

	inline RE::NiAVObject* FindNode(RE::NiAVObject* node, const std::string& name)
	{
		if (!node)
			return nullptr;

		if (node->name == name)
			return node;

		auto niNode = node->IsNode();
		if (niNode)
		{
			for (auto& child : niNode->children)
			{
				if (child)
				{
					auto result = FindNode(child.get(), name);
					if (result)
						return result;
				}
			}
		}
		return nullptr;
	}

	inline void UpdateNode(RE::NiAVObject* node, std::uint32_t flags, float time)
	{
		if (!node)
			return;

		RE::NiUpdateData updateData;
		updateData.flags = flags;
		updateData.time = time;
		node->Update(updateData);
	}

	inline RE::NiAVObject* GetHeadNode(RE::NiAVObject* node)
	{
		static const std::vector<std::string> headBones = {
			"HEAD", "Head", "head", "NPC Head [Head]", "NPC_head", "Bip01 Head",
			"Bip01_Head", "HeadNode", "C_Head", "C_HeadBone", "R_Head", "L_Head"
		};

		if (!node)
			return nullptr;

		for (const auto& bone : headBones)
		{
			auto result = FindNode(node, bone);
			if (result)
				return result;
		}
		return nullptr;
	}

	inline bool CanLook()
	{
		return true;
	}

	inline bool CanMove()
	{
		return true;
	}

	inline bool CannotMoveAndLook()
	{
		return false;
	}

	inline bool IsSitting(RE::PlayerCharacter* player)
	{
		if (!player)
			return false;
		auto actorState = static_cast<RE::ActorState*>(player);
		if (!actorState)
			return false;
		return actorState->DoGetSitSleepState() == RE::SIT_SLEEP_STATE::kIsSitting;
	}

	inline bool IsWeaponDrawn(RE::PlayerCharacter* player)
	{
		if (!player)
			return false;
		auto actorState = static_cast<RE::ActorState*>(player);
		if (!actorState)
			return false;
		return actorState->GetWeaponMagicDrawn();
	}

	inline bool IsSneaking(RE::PlayerCharacter* player)
	{
		if (!player)
			return false;
		return player->IsSneaking();
	}

	inline bool IsSwimming(RE::PlayerCharacter* player)
	{
		if (!player)
			return false;
		auto actorState = static_cast<RE::ActorState*>(player);
		if (!actorState)
			return false;
		return actorState->IsSwimming();
	}

	inline bool IsOnMount(RE::PlayerCharacter* player)
	{
		if (!player)
			return false;
		return player->boolFlags.any(RE::Actor::BOOL_FLAGS::kIsAMount);
	}

	inline bool IsInPowerArmor(RE::PlayerCharacter* player)
	{
		if (!player)
			return false;
		return RE::PowerArmor::ActorInPowerArmor(*player);
	}

	inline bool IsInKillMove(RE::PlayerCharacter* player)
	{
		if (!player)
			return false;
		return player->boolFlags.any(RE::Actor::BOOL_FLAGS::kIsInKillMove);
	}

	inline bool IsDead(RE::PlayerCharacter* player)
	{
		if (!player)
			return false;
		return player->IsDead(false);
	}

	inline bool IsInBleedout(RE::PlayerCharacter* player)
	{
		if (!player)
			return false;
		return player->boolFlags.any(RE::Actor::BOOL_FLAGS::kInBleedoutAnimation);
	}

	inline bool IsInDialogue(RE::PlayerCharacter* player)
	{
		if (!player)
			return false;
		auto actorState = static_cast<RE::ActorState*>(player);
		if (!actorState)
			return false;
		return actorState->talkingToPlayer;
	}

	inline std::int32_t GetWeaponType(RE::PlayerCharacter* player)
	{
		if (!player)
			return -1;

		RE::BGSObjectInstance objInstance(nullptr, nullptr);
		if (!player->GetEquippedItem(&objInstance, RE::BGSEquipIndex{0}))
			return -1;

		auto weapon = objInstance.object->As<RE::TESObjectWEAP>();
		if (!weapon)
			return -1;

		return static_cast<std::int32_t>(weapon->weaponData.type.underlying());
	}

	inline bool IsRangedWeaponEquipped(RE::PlayerCharacter* player, bool crossbow = false)
	{
		auto type = GetWeaponType(player);
		if (type < 0)
			return false;
		return type == 7;
	}
}

