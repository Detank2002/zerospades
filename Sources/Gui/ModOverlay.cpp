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

#include <algorithm>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Main.h"
#include "ModOverlay.h"
#include "ModsScreenHelper.h"
#include <Client/GameAssetReload.h>
#include <Client/GameResources.h>
#include <Client/IAudioDevice.h>
#include <Client/IRenderer.h>
#include <Core/Debug.h>
#include <Core/PakOverlay.h>
#include <Core/ResourceChanges.h>
#include <Core/Strings.h>
#include <Core/TMPUtils.h>
#include <ScriptBindings/ScriptManager.h>

namespace spades {
	namespace gui {
		namespace {
			using Layer = PakOverlay::Layer;

			std::vector<std::string> g_mountedMods;
			std::vector<ModFailure> g_startupFailures;

			void Remove(std::vector<std::string>& mods, const std::string& mod) {
				mods.erase(std::remove(mods.begin(), mods.end(), mod), mods.end());
			}

			void Remove(PakOverlay::PakSet& paks, const std::string& mod) {
				paks.erase(std::remove_if(paks.begin(), paks.end(),
				                          [&](const std::shared_ptr<PakOverlay::Pak>& pak) {
					                          return pak->GetOwner() == mod;
				                          }),
				           paks.end());
			}

			// Reads the paks of `mods`. A mod with a pak that can't be read goes to
			// `failures`, and out of `mods`.
			PakOverlay::PakSet LoadPaks(std::vector<std::string>& mods,
			                            std::vector<ModFailure>& failures) {
				PakOverlay::PakSet paks;
				std::map<std::string, std::string> broken; // mod -> why
				for (const auto& pak : ModsScreenHelper::GetModPaks(mods)) {
					if (broken.count(pak.mod))
						continue;
					try {
						paks.push_back(PakOverlay::Load(pak.path, pak.mod, pak.stamp));
						SPLog("Mod pak loaded (in memory): %s", pak.path.c_str());
					} catch (const std::exception& ex) {
						SPLog("Mod pak failed to load: %s: %s", pak.path.c_str(), ex.what());
						broken[pak.mod] = pak.path + ": " + ex.what();
					}
				}
				for (const auto& mod : broken) {
					Remove(paks, mod.first);
					Remove(mods, mod.first);
					failures.push_back({mod.first, mod.second});
				}
				return paks;
			}

			std::string DescribeError(const ResourceLoadError& error) {
				return error.path.empty() ? error.message : error.path + ": " + error.message;
			}
		} // namespace

		void ModOverlay::Mount() {
			SPADES_MARK_FUNCTION();

			// A --try-mod run applies the mod under test on top of the base paks only.
			if (g_tryMod)
				return;

			Handle<ModsScreenHelper> helper = Handle<ModsScreenHelper>::New();
			std::vector<std::string> mods = helper->GetEnabledMods();
			std::vector<ModFailure> failures;
			PakOverlay::PakSet paks = LoadPaks(mods, failures);
			PakOverlay::SetLayer(Layer::Mods, paks);

			// Decode every game asset the mods bring once, so a broken one disables
			// its mod now rather than failing in a game.
			std::map<std::string, std::string> broken; // mod -> why
			std::set<std::string> checked;
			for (const auto& pak : paks) {
				for (const std::string& file : pak->GetFiles()) {
					if (!ResourceChanges::IsGameAsset(file) ||
					    !checked.insert(ResourceChanges::Normalize(file)).second)
						continue;
					try {
						client::GameResources::CheckFile(file);
					} catch (const std::exception& ex) {
						std::string mod = PakOverlay::GetOwner(file);
						SPLog("Mod failed to load: %s: %s: %s", mod.c_str(), file.c_str(),
						      ex.what());
						broken.emplace(mod, file + ": " + ex.what());
					}
				}
			}
			if (!broken.empty()) {
				for (const auto& mod : broken) {
					Remove(paks, mod.first);
					Remove(mods, mod.first);
					failures.push_back({mod.first, mod.second});
				}
				PakOverlay::SetLayer(Layer::Mods, paks);
			}

			g_mountedMods = mods;
			if (!failures.empty()) {
				helper->SetEnabledMods(mods);
				for (ModFailure& failure : failures)
					g_startupFailures.push_back(std::move(failure));
			}
		}

		void ModOverlay::StartScripts() {
			SPADES_MARK_FUNCTION();

			while (true) {
				try {
					ScriptManager::GetInstance();
					return;
				} catch (const ScriptBuildError& ex) {
					// The mod the failing script comes from, or else the last enabled
					// mod with scripts.
					PakOverlay::PakSet paks = PakOverlay::GetLayer(Layer::Mods);
					std::string culprit =
					  ex.GetFile().empty() ? std::string() : PakOverlay::GetOwner(ex.GetFile());
					for (auto it = paks.rbegin(); it != paks.rend() && culprit.empty(); ++it) {
						for (const std::string& file : (*it)->GetFiles()) {
							if (ResourceChanges::GetAssetKind(file) ==
							    ResourceChanges::AssetKind::Script) {
								culprit = (*it)->GetOwner();
								break;
							}
						}
					}
					if (culprit.empty())
						throw;

					std::string reason = ex.what(); // names the script, if there is one
					SPLog("Mod failed to load: %s: %s", culprit.c_str(), reason.c_str());
					Remove(paks, culprit);
					PakOverlay::SetLayer(Layer::Mods, std::move(paks));
					Remove(g_mountedMods, culprit);
					Handle<ModsScreenHelper>::New()->SetEnabledMods(g_mountedMods);
					g_startupFailures.push_back({culprit, reason});
				}
			}
		}

		std::vector<ModFailure> ModOverlay::TakeStartupFailures() {
			std::vector<ModFailure> failures;
			failures.swap(g_startupFailures);
			return failures;
		}

		ModApplyReport ModOverlay::Apply(client::IRenderer& renderer, client::IAudioDevice& audio,
		                                 client::GameResources* game) {
			SPADES_MARK_FUNCTION();

			ModApplyReport report;
			if (g_tryMod) {
				report.result = ModApplyResult::Refused;
				report.error = _Tr("ModOverlay", "Mods can't be applied while trying a mod.");
				return report;
			}
			// The download replaces paks on disk while it runs.
			if (ModsScreenHelper::IsDownloading()) {
				report.result = ModApplyResult::Refused;
				report.error = _Tr("ModOverlay", "Wait for the mod download to finish.");
				return report;
			}

			Handle<ModsScreenHelper> helper = Handle<ModsScreenHelper>::New();
			std::vector<std::string> previous = g_mountedMods;
			std::vector<std::string> target = helper->GetEnabledMods();
			PakOverlay::PakSet mounted = PakOverlay::GetLayer(Layer::Mods);
			PakOverlay::PakSet paks = LoadPaks(target, report.failures);

			// Without a game, the changed files are loaded only to check them.
			Handle<client::GameResources> check;
			if (!game) {
				check = Handle<client::GameResources>::New(Handle<client::IRenderer>(renderer),
				                                           Handle<client::IAudioDevice>(audio));
				game = check.GetPointerOrNull();
			}

			// Each round leaves out at least one mod, so this ends.
			while (true) {
				PakOverlay::Changes changes = PakOverlay::SetLayer(Layer::Mods, paks);

				std::vector<ResourceLoadError> errors;
				auto reload =
				  stmp::make_unique<client::GameAssetReload>(*game, changes.game, errors);
				if (!errors.empty()) {
					// Blame the mods the failing files come from, while they are mounted.
					// Only a mod of this set can be left out: one the game loaded for
					// itself (GetOwner sees both layers) is not, and leaving out nothing
					// would loop forever.
					std::map<std::string, std::string> culprits;
					bool blameless = false;
					for (const ResourceLoadError& error : errors) {
						std::string mod = error.path.empty() ? "" : PakOverlay::GetOwner(error.path);
						if (std::find(target.begin(), target.end(), mod) == target.end())
							blameless = true;
						else
							culprits.emplace(mod, DescribeError(error));
					}

					reload.reset();
					PakOverlay::SetLayer(Layer::Mods, mounted);

					if (blameless || culprits.empty()) {
						helper->SetEnabledMods(previous);
						report.result = ModApplyResult::Refused;
						report.error = DescribeError(errors.front());
						SPLog("Mods not applied: %s", report.error.c_str());
						return report;
					}
					for (const auto& culprit : culprits) {
						SPLog("Mod failed to load: %s: %s", culprit.first.c_str(),
						      culprit.second.c_str());
						Remove(paks, culprit.first);
						Remove(target, culprit.first);
						report.failures.push_back({culprit.first, culprit.second});
					}
					continue;
				}

				// What the software loaded itself was built from the previous set, so
				// keep that mounted until the restart; the enabled set is the new one.
				if (!client::GameAssetReload::GetSoftwareChanges(changes.software, renderer, audio)
				       .empty()) {
					reload.reset();
					PakOverlay::SetLayer(Layer::Mods, mounted);
					helper->SetEnabledMods(target);
					report.result = ModApplyResult::NeedsRestart;
					return report;
				}

				reload->Commit();
				g_mountedMods = target;
				helper->SetEnabledMods(target);
				SPLog("Mods applied in place");
				report.result = ModApplyResult::Applied;
				return report;
			}
		}

		bool ModOverlay::HasPendingChanges() {
			return Handle<ModsScreenHelper>::New()->GetEnabledMods() != g_mountedMods;
		}

		std::string ModOverlay::Describe(const ModApplyReport& report) {
			std::string text;
			if (report.result == ModApplyResult::Refused)
				text = _Tr("ModOverlay", "The mods were not applied:") + "\n" + report.error;
			std::string failures = Describe(report.failures);
			if (!failures.empty())
				text += (text.empty() ? "" : "\n\n") + failures;
			return text;
		}

		std::string ModOverlay::Describe(const std::vector<ModFailure>& failures) {
			if (failures.empty())
				return {};
			std::string text = _Tr("ModOverlay", "These mods failed to load and were disabled:");
			for (const ModFailure& failure : failures)
				text += "\n- " + failure.mod + ": " + failure.reason;
			return text;
		}
	} // namespace gui
} // namespace spades
