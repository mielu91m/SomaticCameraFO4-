/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

namespace Events {

	class Observer :
		public RE::BSTEventSink<RE::MenuOpenCloseEvent> {

	public:
		static Observer* Get();

		void Register();
		void CheckSPIM();

	private:
		Observer() = default;
		Observer(const Observer&) = delete;
		Observer(Observer&&) = delete;
		~Observer() = default;

		Observer& operator=(const Observer&) = delete;
		Observer& operator=(Observer&&) = delete;

		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

		void ResetArms();
	};

}

