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
#include "IAudioDevice.h"
#include "IRenderer.h"
#include <Core/Bitmap.h>
#include <Core/Debug.h>
#include <Core/FileManager.h>
#include <Core/VoxelModel.h>
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

			// Images the renderer makes itself rather than load from a file.
			bool IsBuiltInImage(const std::string& path) {
				return (!path.empty() && path[0] == '*') || path == "Gfx/White.tga";
			}

			// Each held file, by its normalized path.
			template <class T>
			std::unordered_map<std::string, std::pair<const std::string, Handle<T>>*>
			IndexByNormalizedPath(std::map<std::string, Handle<T>>& held) {
				std::unordered_map<std::string, std::pair<const std::string, Handle<T>>*> index;
				for (auto& entry : held)
					index[ResourceChanges::Normalize(entry.first)] = &entry;
				return index;
			}

			// Loads `path` with `load`. If it is held, the new content goes to `out`
			// beside its target; otherwise it is dropped: that was only a check.
			template <class T, class Load>
			void Prepare(const std::string& path,
			             std::unordered_map<std::string,
			                                std::pair<const std::string, Handle<T>>*>& held,
			             std::vector<std::pair<Handle<T>, Handle<T>>>& out, Load load) {
				auto it = held.find(ResourceChanges::Normalize(path));
				if (it == held.end()) {
					load(path);
					return;
				}
				Handle<T> fresh = load(it->second->first);
				out.emplace_back(it->second->second, std::move(fresh));
			}

			/** Draws with a base renderer, and loads through a GameResources. */
			class GameRenderer : public IRenderer {
				Handle<IRenderer> base;
				Handle<GameResources> resources;

			protected:
				~GameRenderer() {}

			public:
				GameRenderer(Handle<IRenderer> base, Handle<GameResources> resources)
				    : base(std::move(base)), resources(std::move(resources)) {}

				void Init() override { base->Init(); }
				void Shutdown() override { base->Shutdown(); }

				Handle<IImage> RegisterImage(const char* filename) override {
					return resources->RegisterImage(filename);
				}
				Handle<IModel> RegisterModel(const char* filename) override {
					return resources->RegisterModel(filename);
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

			protected:
				~GameAudioDevice() {}

			public:
				GameAudioDevice(Handle<IAudioDevice> base, Handle<GameResources> resources)
				    : base(std::move(base)), resources(std::move(resources)) {}

				// Like the base devices, this hands out a reference the caller owns.
				IAudioChunk* RegisterSound(const char* name) override {
					return resources->RegisterSound(name).Unmanage();
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

		Handle<IImage> GameResources::RegisterImage(const std::string& path) {
			SPADES_MARK_FUNCTION();
			if (IsBuiltInImage(path))
				return renderer->RegisterImage(path.c_str());
			auto it = images.find(path);
			if (it != images.end())
				return it->second;
			Handle<IImage> image = LoadImage(*renderer, path);
			images[path] = image;
			return image;
		}

		Handle<IModel> GameResources::RegisterModel(const std::string& path) {
			SPADES_MARK_FUNCTION();
			auto it = models.find(path);
			if (it != models.end())
				return it->second;
			Handle<IModel> model = LoadModel(*renderer, path);
			models[path] = model;
			return model;
		}

		Handle<IAudioChunk> GameResources::RegisterSound(const std::string& path) {
			SPADES_MARK_FUNCTION();
			auto it = sounds.find(path);
			if (it != sounds.end())
				return it->second;
			Handle<IAudioChunk> sound = LoadSound(*audio, path);
			sounds[path] = sound;
			return sound;
		}

		Handle<IRenderer> GameResources::CreateRenderer() {
			return Handle<GameRenderer>::New(renderer, Handle<GameResources>(*this))
			  .Cast<IRenderer>();
		}

		Handle<IAudioDevice> GameResources::CreateAudioDevice() {
			return Handle<GameAudioDevice>::New(audio, Handle<GameResources>(*this))
			  .Cast<IAudioDevice>();
		}

		std::vector<std::string> GameResources::GetImageAndModelFiles() const {
			std::vector<std::string> files;
			for (const auto& entry : images)
				files.push_back(entry.first);
			for (const auto& entry : models)
				files.push_back(entry.first);
			return files;
		}

		std::vector<std::string> GameResources::GetSoundFiles() const {
			std::vector<std::string> files;
			for (const auto& entry : sounds)
				files.push_back(entry.first);
			return files;
		}

		std::unique_ptr<GameResources::Reload>
		GameResources::PrepareReload(const ResourceChanges& changes,
		                             std::vector<ResourceLoadError>& errors) {
			SPADES_MARK_FUNCTION();

			auto reload = stmp::make_unique<Reload>();
			auto heldImages = IndexByNormalizedPath(images);
			auto heldModels = IndexByNormalizedPath(models);
			auto heldSounds = IndexByNormalizedPath(sounds);
			std::set<std::string> modelsDone; // a model and its metadata can both change

			for (const std::string& path : changes.GetPaths()) {
				AssetKind kind = ResourceChanges::GetAssetKind(path);

				// A metadata change is a change of the model beside it.
				std::string file = path;
				if (kind == AssetKind::ModelMetadata) {
					file = path.substr(0, path.size() - 10) + ".kv6";
					kind = AssetKind::Model;
				}

				if (!FileManager::FileExists(file.c_str()))
					continue;
				if (kind == AssetKind::Model &&
				    !modelsDone.insert(ResourceChanges::Normalize(file)).second)
					continue;

				try {
					switch (kind) {
						case AssetKind::Image:
							Prepare(file, heldImages, reload->images,
							        [&](const std::string& p) { return LoadImage(*renderer, p); });
							break;
						case AssetKind::Model:
							Prepare(file, heldModels, reload->models,
							        [&](const std::string& p) { return LoadModel(*renderer, p); });
							break;
						case AssetKind::Sound:
							Prepare(file, heldSounds, reload->sounds,
							        [&](const std::string& p) { return LoadSound(*audio, p); });
							break;
						default: break; // scripts are ScriptManager's, the rest the software's
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

		void GameResources::ReloadGraphics() { ReloadAll(GetImageAndModelFiles()); }

		void GameResources::ReloadSounds() { ReloadAll(GetSoundFiles()); }

		void GameResources::ReloadAll(const std::vector<std::string>& files) {
			SPADES_MARK_FUNCTION();

			ResourceChanges changes;
			for (const std::string& file : files)
				changes.Add(file);
			std::vector<ResourceLoadError> errors;
			auto reload = PrepareReload(changes, errors);
			for (const ResourceLoadError& error : errors)
				SPLog("'%s' not reloaded: %s", error.path.c_str(), error.message.c_str());
			CommitReload(*reload);
		}
	} // namespace client
} // namespace spades
