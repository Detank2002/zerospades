/*
 Copyright (c) 2026 Francois ND
 based on code of OpenSpades (c) yvt 2013.

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

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <Core/Math.h>
#include <Core/TMPUtils.h>

namespace spades {
	namespace client {

		/**
		 * The content of a pie menu, in the shape the *Pie Menu* protocol extension
		 * (extension id 0x31, packet id 0x71) puts on the wire.
		 *
		 * The client's own rings are written in this same shape, so a menu the server
		 * sends and the one the client ships with are drawn, browsed and sent by the
		 * same code, and differ only in where they came from.
		 */

		constexpr int kPieSliceCount = 6;

		/** What the crosshair is on when the menu opens, as the bits of a pie's
		 * Contexts mask. A spectator only ever opens on `PieContextSpectator`, and a
		 * dead player on `PieContextDead`. */
		enum PieContext : std::uint8_t {
			PieContextBlock = 1 << 0,
			PieContextTeammate = 1 << 1,
			PieContextEnemy = 1 << 2,
			PieContextSpectator = 1 << 3,
			PieContextDead = 1 << 4,
			PieContextSky = 1 << 5,
		};

		/** Choosing a slice sends its Text as a chat line, whatever it holds. */
		struct PieSlice {
			/** Sent on global chat rather than team chat. */
			bool global = false;
			/** What it sends, before substitution. */
			std::string text;
			/** What it draws. Empty means the Text is drawn. */
			std::string label;
			/** Shown while the slice is highlighted, never sent. Empty means none. */
			std::string tooltip;

			const std::string& DisplayLabel() const { return label.empty() ? text : label; }
			/** Draws nothing and cannot be chosen, whatever its tooltip. */
			bool IsBlank() const { return text.empty() && label.empty(); }
		};

		struct Pie {
			std::uint8_t contexts = 0;
			/** The centre caption. Empty means none. */
			std::string theme;
			/** Slice 0 is at the top and the rest follow clockwise. */
			std::array<PieSlice, kPieSliceCount> slices;
		};

		bool operator==(const PieSlice&, const PieSlice&);
		bool operator==(const Pie&, const Pie&);
		inline bool operator!=(const PieSlice& a, const PieSlice& b) { return !(a == b); }
		inline bool operator!=(const Pie& a, const Pie& b) { return !(a == b); }

		struct PieMenu {
			std::vector<Pie> pies;

			/** The pies offered in a context, in the order they were defined. */
			std::vector<Pie> PiesFor(PieContext) const;
		};

		/** Where the camera looked when the menu opened, for `%b`, `%h` and `%r`. */
		struct PieAim {
			Vector3 origin = {0.0F, 0.0F, 0.0F};
			/** Of length 1. */
			Vector3 direction = {0.0F, 0.0F, 1.0F};
			/** The first solid block on the ray. */
			stmp::optional<IntVector3> block;
			/** Where the ray first meets a block or a player. */
			stmp::optional<Vector3> point;
		};

		/**
		 * Expands a slice's Text, once and without re-scanning the result.
		 *
		 * `%p` becomes the target as a command's player argument, `#` and its id, or
		 * nothing when there is no target. `%b` becomes the block aimed at, `x y z`,
		 * or nothing. `%h` becomes the point aimed at, `x y z`, or nothing. `%r`
		 * becomes the camera ray, `x y z dx dy dz`. `%%` becomes
		 * `%`. Any other `%` is left as it is.
		 */
		std::string ExpandPieText(const std::string& text, int targetPlayerId,
								  const PieAim& aim);

		/**
		 * Reads a Menu sub packet, from just after its sub packet id to the end of
		 * the packet. The menu is applied whole or not at all, so anything wrong
		 * anywhere refuses all of it: the result is empty and `error` says why.
		 */
		stmp::optional<PieMenu> ParsePieMenu(const std::string& data, std::string& error);

		/** The rings the client offers until a server sends its own. The rings that
		 * point at a place drop a Teamplay marker there with `/pingmark` when
		 * `canPing`, and only say their words otherwise. */
		const PieMenu& BuiltInPieMenu(bool canPing);
	} // namespace client
} // namespace spades
