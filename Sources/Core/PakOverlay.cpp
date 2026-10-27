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
		PakOverlay::PakSet g_layers[2]; // indexed by Layer, bottom first

		PakOverlay::PakSet& GetLayerSet(PakOverlay::Layer layer) {
			return g_layers[layer == PakOverlay::Layer::Mods ? 0 : 1];
		}

		struct Winner {
			std::string file; // as the pak spells it
			std::string id;   // of the pak it is read from
			const PakOverlay::Pak* pak;
		};

		// The pak each mounted file is read from (the topmost), by normalized path.
		std::unordered_map<std::string, Winner> GetWinners() {
			std::unordered_map<std::string, Winner> winners;
			for (const PakOverlay::PakSet& layer : g_layers) {
				for (const auto& pak : layer) {
					for (const std::string& file : pak->GetFiles())
						winners[ResourceChanges::Normalize(file)] = {file, pak->GetId(), pak.get()};
				}
			}
			return winners;
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

	std::shared_ptr<PakOverlay::Pak> PakOverlay::Load(const std::string& path,
	                                                  const std::string& owner) {
		SPADES_MARK_FUNCTION();

		std::string bytes = FileManager::ReadAllBytes(path.c_str());
		auto stream = stmp::make_unique<DynamicMemoryStream>();
		stream->Write(bytes.data(), bytes.size());
		stream->SetPosition(0);

		auto pak = std::make_shared<Pak>();
		pak->owner = owner;
		pak->id = path + "#" + std::to_string(Checksum(bytes));
		pak->fs = new ZipFileSystem(stream.get());
		pak->unmounted.reset(pak->fs);
		pak->stream = std::move(stream);
		return pak;
	}

	const PakOverlay::PakSet& PakOverlay::GetLayer(Layer layer) { return GetLayerSet(layer); }

	ResourceChanges PakOverlay::SetLayer(Layer layer, PakSet paks) {
		SPADES_MARK_FUNCTION();

		auto before = GetWinners();

		// Take every pak down, then mount the layers again in order: the file
		// systems mounted later go on top.
		for (const PakSet& set : g_layers) {
			for (const auto& pak : set) {
				if (!pak->unmounted)
					pak->unmounted = FileManager::RemoveFileSystem(pak->fs);
			}
		}
		GetLayerSet(layer) = std::move(paks);
		for (const PakSet& set : g_layers) {
			for (const auto& pak : set) {
				if (pak->unmounted)
					FileManager::PrependFileSystem(pak->unmounted.release());
			}
		}

		auto after = GetWinners();

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

	std::string PakOverlay::GetOwner(const std::string& path) {
		auto winners = GetWinners();
		auto it = winners.find(ResourceChanges::Normalize(path));
		return it == winners.end() ? std::string() : it->second.pak->owner;
	}
} // namespace spades
