/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.

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

#include "IAudioChunk.h"
#include <Core/Math.h>
#include <Core/RefCountedObject.h>

namespace spades {
	namespace client {
		class GameMap;

		struct AudioParam {
			float volume;
			float pitch;
			float referenceDistance;

			AudioParam() {
				volume = 1.0F;
				pitch = 1.0F;
				referenceDistance = 1.0F;
			}
		};

		class IAudioDevice : public RefCountedObject {
		public:
			virtual ~IAudioDevice() {}

			/** Loads `name` once; later calls share the chunk. */
			virtual Handle<IAudioChunk> RegisterSound(const char* name) = 0;

			/**
			 * Clear the cache of chunks loaded via `RegisterSound`. This method
			 * is merely a hint - the implementation may partially or completely
			 * ignore the request.
			 */
			virtual void ClearCache() {}

			/** The files the chunks loaded via `RegisterSound` come from. */
			virtual std::vector<std::string> GetRegisteredFiles() = 0;

			/** Loads `name` into a new chunk, outside the `RegisterSound` cache. */
			virtual Handle<IAudioChunk> CreateSound(const char* name) = 0;

			/**
			 * Gives `target` the sound of `source`, which gets the old one, so the
			 * holders of `target` play the new sound. Both come from this device.
			 */
			virtual void ReplaceSound(IAudioChunk& target, IAudioChunk& source) = 0;

			virtual void SetGameMap(GameMap*) = 0;

			virtual void Play(IAudioChunk*, const Vector3& origin, const AudioParam&) = 0;
			virtual void PlayLocal(IAudioChunk*, const Vector3& origin, const AudioParam&) = 0;
			virtual void PlayLocal(IAudioChunk*, const AudioParam&) = 0;

			virtual void Respatialize(const Vector3& eye, const Vector3& front,
			                          const Vector3& up) = 0;
		};
	} // namespace client
} // namespace spades