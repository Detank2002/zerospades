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

#include <set>
#include <unordered_map>

#include "GameResources.h"
#include "Skin.h"
#include "IAudioDevice.h"
#include "IRenderer.h"
#include <Core/AudioStream.h>
#include <Core/Bitmap.h>
#include <Core/IAudioStream.h>
#include <Core/Debug.h>
#include <Core/FileManager.h>
#include <Core/VoxelModel.h>
#include <Core/TMPUtils.h>
#include <Core/VoxelModelLoader.h>

namespace spades {
	namespace client {
		namespace {
			using AssetKind = ResourceChanges::AssetKind;

			// The one way each kind of game asset is loaded.
			Handle<IImage> LoadImage(IRenderer& renderer, const std::string& path) {
				return renderer.CreateImage(*Bitmap::Load(path));
			}
			Handle<IModel> LoadModel(IRenderer& renderer, const std::string& path) {
				return renderer.CreateModel(*VoxelModelLoader::Load(path.c_str()));
			}
			Handle<IAudioChunk> LoadSound(IAudioDevice& audio, const std::string& path) {
				return audio.CreateSound(path.c_str());
			}

			// The game's reads while a file of `skin`, or of no skin, loads.
			class LoadScope {
				FileManager::GameScope game;
				std::unique_ptr<Skin::Scope> skin;

			public:
				explicit LoadScope(const Skin* s) {
					if (s)
						skin = stmp::make_unique<Skin::Scope>(*s);
				}
			};

			template <class T, class Load>
			Handle<T> Register(std::map<std::string, Handle<T>>& cache, const std::string& path,
			                   const Skin* skin, Load load) {
				auto it = cache.find(path);
				if (it != cache.end())
					return it->second;
				Handle<T> object;
				{
					LoadScope scope(skin);
					object = load(path);
				}
				cache[path] = object;
				return object;
			}

			// The held entry of `cache` for `path`, by normalized path, or null.
			template <class T>
			std::pair<const std::string, Handle<T>>* Find(std::map<std::string, Handle<T>>& cache,
			                                             const std::string& path) {
				std::string key = ResourceChanges::Normalize(path);
				for (auto& entry : cache) {
					if (ResourceChanges::Normalize(entry.first) == key)
						return &entry;
				}
				return nullptr;
			}

			/** Draws with a base renderer, and loads through a GameResources. */
			class GameRenderer : public IRenderer {
				Handle<IRenderer> base;
				Handle<GameResources> resources;
				Handle<Skin> skin;

			protected:
				~GameRenderer() {}

			public:
				GameRenderer(Handle<IRenderer> base, Handle<GameResources> resources,
				             Handle<Skin> skin)
				    : base(std::move(base)), resources(std::move(resources)), skin(std::move(skin)) {}

				void Init() override { base->Init(); }
				void Shutdown() override { base->Shutdown(); }

				Handle<IImage> RegisterImage(const char* filename) override {
					return resources->RegisterImage(filename, skin.GetPointerOrNull());
				}
				Handle<IModel> RegisterModel(const char* filename) override {
					return resources->RegisterModel(filename, skin.GetPointerOrNull());
				}
				void ClearCache() override { resources->ReloadGraphics(); }
				std::vector<std::string> GetRegisteredFiles() override {
					return resources->GetImageAndModelFiles();
				}

				Handle<IImage> CreateImage(Bitmap& bmp) override { return base->CreateImage(bmp); }
				Handle<IModel> CreateModel(VoxelModel& m) override { return base->CreateModel(m); }
				void ReplaceImage(IImage& target, IImage& source) override {
					base->ReplaceImage(target, source);
				}
				void ReplaceModel(IModel& target, IModel& source) override {
					base->ReplaceModel(target, source);
				}

				void SetGameMap(stmp::optional<GameMap&> map) override { base->SetGameMap(map); }
				void SetFogDistance(float d) override { base->SetFogDistance(d); }
				void SetFogColor(Vector3 c) override { base->SetFogColor(c); }

				void StartScene(const SceneDefinition& def) override { base->StartScene(def); }
				void AddLight(const DynamicLightParam& light) override { base->AddLight(light); }
				void RenderModel(IModel& model, const ModelRenderParam& p) override {
					base->RenderModel(model, p);
				}
				void AddDebugLine(Vector3 a, Vector3 b, Vector4 color) override {
					base->AddDebugLine(a, b, color);
				}
				void AddSprite(IImage& img, Vector3 center, float radius, float rotation) override {
					base->AddSprite(img, center, radius, rotation);
				}
				void AddLongSprite(IImage& img, Vector3 p1, Vector3 p2, float radius) override {
					base->AddLongSprite(img, p1, p2, radius);
				}
				void EndScene() override { base->EndScene(); }

				void MultiplyScreenColor(Vector3 c) override { base->MultiplyScreenColor(c); }
				void SetColor(Vector4 c) override { base->SetColor(c); }
				void SetColorAlphaPremultiplied(Vector4 c) override {
					base->SetColorAlphaPremultiplied(c);
				}

				void DrawImage(stmp::optional<IImage&> img, const Vector2& outTopLeft) override {
					base->DrawImage(img, outTopLeft);
				}
				void DrawImage(stmp::optional<IImage&> img, const AABB2& outRect) override {
					base->DrawImage(img, outRect);
				}
				void DrawImage(stmp::optional<IImage&> img, const Vector2& outTopLeft,
				               const AABB2& inRect) override {
					base->DrawImage(img, outTopLeft, inRect);
				}
				void DrawImage(stmp::optional<IImage&> img, const AABB2& outRect,
				               const AABB2& inRect) override {
					base->DrawImage(img, outRect, inRect);
				}
				void DrawImage(stmp::optional<IImage&> img, const Vector2& outTopLeft,
				               const Vector2& outTopRight, const Vector2& outBottomLeft,
				               const AABB2& inRect) override {
					base->DrawImage(img, outTopLeft, outTopRight, outBottomLeft, inRect);
				}

				void DrawFlatGameMap(const AABB2& outRect, const AABB2& inRect) override {
					base->DrawFlatGameMap(outRect, inRect);
				}
				void DrawFlatGameMap(const Vector2& outTopLeft, const Vector2& outTopRight,
				                     const Vector2& outBottomLeft, const AABB2& inRect) override {
					base->DrawFlatGameMap(outTopLeft, outTopRight, outBottomLeft, inRect);
				}

				void BeginClippingCircle(const Vector2& center, float radius) override {
					base->BeginClippingCircle(center, radius);
				}
				void EndClippingCircle() override { base->EndClippingCircle(); }
				void BeginClippingRect(const AABB2& outRect) override {
					base->BeginClippingRect(outRect);
				}
				void EndClippingRect() override { base->EndClippingRect(); }

				bool IsRendererSW() const override { return base->IsRendererSW(); }

				void FrameDone() override { base->FrameDone(); }
				void Flip() override { base->Flip(); }
				Handle<Bitmap> ReadBitmap() override { return base->ReadBitmap(); }

				float ScreenWidth() override { return base->ScreenWidth(); }
				float ScreenHeight() override { return base->ScreenHeight(); }

				void DrawFilledRectFade(float x0, float y0, float x1, float y1, Vector4 color0,
				                        Vector4 color1, bool horizontal) override {
					base->DrawFilledRectFade(x0, y0, x1, y1, color0, color1, horizontal);
				}
				void DrawFilledTriangle(const Vector2& v0, const Vector2& v1,
				                        const Vector2& v2) override {
					base->DrawFilledTriangle(v0, v1, v2);
				}
			};

			/** Plays with a base audio device, and loads through a GameResources. */
			class GameAudioDevice : public IAudioDevice {
				Handle<IAudioDevice> base;
				Handle<GameResources> resources;
				Handle<Skin> skin;

			protected:
				~GameAudioDevice() {}

			public:
				GameAudioDevice(Handle<IAudioDevice> base, Handle<GameResources> resources,
				                Handle<Skin> skin)
				    : base(std::move(base)), resources(std::move(resources)), skin(std::move(skin)) {}

				Handle<IAudioChunk> RegisterSound(const char* name) override {
					return resources->RegisterSound(name, skin.GetPointerOrNull());
				}
				void ClearCache() override { resources->ReloadSounds(); }
				std::vector<std::string> GetRegisteredFiles() override {
					return resources->GetSoundFiles();
				}
				Handle<IAudioChunk> CreateSound(const char* name) override {
					return base->CreateSound(name);
				}
				void ReplaceSound(IAudioChunk& target, IAudioChunk& source) override {
					base->ReplaceSound(target, source);
				}

				void SetGameMap(GameMap* map) override { base->SetGameMap(map); }

				void Play(IAudioChunk* chunk, const Vector3& origin,
				          const AudioParam& param) override {
					base->Play(chunk, origin, param);
				}
				void PlayLocal(IAudioChunk* chunk, const Vector3& origin,
				               const AudioParam& param) override {
					base->PlayLocal(chunk, origin, param);
				}
				void PlayLocal(IAudioChunk* chunk, const AudioParam& param) override {
					base->PlayLocal(chunk, param);
				}
				void Respatialize(const Vector3& eye, const Vector3& front,
				                  const Vector3& up) override {
					base->Respatialize(eye, front, up);
				}
			};
		} // namespace

		GameResources::GameResources(Handle<IRenderer> renderer, Handle<IAudioDevice> audio)
		    : renderer(std::move(renderer)), audio(std::move(audio)) {}

		GameResources::~GameResources() {}

		GameResources::Cache& GameResources::GetCache(Skin* skin, const std::string& path) {
			if (!skin || !skin->Has(path))
				return shared;
			auto& entry = skinned[skin];
			if (!entry.first)
				entry.first = Handle<Skin>(*skin);
			return entry.second;
		}

		Handle<IImage> GameResources::RegisterImage(const std::string& path, Skin* skin) {
			SPADES_MARK_FUNCTION();
			Cache& cache = GetCache(skin, path);
			const Skin* from = &cache == &shared ? nullptr : skin;
			return Register(cache.images, path, from, [&](const std::string& p) {
				// Not a file: one the renderer makes itself, such as "*white".
				if (!FileManager::FileExists(p.c_str()))
					return renderer->RegisterImage(p.c_str());
				return LoadImage(*renderer, p);
			});
		}

		Handle<IModel> GameResources::RegisterModel(const std::string& path, Skin* skin) {
			SPADES_MARK_FUNCTION();
			Cache& cache = GetCache(skin, path);
			const Skin* from = &cache == &shared ? nullptr : skin;
			return Register(cache.models, path, from,
			                [&](const std::string& p) { return LoadModel(*renderer, p); });
		}

		Handle<IAudioChunk> GameResources::RegisterSound(const std::string& path, Skin* skin) {
			SPADES_MARK_FUNCTION();
			Cache& cache = GetCache(skin, path);
			const Skin* from = &cache == &shared ? nullptr : skin;
			return Register(cache.sounds, path, from,
			                [&](const std::string& p) { return LoadSound(*audio, p); });
		}

		Handle<IRenderer> GameResources::CreateRenderer(Handle<Skin> skin) {
			return Handle<GameRenderer>::New(renderer, Handle<GameResources>(*this), std::move(skin))
			  .Cast<IRenderer>();
		}

		Handle<IAudioDevice> GameResources::CreateAudioDevice(Handle<Skin> skin) {
			return Handle<GameAudioDevice>::New(audio, Handle<GameResources>(*this),
			                                    std::move(skin))
			  .Cast<IAudioDevice>();
		}

		void GameResources::ForgetSkin(const Skin& skin) { skinned.erase(&skin); }

		std::vector<std::string> GameResources::GetImageAndModelFiles() const {
			std::vector<std::string> files;
			for (const auto& entry : shared.images)
				files.push_back(entry.first);
			for (const auto& entry : shared.models)
				files.push_back(entry.first);
			return files;
		}

		std::vector<std::string> GameResources::GetSoundFiles() const {
			std::vector<std::string> files;
			for (const auto& entry : shared.sounds)
				files.push_back(entry.first);
			return files;
		}

		std::unique_ptr<GameResources::Reload>
		GameResources::PrepareReload(const ResourceChanges& changes,
		                             std::vector<ResourceLoadError>& errors) {
			SPADES_MARK_FUNCTION();

			// Every cache, with the skin its files load through.
			std::vector<std::pair<Cache*, const Skin*>> caches{{&shared, nullptr}};
			for (auto& entry : skinned)
				caches.emplace_back(&entry.second.second, entry.second.first.GetPointerOrNull());

			auto reload = stmp::make_unique<Reload>();
			std::set<std::string> modelsDone; // a model and its metadata can both change

			for (const std::string& path : changes.GetPaths()) {
				AssetKind kind = ResourceChanges::GetAssetKind(path);
				if (kind != AssetKind::Image && kind != AssetKind::Model &&
				    kind != AssetKind::ModelMetadata && kind != AssetKind::Sound)
					continue; // scripts are ScriptManager's, the rest the software's

				// A metadata change is a change of the model beside it.
				std::string file = path;
				if (kind == AssetKind::ModelMetadata) {
					file = path.substr(0, path.size() - 10) + ".kv6";
					kind = AssetKind::Model;
				}
				if (kind == AssetKind::Model &&
				    !modelsDone.insert(ResourceChanges::Normalize(file)).second)
					continue;

				try {
					bool held = false;
					for (auto& cache : caches) {
						LoadScope scope(cache.second);
						if (kind == AssetKind::Image) {
							if (auto* entry = Find(cache.first->images, file)) {
								held = true;
								if (FileManager::FileExists(entry->first.c_str()))
									reload->images.emplace_back(
									  entry->second, LoadImage(*renderer, entry->first));
							}
						} else if (kind == AssetKind::Model) {
							if (auto* entry = Find(cache.first->models, file)) {
								held = true;
								if (FileManager::FileExists(entry->first.c_str()))
									reload->models.emplace_back(
									  entry->second, LoadModel(*renderer, entry->first));
							}
						} else if (auto* entry = Find(cache.first->sounds, file)) {
							held = true;
							if (FileManager::FileExists(entry->first.c_str()))
								reload->sounds.emplace_back(entry->second,
								                            LoadSound(*audio, entry->first));
						}
					}

					// Held nowhere: only check it loads. A file gone from every pak
					// is skipped, and its holders keep what they have.
					if (!held) {
						LoadScope scope(nullptr);
						if (FileManager::FileExists(file.c_str()))
							CheckFile(file);
					}
				} catch (const std::exception& ex) {
					errors.push_back({path, ex.what()});
				}
			}
			return reload;
		}

		void GameResources::CommitReload(Reload& reload) {
			SPADES_MARK_FUNCTION();

			for (auto& image : reload.images)
				renderer->ReplaceImage(*image.first, *image.second);
			for (auto& model : reload.models)
				renderer->ReplaceModel(*model.first, *model.second);
			for (auto& sound : reload.sounds)
				audio->ReplaceSound(*sound.first, *sound.second);
			generation++;
		}

		void GameResources::CheckFile(const std::string& path) {
			SPADES_MARK_FUNCTION();

			switch (ResourceChanges::GetAssetKind(path)) {
				case AssetKind::Image: Bitmap::Load(path); break;
				case AssetKind::Model: VoxelModelLoader::Load(path.c_str()); break;
				case AssetKind::ModelMetadata: {
					std::string model = path.substr(0, path.size() - 10) + ".kv6";
					if (FileManager::FileExists(model.c_str()))
						VoxelModelLoader::Load(model.c_str());
					break;
				}
				case AssetKind::Sound: {
					// The header is enough to tell a sound from garbage.
					std::unique_ptr<IAudioStream> stream(OpenAudioStream(path));
					break;
				}
				default: break;
			}
		}

		void GameResources::ReloadGraphics() { ReloadAll(true, false); }

		void GameResources::ReloadSounds() { ReloadAll(false, true); }

		void GameResources::ReloadAll(bool graphics, bool sounds) {
			SPADES_MARK_FUNCTION();

			ResourceChanges changes;
			auto add = [&](const Cache& cache) {
				if (graphics) {
					for (const auto& entry : cache.images)
						changes.Add(entry.first);
					for (const auto& entry : cache.models)
						changes.Add(entry.first);
				}
				if (sounds) {
					for (const auto& entry : cache.sounds)
						changes.Add(entry.first);
				}
			};
			add(shared);
			for (const auto& entry : skinned)
				add(entry.second.second);
			std::vector<ResourceLoadError> errors;
			auto reload = PrepareReload(changes, errors);
			for (const ResourceLoadError& error : errors)
				SPLog("'%s' not reloaded: %s", error.path.c_str(), error.message.c_str());
			CommitReload(*reload);
		}
	} // namespace client
} // namespace spades
