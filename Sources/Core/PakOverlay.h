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
	 * Paks mounted over the base paks, in layers: the enabled mods, then on top
	 * the mods a game loaded for itself, such as skins and weapon mods. Within a
	 * layer, the last pak is on top.
	 *
	 * Each pak is read into memory once, so the file on disk is never held open
	 * and can be overwritten or deleted while the game runs. A pak taken out of a
	 * layer stays in memory for as long as someone holds it, so a change can be
	 * undone by putting the previous paks back.
	 */
	class PakOverlay {
	public:
		enum class Layer { Mods, Game };

		/** A pak read into memory, mounted or not. */
		class Pak {
			friend class PakOverlay;
			std::string owner; // the mod it belongs to
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
		};
		using PakSet = std::vector<std::shared_ptr<Pak>>;

		/** Reads the pak at `path`, a resource path, for `owner`. Raises if it can't. */
		static std::shared_ptr<Pak> Load(const std::string& path, const std::string& owner);

		static const PakSet& GetLayer(Layer);

		/**
		 * Mounts `paks` as `layer`, in place of the paks it held, and returns the
		 * files that are now read from another pak, or from none. To undo it, set
		 * the previous paks again.
		 */
		static ResourceChanges SetLayer(Layer, PakSet paks);

		/** The owner of the overlay pak `path` is read from, or "" if none is. */
		static std::string GetOwner(const std::string& path);
	};
} // namespace spades
