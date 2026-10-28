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
#include <unordered_set>
#include <vector>

#include <Core/FileManager.h>
#include <Core/PakOverlay.h>
#include <Core/RefCountedObject.h>

namespace spades {
	namespace client {
		/**
		 * A mod the game applies to some players only, such as a player or weapon
		 * skin. Its paks are read into memory but mounted nowhere: they sit on top
		 * of what the game sees only while those players' assets load (`Scope`).
		 *
		 * A skin holds game assets only. If it brings scripts, it gets a script
		 * module of its own, built from the game's scripts with its own on top.
		 */
		class Skin : public RefCountedObject {
		public:
			/**
			 * Reads the mod `name` from Mods/ and checks every file in it loads.
			 * Raises if it isn't there, doesn't load, or holds more than game assets.
			 */
			static Handle<Skin> Load(const std::string& name);

			const std::string& GetName() const { return name; }
			/** Whether the skin has its own `path`, or its own metadata for it. */
			bool Has(const std::string& path) const;

			/** The script module to make this skin's players' skins from. */
			const std::string& GetScriptModule() const { return scriptModule; }
			/**
			 * Builds the skin's script module again, from the game's scripts as they
			 * are mounted now. Returns why it doesn't build, or ""; on an error the
			 * previous module stays. Nothing to do without scripts of its own.
			 */
			std::string BuildScripts();

			/** While one is alive, the game's reads on this thread see the skin. */
			class Scope {
				FileManager::GameScope game;
				FileManager::Overlay overlay;

			public:
				explicit Scope(const Skin&);
			};

		protected:
			~Skin();

		private:
			std::string name;
			PakOverlay::PakSet paks;
			std::unordered_set<std::string> files; // normalized
			bool hasScripts = false;
			std::string scriptModule = "Client";

			Skin() = default;
			std::vector<IFileSystem*> GetFileSystems() const;
		};
	} // namespace client
} // namespace spades
