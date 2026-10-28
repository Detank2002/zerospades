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
#include <cctype>
#include <initializer_list>
#include <limits>
#include <unordered_map>
#include <utility>

#include <zlib.h>

#include "PakOverlay.h"
#include "Debug.h"
#include "DynamicMemoryStream.h"
#include "FileManager.h"
#include "TMPUtils.h"
#include "ZipFileSystem.h"

namespace spades {
	namespace {
		PakOverlay::PakSet g_mods, g_game;

		PakOverlay::PakSet& GetLayerSet(PakOverlay::Layer layer) {
			return layer == PakOverlay::Layer::Mods ? g_mods : g_game;
		}

		bool IsPak(const std::string& name) {
			if (name.size() < 4)
				return false;
			std::string ext = name.substr(name.size() - 4);
			for (char& c : ext)
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return ext == ".pak" || ext == ".zip";
		}

		bool IsWeak(const std::string& file) {
			return ResourceChanges::Normalize(file).size() != file.size();
		}

		struct Winner {
			std::string file; // as the pak spells it
			std::string id;   // of the pak it is read from
			const PakOverlay::Pak* pak;
			bool weak;
		};
		using Winners = std::unordered_map<std::string, Winner>;

		// The pak each file of `layers` is read from, by normalized path. Like
		// FileManager, a plain file beats any `.weak` one, then the topmost wins.
		Winners GetWinners(std::initializer_list<const PakOverlay::PakSet*> layers) {
			Winners winners;
			for (const PakOverlay::PakSet* layer : layers) {
				for (const auto& pak : *layer) {
					for (const std::string& file : pak->GetFiles()) {
						bool weak = IsWeak(file);
						Winner& winner = winners[ResourceChanges::Normalize(file)];
						if (winner.pak && weak && !winner.weak)
							continue;
						winner = {file, pak->GetId(), pak.get(), weak};
					}
				}
			}
			return winners;
		}

		ResourceChanges Diff(const Winners& before, const Winners& after) {
			ResourceChanges changes;
			for (const auto& file : before) {
				auto it = after.find(file.first);
				if (it == after.end() || it->second.id != file.second.id)
					changes.Add(file.second.file);
			}
			for (const auto& file : after) {
				if (before.find(file.first) == before.end())
					changes.Add(file.second.file);
			}
			return changes;
		}

		std::uint32_t Checksum(const std::string& bytes) {
			uLong crc = crc32(0, nullptr, 0);
			const auto* data = reinterpret_cast<const Bytef*>(bytes.data());
			std::size_t left = bytes.size();
			while (left > 0) {
				auto n = static_cast<uInt>(
				  std::min<std::size_t>(left, std::numeric_limits<uInt>::max()));
				crc = crc32(crc, data, n);
				data += n;
				left -= n;
			}
			return static_cast<std::uint32_t>(crc);
		}
	} // namespace

	PakOverlay::Pak::Pak() : fs(nullptr) {}
	// `unmounted` goes before `stream`, which the file system reads from.
	PakOverlay::Pak::~Pak() {}

	std::vector<std::string> PakOverlay::Pak::GetFiles() const { return fs->GetAllFiles(); }
	IFileSystem* PakOverlay::Pak::GetFileSystem() const { return fs; }

	std::vector<std::string> PakOverlay::FindModPaks(const std::string& name) {
		SPADES_MARK_FUNCTION();

		// A mod is a pak in Mods/, or a folder of paks there.
		std::string path = std::string("Mods/") + name;
		if (name.empty() || name.find('/') != std::string::npos || name == "..")
			return {};
		if (IsPak(name))
			return FileManager::FileExists(path.c_str()) ? std::vector<std::string>{path}
			                                             : std::vector<std::string>{};

		std::vector<std::string> paks;
		for (const std::string& file : FileManager::EnumFiles(path.c_str())) {
			if (IsPak(file))
				paks.push_back(path + "/" + file);
		}
		std::sort(paks.begin(), paks.end());
		return paks;
	}

	std::shared_ptr<PakOverlay::Pak> PakOverlay::Load(const std::string& path,
	                                                  const std::string& owner,
	                                                  const std::string& stamp) {
		SPADES_MARK_FUNCTION();

		if (!stamp.empty()) {
			for (const PakSet* layer : {&g_mods, &g_game}) {
				for (const auto& pak : *layer) {
					if (pak->path == path && pak->stamp == stamp && pak->owner == owner)
						return pak;
				}
			}
		}

		std::string bytes = FileManager::ReadAllBytes(path.c_str());
		auto stream = stmp::make_unique<DynamicMemoryStream>();
		stream->Write(bytes.data(), bytes.size());
		stream->SetPosition(0);

		auto pak = std::make_shared<Pak>();
		pak->owner = owner;
		pak->path = path;
		pak->stamp = stamp;
		pak->id = path + "#" + std::to_string(Checksum(bytes));
		pak->fs = new ZipFileSystem(stream.get());
		pak->unmounted.reset(pak->fs);
		pak->stream = std::move(stream);
		return pak;
	}

	const PakOverlay::PakSet& PakOverlay::GetLayer(Layer layer) { return GetLayerSet(layer); }

	PakOverlay::Changes PakOverlay::SetLayer(Layer layer, PakSet paks) {
		SPADES_MARK_FUNCTION();

		Winners gameBefore = GetWinners({&g_mods, &g_game});
		Winners softwareBefore = GetWinners({&g_mods});

		// Take every pak down, then mount the layers again in order: the file
		// systems mounted later go on top.
		for (const PakSet* set : {&g_mods, &g_game}) {
			for (const auto& pak : *set) {
				if (!pak->unmounted)
					pak->unmounted = FileManager::RemoveFileSystem(pak->fs);
			}
		}
		GetLayerSet(layer) = std::move(paks);
		for (const auto& pak : g_mods) {
			if (pak->unmounted)
				FileManager::PrependFileSystem(pak->unmounted.release());
		}
		for (const auto& pak : g_game) {
			if (pak->unmounted)
				FileManager::PrependGameFileSystem(pak->unmounted.release());
		}

		Changes changes;
		changes.game = Diff(gameBefore, GetWinners({&g_mods, &g_game}));
		changes.software = Diff(softwareBefore, GetWinners({&g_mods}));
		return changes;
	}

	std::string PakOverlay::GetOwner(const std::string& path) {
		Winners winners = GetWinners({&g_mods, &g_game});
		auto it = winners.find(ResourceChanges::Normalize(path));
		return it == winners.end() ? std::string() : it->second.pak->owner;
	}
} // namespace spades
