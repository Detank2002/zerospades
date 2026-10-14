/*
 Copyright (c) 2026 Fran6nd, ZeroSpades developers.

 This file is part of ZeroSpades, a fork of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#pragma once

#include <string>
#include <vector>

#include <Gui/Utils/ConsoleCommandCandidate.h>
#include <Gui/UI/Widgets/FieldWithHistory.h>

namespace spades {
	namespace client {
		class IFont;
	}
	namespace gui {
		class ConsoleHelper;

		/**
		 * Displays console command candidates below a `ConsoleCommandField`. The box fits its content
		 * but stays on the screen: a description that is too wide continues on the following rows, and
		 * only the candidates that fit above the bottom edge are shown.
		 */
		class ConsoleCommandFieldCandidateView : public ui::UIElement {
			/** One displayed row: the first one of a candidate also shows its name. */
			struct Row {
				size_t candidate = 0;
				std::string text; // the part of the description shown in this row
				bool first = false;
				std::string head; // the part of the name that was already typed
				std::string tail;
				float headW = 0.0F;
			};

			std::vector<ConsoleCommandCandidate> candidates;
			size_t prefixLength;
			size_t extraCount; // matches that are not in `candidates`

			// The layout is measured on the first frame and reused by every later one.
			bool laidOut = false;
			std::vector<Row> rows;
			std::string moreText; // "+N more"; empty when every candidate is shown
			float nameColumnW = 0.0F;
			float boxW = 0.0F;

			void BuildRows(client::IFont& font);

		public:
			/**
			 * `prefixLength` is how many characters of each name the user already typed.
			 * `extraCount` is how many more matches exist besides `candidates`.
			 */
			ConsoleCommandFieldCandidateView(ui::UIManager* manager,
			                                 std::vector<ConsoleCommandCandidate> candidates,
			                                 size_t prefixLength, size_t extraCount);
			void Render() override;
		};

		/** A `Field` with command name autocompletion. */
		class ConsoleCommandField : public ui::FieldWithHistory {
			Handle<ConsoleCommandFieldCandidateView> valueView;
			ConsoleHelper* helper; // weak

		public:
			ConsoleCommandField(ui::UIManager* manager,
			                    std::vector<ui::CommandHistoryItem>* history, ConsoleHelper* helper);

			void OnChanged() override;
			void Clear() override;
			void KeyDown(const std::string& key) override;
		};
	} // namespace gui
} // namespace spades
