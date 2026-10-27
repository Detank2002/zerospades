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

#include <cctype>

#include "ResourceChanges.h"

namespace spades {
	namespace {
		bool StartsWith(const std::string& s, const char* prefix) {
			return s.compare(0, std::char_traits<char>::length(prefix), prefix) == 0;
		}

		bool EndsWith(const std::string& s, const char* suffix) {
			std::size_t n = std::char_traits<char>::length(suffix);
			return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
		}

		// FileManager serves `foo.weak` as `foo` when nothing has `foo` itself.
		std::string StripWeak(const std::string& path) {
			std::string out = path;
			if (out.size() >= 5) {
				std::string tail = out.substr(out.size() - 5);
				for (char& c : tail)
					c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				if (tail == ".weak")
					out.resize(out.size() - 5);
			}
			return out;
		}
	} // namespace

	std::string ResourceChanges::Normalize(const std::string& path) {
		std::string out = StripWeak(path);
		for (char& c : out) {
			if (c == '\\')
				c = '/';
			else
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		return out;
	}

	ResourceChanges::AssetKind ResourceChanges::GetAssetKind(const std::string& path) {
		std::string p = Normalize(path);

		if (StartsWith(p, "scripts/"))
			return AssetKind::Script;
		if (EndsWith(p, ".kv6"))
			return AssetKind::Model;
		if (EndsWith(p, ".meta.json"))
			return AssetKind::ModelMetadata;
		if (EndsWith(p, ".png") || EndsWith(p, ".jpg") || EndsWith(p, ".tif") ||
		    EndsWith(p, ".bmp") || EndsWith(p, ".tga"))
			return AssetKind::Image;
		if (EndsWith(p, ".opus") || EndsWith(p, ".ogg") || EndsWith(p, ".wav"))
			return AssetKind::Sound;
		return AssetKind::None;
	}

	void ResourceChanges::Add(const std::string& path) {
		// Zip files can list their folders too; those hold nothing to reload.
		if (path.empty() || path.back() == '/')
			return;
		paths.emplace(Normalize(path), StripWeak(path));
	}

	bool ResourceChanges::Contains(const std::string& path) const {
		return paths.count(Normalize(path)) != 0;
	}

	bool ResourceChanges::ContainsUnder(const std::string& dir) const {
		std::string prefix = Normalize(dir);
		if (!prefix.empty() && prefix.back() != '/')
			prefix += '/';
		for (const auto& path : paths) {
			if (StartsWith(path.first, prefix.c_str()))
				return true;
		}
		return false;
	}

	std::vector<std::string> ResourceChanges::GetPaths() const {
		std::vector<std::string> out;
		out.reserve(paths.size());
		for (const auto& path : paths)
			out.push_back(path.second);
		return out;
	}
} // namespace spades
