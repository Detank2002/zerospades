/*
 Copyright (c) 2026 ZeroSpades developers.

 This file is part of ZeroSpades, a fork of OpenSpades.

 ZeroSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 ZeroSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with ZeroSpades.	 If not, see <http://www.gnu.org/licenses/>.

 */

#pragma once

#include <string>
#include <vector>

namespace spades {
	namespace client {
		class GameResources;
		class IAudioDevice;
		class IRenderer;
	} // namespace client
	namespace gui {
		/** What applying the enabled mods took. */
		enum class ModApplyResult {
			Applied,      // in place, no restart
			NeedsRestart, // the software's own files changed; the enabled set waits for it
			Refused,      // nothing changed, and the previous set is enabled again
		};

		/** A mod that failed to load, and why. It is disabled again. */
		struct ModFailure {
			std::string mod;
			std::string reason;
		};

		struct ModApplyReport {
			ModApplyResult result = ModApplyResult::Applied;
			std::vector<ModFailure> failures;
			std::string error; // why, when Refused
		};

		/**
		 * The enabled mods' paks, mounted over the base paks (see PakOverlay): the
		 * last enabled mod is on top and wins conflicts. A --try-mod run mounts
		 * none of them.
		 */
		class ModOverlay {
		public:
			/**
			 * Mounts the enabled set when the game starts. A mod whose paks can't
			 * be read is disabled; `TakeStartupFailures` returns them.
			 */
			static void Mount();
			/**
			 * Starts the script engine on the mounted scripts. While they don't
			 * build, the last enabled mod with scripts is disabled.
			 */
			static void StartScripts();
			static std::vector<ModFailure> TakeStartupFailures();

			/**
			 * Mounts the enabled set in place of the mounted one. Runs on the main
			 * thread, between frames.
			 *
			 * Each mod with a pak, a script or a game asset that fails to load is
			 * disabled again and left out. When the rest only changes game assets,
			 * those are reloaded in place (in `game`, the running game's assets, if
			 * any) and the result is `Applied`. When it changes the software's own
			 * files, found through `renderer` and `audio` (the software's), the
			 * previous set stays mounted until the restart. A failure no mod can be
			 * blamed for refuses the whole change.
			 */
			static ModApplyReport Apply(client::IRenderer& renderer, client::IAudioDevice& audio,
			                            client::GameResources* game);

			/** Whether the enabled set differs from the mounted one. */
			static bool HasPendingChanges();

			/** What went wrong in `report`, for the user; "" if nothing did. */
			static std::string Describe(const ModApplyReport& report);
			static std::string Describe(const std::vector<ModFailure>& failures);
		};
	} // namespace gui
} // namespace spades
