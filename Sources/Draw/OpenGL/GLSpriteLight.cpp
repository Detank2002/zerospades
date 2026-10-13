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

#include <algorithm>
#include <cmath>

#include "GLDynamicLight.h"
#include "GLSpriteLight.h"

namespace spades {
	namespace draw {
		void EvaluateSpriteDynamicLights(const std::vector<GLDynamicLight> &lights,
		                                 const Vector3 &center, float radius, Vector4 &dlR,
		                                 Vector4 &dlG, Vector4 &dlB) {
			dlR = MakeVector4(0, 0, 0, 0);
			dlG = MakeVector4(0, 0, 0, 0);
			dlB = MakeVector4(0, 0, 0, 0);

			for (const GLDynamicLight &l : lights) {
				const client::DynamicLightParam &dl = l.GetParam();
				float spotTan =
				  dl.type == client::DynamicLightTypeSpotlight ? tanf(dl.spotAngle * 0.5F) : 0.0F;

				Vector3 v = dl.origin - center;
				float effectiveRadius = radius + dl.radius;

				if (v.GetChebyshevLength() > effectiveRadius)
					continue;
				float distSqr = v.GetSquaredLength();
				if (distSqr > effectiveRadius * effectiveRadius)
					continue;
				float att = 1.0F - distSqr / (effectiveRadius * effectiveRadius);
				float unif;
				unif = 1.0F - distSqr / (radius * radius);
				unif = std::max(0.2F, unif);

				if (dl.type == client::DynamicLightTypeSpotlight) {
					float forward = Vector3::Dot(v, dl.spotAxis[2]);
					if (forward > radius) {
						continue;
					} else if (forward >= -radius) {
						att *= 0.5F - (forward / radius) * 0.5F;
					} else {
						float cx = Vector3::Dot(center - dl.origin, dl.spotAxis[0]);
						float cy = Vector3::Dot(center - dl.origin, dl.spotAxis[1]);
						float sq = sqrtf(cx * cx + cy * cy);
						float sprTan = radius / (-radius - forward);
						float eff = sprTan + spotTan;
						sq /= -forward;
						if (sq > eff)
							continue;
						if (sq > eff - spotTan)
							att *= (eff - sq) / spotTan;
					}
				}

				Vector4 final;

				if (unif < .9999f) {
					float directionalScale = (1.f - unif) / sqrtf(distSqr);
					Vector4 directional = {v.x * directionalScale, v.y * directionalScale,
					                       v.z * directionalScale, unif};
					directional *= att;
					final = directional;
				} else {
					final.x = 0.0F;
					final.y = 0.0F;
					final.z = 0.0F;
					final.w = 0.0F;
				}

				final.w += unif * att;

				dlR += final * dl.color.x;
				dlG += final * dl.color.y;
				dlB += final * dl.color.z;
			}
		}
	} // namespace draw
} // namespace spades
