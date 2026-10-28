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

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "IAudioChunk.h"
#include "IImage.h"
#include "IModel.h"
#include "Skin.h"
#include <Core/RefCountedObject.h>
#include <Core/ResourceChanges.h>

namespace spades {
	namespace client {
		class IAudioDevice;
		class IRenderer;

		/**
		 * The assets of one game: every model, image and sound the game loads goes
		 * through here, by way of the renderer and audio device `CreateRenderer` and
		 * `CreateAudioDevice` return. Each file is loaded once, shared, and freed
		 * with the game.
		 *
		 * The software's own assets (the menus, the loading screen, fonts) are
		 * loaded by the base renderer and audio device instead, and only a restart
		 * replaces them. Game assets are replaced while the game runs:
		 * `PrepareReload` loads every changed file, and `CommitReload` swaps the
		 * new content in, so the holders of the old objects see it.
		 */
		class GameResources : public RefCountedObject {
		public:
			/** Content loaded by `PrepareReload`, waiting to replace what is held. */
			class Reload {
				friend class GameResources;
				// target, then its new content
				std::vector<std::pair<Handle<IImage>, Handle<IImage>>> images;
				std::vector<std::pair<Handle<IModel>, Handle<IModel>>> models;
				std::vector<std::pair<Handle<IAudioChunk>, Handle<IAudioChunk>>> sounds;
			};

			GameResources(Handle<IRenderer> renderer, Handle<IAudioDevice> audio);

			/**
			 * Loads `path` once and shares it. With a `skin` that has its own `path`,
			 * the skin's is loaded, once, and shared by that skin's players.
			 */
			Handle<IImage> RegisterImage(const std::string& path, Skin* skin = nullptr);
			Handle<IModel> RegisterModel(const std::string& path, Skin* skin = nullptr);
			Handle<IAudioChunk> RegisterSound(const std::string& path, Skin* skin = nullptr);

			/**
			 * A renderer that draws with the base one and loads through this set,
			 * seeing `skin` on top if there is one.
			 */
			Handle<IRenderer> CreateRenderer(Handle<Skin> skin = {});
			/**
			 * An audio device that plays with the base one and loads through this
			 * set, seeing `skin` on top if there is one.
			 */
			Handle<IAudioDevice> CreateAudioDevice(Handle<Skin> skin = {});
			/** Drops what was loaded from the skin `name`; its holders keep it. */
			void ForgetSkin(const std::string& name);

			std::vector<std::string> GetImageAndModelFiles() const;
			std::vector<std::string> GetSoundFiles() const;

			/**
			 * Loads every model, image and sound in `changes` the way `Register*`
			 * would: the ones this set holds are kept for `CommitReload`, and the
			 * others are only checked. Each file that fails goes to `errors`.
			 *
			 * A file no longer mounted is skipped: its holders keep what they have.
			 */
			std::unique_ptr<Reload> PrepareReload(const ResourceChanges& changes,
			                                      std::vector<ResourceLoadError>& errors);
			void CommitReload(Reload&);

			/** Reloads every held image and model, or every held sound, logging failures. */
			void ReloadGraphics();
			void ReloadSounds();

			/**
			 * Decodes the game asset at `path` without making anything of it, and
			 * raises if it doesn't load. Needs no renderer or audio device.
			 */
			static void CheckFile(const std::string& path);

			/** Changes with each `CommitReload`. */
			std::uint64_t GetGeneration() const { return generation; }

		protected:
			~GameResources();

		private:
			struct Cache {
				std::map<std::string, Handle<IImage>> images;
				std::map<std::string, Handle<IModel>> models;
				std::map<std::string, Handle<IAudioChunk>> sounds;
			};

			Handle<IRenderer> renderer;
			Handle<IAudioDevice> audio;
			Cache shared;
			// The files skins have their own of, by skin name.
			std::map<std::string, std::pair<Handle<Skin>, Cache>> skinned;
			std::uint64_t generation = 0;

			/** Where `path`, loaded for `skin`, is held. */
			Cache& GetCache(Skin* skin, const std::string& path);
			void ReloadAll(bool graphics, bool sounds);
		};
	} // namespace client
} // namespace spades
