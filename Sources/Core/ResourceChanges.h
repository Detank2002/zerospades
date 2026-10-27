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
#include <unordered_map>
#include <vector>

namespace spades {
	/** A file that failed to load, and why. */
	struct ResourceLoadError {
		std::string path;
		std::string message;
	};

	/**
	 * The resource files whose content may have changed, for example because the
	 * mounted mods changed. Paths compare the way zip entries do: case-insensitive,
	 * with '/' as the separator.
	 *
	 * Resources come in two kinds. Game assets (models, images, sounds and the
	 * skin scripts) are loaded by the game's GameResources and can be swapped
	 * while it runs. Everything else (fonts, shaders, translations, the menus'
	 * own images and sounds...) belongs to the software and takes a restart.
	 */
	class ResourceChanges {
		// Normalized path -> the path as the file system spells it.
		std::unordered_map<std::string, std::string> paths;

	public:
		/** The kinds of game assets. */
		enum class AssetKind {
			None,          // not a game asset: the software's, a restart picks it up
			Model,         // .kv6
			ModelMetadata, // the .meta.json VoxelModelLoader reads beside a model
			Image,         // a format Bitmap::Load reads
			Sound,
			Script, // the script module only holds the skins
		};

		/** The key two spellings of one file share; `foo.weak` is `foo`. */
		static std::string Normalize(const std::string& path);

		static AssetKind GetAssetKind(const std::string& path);
		static bool IsGameAsset(const std::string& path) {
			return GetAssetKind(path) != AssetKind::None;
		}

		void Add(const std::string& path);
		bool Contains(const std::string& path) const;
		/** Whether a changed path is in `dir` or below it. */
		bool ContainsUnder(const std::string& dir) const;
		bool IsEmpty() const { return paths.empty(); }

		/** The changed paths, as the file system spells them. */
		std::vector<std::string> GetPaths() const;
	};
} // namespace spades
