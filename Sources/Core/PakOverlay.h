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

#include <memory>
#include <string>
#include <vector>

#include "ResourceChanges.h"

namespace spades {
	class IFileSystem;
	class IStream;
	class ZipFileSystem;

	/**
	 * Mod paks mounted over the base paks, in two layers:
	 *
	 * - Mods: the enabled mods, seen by everything.
	 * - Game: the mods a game loaded for itself, such as skins and weapon mods.
	 *   They sit on top of the enabled mods, and only the game's own reads see
	 *   them (FileManager::GameScope), so the software never picks them up.
	 *
	 * Within a layer, the last pak is on top. Each pak is read into memory, so
	 * the file on disk is never held open and can be replaced while the game
	 * runs. A pak taken out of a layer stays in memory for as long as someone
	 * holds it, so a change is undone by setting the previous paks again.
	 */
	class PakOverlay {
	public:
		enum class Layer { Mods, Game };

		/** A pak read into memory, mounted or not. */
		class Pak {
			friend class PakOverlay;
			std::string owner; // the mod it belongs to
			std::string path;
			std::string stamp; // what the file on disk looked like when read, or ""
			std::string id;    // path and checksum: equal ids, equal files
			std::unique_ptr<IStream> stream;        // outlives the file system
			std::unique_ptr<IFileSystem> unmounted; // owns `fs` while not mounted
			ZipFileSystem* fs;

		public:
			Pak();
			~Pak();
			const std::string& GetOwner() const { return owner; }
			const std::string& GetId() const { return id; }
			std::vector<std::string> GetFiles() const;
			/** Its file system; mounted or not, valid while the pak lives. */
			IFileSystem* GetFileSystem() const;
		};
		using PakSet = std::vector<std::shared_ptr<Pak>>;

		/** The files a layer change made read from another pak, or from none. */
		struct Changes {
			ResourceChanges game;     // as the game sees them: both layers
			ResourceChanges software; // as the software sees them: the enabled mods
		};

		/**
		 * The paks of the mod named `name`, a pak in Mods/ or a folder of paks
		 * there, in mount order. Empty if there is no such mod.
		 */
		static std::vector<std::string> FindModPaks(const std::string& name);

		/**
		 * Reads the pak at `path`, a resource path, for the mod `owner`. Raises if it
		 * can't. With a `stamp`, such as the file's size and time, a pak already
		 * read from `path` with the same stamp is shared instead of read again.
		 */
		static std::shared_ptr<Pak> Load(const std::string& path, const std::string& owner,
		                                 const std::string& stamp = {});

		static const PakSet& GetLayer(Layer);

		/**
		 * Mounts `paks` as `layer`, in place of the paks it held, and returns what
		 * that changed. To undo it, set the previous paks again.
		 */
		static Changes SetLayer(Layer, PakSet paks);

		/** The owner of the pak the game reads `path` from, or "" if no mod has it. */
		static std::string GetOwner(const std::string& path);
	};
} // namespace spades
