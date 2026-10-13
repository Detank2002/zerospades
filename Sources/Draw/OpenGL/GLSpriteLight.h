/*
 Copyright (c) 2013 yvt, 2026 ZeroSpades developers.

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
 along with ZeroSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#pragma once

#include <vector>

#include <Core/Math.h>

namespace spades {
	namespace draw {
		class GLDynamicLight;

		/**
		 * The dynamic lights falling on the volume a sprite of `radius` at `center`
		 * stands for, as terms the sprite shaders evaluate at the normal `n` of a
		 * corner: `dot(vec4(n, 1.0), dlR)` for red, and so on. Zero where no light
		 * reaches it.
		 */
		void EvaluateSpriteDynamicLights(const std::vector<GLDynamicLight> &lights,
		                                 const Vector3 &center, float radius, Vector4 &dlR,
		                                 Vector4 &dlG, Vector4 &dlB);
	} // namespace draw
} // namespace spades
