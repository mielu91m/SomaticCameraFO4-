/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "cameras/ICamera.h"

namespace Patch {
	class Hooks;
}

namespace Events {
	class Observer;
}

namespace ImprovedCamera {

	class ImprovedCameraFO4 {

	public:
		ImprovedCameraFO4();
		~ImprovedCameraFO4() = default;

	public:
		bool ProcessInput(const RE::InputEvent* const* a_event);

		void UpdateCamera(std::uint8_t currentID, std::uint8_t previousID, float deltaTime);
		void UpdateFirstPerson();
		void UpdateHeadTracking();
		void ResetPlayerNodes();
		void ForceFirstPerson();
		void ForceThirdPerson();
		void TogglePOV();
		void ResetState(bool forced = false);
		void Ragdoll(RE::Actor* actor);

		bool IsFirstPerson() const { return m_IsFirstPerson; }
		bool IsPseudoFPPActive() const { return m_PseudoFPPActive; }
		void SetPseudoFPPActive(bool a_active);
		bool IsPseudoMenuStandDown() const { return m_PseudoMenuBlockFrames > 0; }
		bool IsTerminalMenuOpen() const { return m_TerminalMenuIsOpen; }
		bool IsPlayerInFurniture() const;
		void PopPseudoK3rdPerson();
		void RestorePseudoK3rdPerson();
		float UpdateNearDistance(float fNear);

		void RequestAPIs();
		void DetectMods();
		void PerFrameUpdate();

	private:
		class NodeOverride {

		public:
			NodeOverride(RE::NiNode* node, float scale) :
				node(node)
			{
				if (node) {
					old_scale = node->local.scale;
					node->local.scale = scale;
				}
			}
			~NodeOverride()
			{
				if (node)
					node->local.scale = old_scale;
			}

		private:
			RE::NiNode* node = nullptr;
			float old_scale = 1.0f;
		};

		void UpdateSkeleton(bool show);
		void DisplayShadows(bool show);

		void UpdateFOV(RE::PlayerCamera* camera);

		void TranslateCamera();
		void TranslateFirstPersonModel();
		void TranslateThirdPersonModel();
		void AdjustModelPosition(RE::NiPoint3& position, bool headbob);

		bool HeadRotation();
		void ScalePoint(RE::NiPoint3* point, float scale);

		void SetupCameraData();

	private:
		Systems::Config* m_pluginConfig = nullptr;

		Interface::Camera m_Camera{};
		Interface::ICamera* m_ICamera = nullptr;

		std::uint8_t m_CameraEventID = 0;
		float m_DeltaTime = 0.0f;
		uint8_t m_PreviousCameraID = 255;
		uint8_t m_CurrentCameraID = 255;

		bool m_IsThirdPersonForced = false;
		bool m_IsFirstPerson = false;
		bool m_IsFakeCamera = false;
		uint8_t m_LastStateID = 0;
		uint8_t m_iRagdollFrame = 0;
		RE::NiPoint3 m_thirdpersonLocalTranslate{};

		bool m_SmoothCamSnapshot = false;
		bool m_PseudoFPPActive = false;
		bool m_PseudoPushedK3rdPerson = false;
		bool m_PseudoPendingK3rdPersonPush = false;
		int m_PseudoReenableFrameCount = 0;
		bool m_PseudoActiveBeforePipboy = false;
		// True while a TerminalMenu session is active (from when the menu
		// opens until it fully closes). While set, pseudo stays disabled and
		// the furniture-exit re-enable in PerFrameUpdate is deferred, so
		// pseudo cannot capture the terminal camera/animations mid-session.
		bool m_TerminalMenuIsOpen = false;
		// Set true when pseudo detected a furniture/transition exit (camera in
		// kFurniture/kPCTransition, transitioning to k3rdPerson/kFirstPerson).
		// PerFrameUpdate re-enables pseudo when the furniture state is gone.
		bool m_PseudoPendingFurnitureExit = false;
		// Grace frames counted down ONLY after the player has fully left the
		// furniture object (terminal get-up / chair stand-up animation). While
		// the player still occupies furniture (camera in kFurniture/kPCTransition
		// OR currentFurniture/occupiedFurniture handle alive) it is re-armed to
		// a fixed value so pseudo cannot re-engage mid-exit; once the player is
		// fully out it counts down and pseudo is re-enabled at 0.
		int m_PseudoFurnitureExitGraceFrames = 0;
		// Hard stand-down counter: while > 0 every pseudo hook behaves as if a
		// blocking menu is open, so the engine can freely run the menu camera
		// (Pip-Boy / terminal transition to kFirstPerson, etc.). Set on menu
		// open AND close (grace period so the engine's menu camera restore
		// settles), counted down once per frame in PerFrameUpdate.
		int m_PseudoMenuBlockFrames = 0;

		friend class Events::Observer;
		friend class ::Patch::Hooks;
	};

}

