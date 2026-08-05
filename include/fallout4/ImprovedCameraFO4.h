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

		friend class Events::Observer;
		friend class ::Patch::Hooks;
	};

}

