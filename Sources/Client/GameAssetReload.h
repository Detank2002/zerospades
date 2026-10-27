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

#include "GameResources.h"
#include <Core/ResourceChanges.h>

namespace spades {
	namespace client {
		class IAudioDevice;
		class IRenderer;

		/**
		 * A change of the game's assets on its way in, once the mounted files
		 * changed: the skin scripts and every model, image and sound it touches
		 * are loaded and checked, and wait to replace what is loaded. `Commit`
		 * swaps them all in; dropped without it, nothing changes.
		 */
		class GameAssetReload {
			GameResources& resources;
			std::unique_ptr<GameResources::Reload> assets;
			bool scriptsPrepared = false;

		public:
			/** Loads what `changes` touches. Each file that fails goes to `errors`. */
			GameAssetReload(GameResources&, const ResourceChanges& changes,
			                std::vector<ResourceLoadError>& errors);
			~GameAssetReload();

			GameAssetReload(const GameAssetReload&) = delete;
			void operator=(const GameAssetReload&) = delete;

			void Commit();

			/**
			 * The changed files only a restart picks up: the ones that are not game
			 * assets, and the ones the software loaded itself, through `renderer`
			 * and `audio`.
			 */
			static std::vector<std::string> GetSoftwareChanges(const ResourceChanges& changes,
			                                                   IRenderer& renderer,
			                                                   IAudioDevice& audio);
		};
	} // namespace client
} // namespace spades
