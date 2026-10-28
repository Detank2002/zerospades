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

#include "Skin.h"
#include "GameResources.h"
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/ResourceChanges.h>
#include <Core/VoxelModelLoader.h>
#include <ScriptBindings/ScriptManager.h>

namespace spades {
	namespace client {
		Handle<Skin> Skin::Load(const std::string& name) {
			SPADES_MARK_FUNCTION();

			std::vector<std::string> paths = PakOverlay::FindModPaks(name);
			if (paths.empty())
				SPRaise("There is no mod named '%s' in Mods/.", name.c_str());

			Handle<Skin> skin{new Skin(), false};
			skin->name = name;
			for (const std::string& path : paths)
				skin->paks.push_back(PakOverlay::Load(path, name));

			std::vector<std::string> assets;
			for (const auto& pak : skin->paks) {
				for (const std::string& file : pak->GetFiles()) {
					if (file.empty() || file.back() == '/')
						continue;
					ResourceChanges::AssetKind kind = ResourceChanges::GetAssetKind(file);
					if (kind == ResourceChanges::AssetKind::None)
						SPRaise("'%s' is not a game asset; a skin can only hold game assets.",
						        file.c_str());
					if (kind == ResourceChanges::AssetKind::Script)
						skin->hasScripts = true;
					else
						assets.push_back(file);
					skin->files.insert(ResourceChanges::Normalize(file));
				}
			}

			{
				Scope scope(*skin);
				for (const std::string& file : assets) {
					try {
						GameResources::CheckFile(file);
					} catch (const std::exception& ex) {
						SPRaise("%s: %s", file.c_str(), ex.what());
					}
				}
			}

			if (skin->hasScripts) {
				std::string error = skin->BuildScripts();
				if (!error.empty())
					SPRaise("%s", error.c_str());
			}
			return skin;
		}

		Skin::~Skin() {
			if (scriptModule != "Client")
				ScriptManager::GetInstance()->DiscardModule(scriptModule);
		}

		bool Skin::Has(const std::string& path) const {
			return files.count(ResourceChanges::Normalize(path)) != 0 ||
			       files.count(
			         ResourceChanges::Normalize(VoxelModelLoader::GetMetadataPath(path))) != 0;
		}

		std::string Skin::BuildScripts() {
			SPADES_MARK_FUNCTION();

			if (!hasScripts)
				return {};
			std::string module = "Skin:" + name;
			std::string file;
			std::string error;
			{
				Scope scope(*this);
				error = ScriptManager::GetInstance()->ReplaceModule(module, file);
			}
			if (error.empty())
				scriptModule = module;
			return error;
		}

		std::vector<IFileSystem*> Skin::GetFileSystems() const {
			std::vector<IFileSystem*> fileSystems;
			// The last pak is on top.
			for (auto it = paks.rbegin(); it != paks.rend(); ++it)
				fileSystems.push_back((*it)->GetFileSystem());
			return fileSystems;
		}

		Skin::Scope::Scope(const Skin& skin) : overlay(skin.GetFileSystems()) {}
	} // namespace client
} // namespace spades
