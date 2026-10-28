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

#include <unordered_set>

#include "GameAssetReload.h"
#include "IAudioDevice.h"
#include "IRenderer.h"
#include <Core/Debug.h>
#include <Core/FileManager.h>
#include <Core/VoxelModelLoader.h>
#include <ScriptBindings/ScriptManager.h>

namespace spades {
	namespace client {
		GameAssetReload::GameAssetReload(GameResources& resources,
		                                 const ResourceChanges& changes,
		                                 std::vector<ResourceLoadError>& errors)
		    : resources(resources) {
			SPADES_MARK_FUNCTION();

			// The game's scripts include the mods it loaded for itself.
			FileManager::GameScope scope;
			if (changes.ContainsUnder("Scripts")) {
				std::string file;
				std::string error = ScriptManager::GetInstance()->PrepareReload(file);
				if (error.empty())
					scriptsPrepared = true;
				else
					errors.push_back({file, error});
			}
			assets = resources.PrepareReload(changes, errors);
		}

		GameAssetReload::~GameAssetReload() {
			if (scriptsPrepared)
				ScriptManager::GetInstance()->DiscardReload();
		}

		void GameAssetReload::Commit() {
			SPADES_MARK_FUNCTION();

			if (scriptsPrepared) {
				ScriptManager::GetInstance()->CommitReload();
				scriptsPrepared = false;
			}
			resources.CommitReload(*assets);
		}

		std::vector<std::string>
		GameAssetReload::GetSoftwareChanges(const ResourceChanges& changes, IRenderer& renderer,
		                                    IAudioDevice& audio) {
			std::unordered_set<std::string> loaded;
			for (const std::string& file : renderer.GetRegisteredFiles()) {
				loaded.insert(ResourceChanges::Normalize(file));
				loaded.insert(ResourceChanges::Normalize(VoxelModelLoader::GetMetadataPath(file)));
			}
			for (const std::string& file : audio.GetRegisteredFiles())
				loaded.insert(ResourceChanges::Normalize(file));

			std::vector<std::string> software;
			for (const std::string& path : changes.GetPaths()) {
				if (!ResourceChanges::IsGameAsset(path) ||
				    loaded.count(ResourceChanges::Normalize(path)) != 0)
					software.push_back(path);
			}
			return software;
		}
	} // namespace client
} // namespace spades
