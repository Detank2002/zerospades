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

#include <cmath>

#include "GLSpriteRenderer.h"
#include "GLImage.h"
#include "GLProgram.h"
#include "GLRenderer.h"
#include "GLSpriteLight.h"
#include "IGLDevice.h"
#include "../SW/SWFeatureLevel.h" // for fastRcp
#include <Core/Debug.h>

namespace spades {
	namespace draw {
		namespace {
			void Store4(float (&out)[4], const Vector4 &v) {
				out[0] = v.x;
				out[1] = v.y;
				out[2] = v.z;
				out[3] = v.w;
			}
		} // namespace


		GLSpriteRenderer::GLSpriteRenderer(GLRenderer &renderer)
		    : renderer(renderer),
		      device(renderer.GetGLDevice()),
		      settings(renderer.GetSettings()),
		      projectionViewMatrix("projectionViewMatrix"),
		      rightVector("rightVector"),
		      upVector("upVector"),
		      frontVector("frontVector"),
		      texture("mainTexture"),
		      viewMatrix("viewMatrix"),
		      fogDistance("fogDistance"),
		      fogColor("fogColor"),
		      viewOriginVector("viewOriginVector"),
		      positionAttribute("positionAttribute"),
		      spritePosAttribute("spritePosAttribute"),
		      colorAttribute("colorAttribute"),
		      dlRAttribute("dlRAttribute"),
		      dlGAttribute("dlGAttribute"),
		      dlBAttribute("dlBAttribute"),
		      scatteringAttribute("scatteringAttribute"),
		      daylight("daylight") {
			SPADES_MARK_FUNCTION();

			program = renderer.RegisterProgram("Shaders/OpenGL/Sprite.program");
		}

		GLSpriteRenderer::~GLSpriteRenderer() { SPADES_MARK_FUNCTION(); }

		void GLSpriteRenderer::Add(spades::draw::GLImage *img, spades::Vector3 center, float rad,
		                           float ang, Vector4 color) {
			SPADES_MARK_FUNCTION_DEBUG();
			Sprite spr;
			spr.image = img;
			spr.center = center;
			spr.radius = rad;
			spr.angle = ang;
			const bool emissive = color.x > color.w || color.y > color.w || color.z > color.w;
			if (settings.r_hdr) {
				// linearize color
				if (emissive) {
					// emissive material
					color.x *= color.x;
					color.y *= color.y;
					color.z *= color.z;
				} else {
					// scattering/absorptive material
					float rcp = fastRcp(color.w + 0.01F);
					color.x *= color.x * rcp;
					color.y *= color.y * rcp;
					color.z *= color.z * rcp;
				}
			}
			// A scattering one is lit at its corners by the daylight, which dims it as
			// it dims the world, and the dynamic lights (Sprite.vs)
			spr.scattering = !emissive;
			spr.color = color;
			sprites.push_back(spr);
		}

		void GLSpriteRenderer::Clear() {
			SPADES_MARK_FUNCTION();
			sprites.clear();
		}

		void GLSpriteRenderer::Render() {
			SPADES_MARK_FUNCTION();
			lastImage = NULL;

			// The dynamic lights on each scattering sprite, for its corners to be lit by
			for (Sprite &spr : sprites)
				if (spr.scattering)
					EvaluateSpriteDynamicLights(renderer.lights, spr.center, spr.radius,
					                            spr.dlR, spr.dlG, spr.dlB);

			program->Use();

			projectionViewMatrix(program);
			rightVector(program);
			upVector(program);
			texture(program);
			viewMatrix(program);
			fogDistance(program);
			fogColor(program);
			viewOriginVector(program);

			positionAttribute(program);
			spritePosAttribute(program);
			colorAttribute(program);
			dlRAttribute(program);
			dlGAttribute(program);
			dlBAttribute(program);
			scatteringAttribute(program);
			daylight(program);
			frontVector(program);

			projectionViewMatrix.SetValue(renderer.GetProjectionViewMatrix());
			viewMatrix.SetValue(renderer.GetViewMatrix());

			fogDistance.SetValue(renderer.GetFogDistance());

			const auto &viewOrigin = renderer.GetSceneDef().viewOrigin;
			viewOriginVector.SetValue(viewOrigin.x, viewOrigin.y, viewOrigin.z);

			Vector3 fogCol = renderer.GetFogColor();
			//fogCol *= fogCol; // linearize
			fogColor.SetValue(fogCol.x, fogCol.y, fogCol.z);

			const client::SceneDefinition &def = renderer.GetSceneDef();
			rightVector.SetValue(def.viewAxis[0].x, def.viewAxis[0].y, def.viewAxis[0].z);
			upVector.SetValue(def.viewAxis[1].x, def.viewAxis[1].y, def.viewAxis[1].z);
			frontVector.SetValue(def.viewAxis[2].x, def.viewAxis[2].y, def.viewAxis[2].z);
			texture.SetValue(0);
			daylight.SetValue(renderer.GetDaylight());

			device.ActiveTexture(0);

			device.EnableVertexAttribArray(positionAttribute(), true);
			device.EnableVertexAttribArray(spritePosAttribute(), true);
			device.EnableVertexAttribArray(colorAttribute(), true);
			device.EnableVertexAttribArray(dlRAttribute(), true);
			device.EnableVertexAttribArray(dlGAttribute(), true);
			device.EnableVertexAttribArray(dlBAttribute(), true);
			device.EnableVertexAttribArray(scatteringAttribute(), true);

			for (size_t i = 0; i < sprites.size(); i++) {
				Sprite &spr = sprites[i];
				if (spr.image != lastImage) {
					Flush();
					lastImage = spr.image;
					SPAssert(vertices.empty());
				}

				Vertex v;
				v.x = spr.center.x;
				v.y = spr.center.y;
				v.z = spr.center.z;
				v.radius = spr.radius;
				v.angle = spr.angle;
				v.r = spr.color.x;
				v.g = spr.color.y;
				v.b = spr.color.z;
				v.a = spr.color.w;
				Store4(v.dlR, spr.dlR);
				Store4(v.dlG, spr.dlG);
				Store4(v.dlB, spr.dlB);
				v.scattering = spr.scattering ? 1.0F : 0.0F;

				uint32_t idx = (uint32_t)vertices.size();
				v.sx = -1;
				v.sy = -1;
				vertices.push_back(v);
				v.sx = 1;
				v.sy = -1;
				vertices.push_back(v);
				v.sx = -1;
				v.sy = 1;
				vertices.push_back(v);
				v.sx = 1;
				v.sy = 1;
				vertices.push_back(v);

				indices.push_back(idx);
				indices.push_back(idx + 1);
				indices.push_back(idx + 2);
				indices.push_back(idx + 1);
				indices.push_back(idx + 3);
				indices.push_back(idx + 2);
			}

			Flush();

			device.EnableVertexAttribArray(positionAttribute(), false);
			device.EnableVertexAttribArray(spritePosAttribute(), false);
			device.EnableVertexAttribArray(colorAttribute(), false);
			device.EnableVertexAttribArray(dlRAttribute(), false);
			device.EnableVertexAttribArray(dlGAttribute(), false);
			device.EnableVertexAttribArray(dlBAttribute(), false);
			device.EnableVertexAttribArray(scatteringAttribute(), false);
		}

		void GLSpriteRenderer::Flush() {
			SPADES_MARK_FUNCTION_DEBUG();

			if (vertices.empty())
				return;

			device.VertexAttribPointer(positionAttribute(), 4, IGLDevice::FloatType, false,
			                           sizeof(Vertex), &(vertices[0].x));
			device.VertexAttribPointer(spritePosAttribute(), 4, IGLDevice::FloatType, false,
			                           sizeof(Vertex), &(vertices[0].sx));
			device.VertexAttribPointer(colorAttribute(), 4, IGLDevice::FloatType, false,
			                           sizeof(Vertex), &(vertices[0].r));
			device.VertexAttribPointer(dlRAttribute(), 4, IGLDevice::FloatType, false,
			                           sizeof(Vertex), vertices[0].dlR);
			device.VertexAttribPointer(dlGAttribute(), 4, IGLDevice::FloatType, false,
			                           sizeof(Vertex), vertices[0].dlG);
			device.VertexAttribPointer(dlBAttribute(), 4, IGLDevice::FloatType, false,
			                           sizeof(Vertex), vertices[0].dlB);
			device.VertexAttribPointer(scatteringAttribute(), 1, IGLDevice::FloatType, false,
			                           sizeof(Vertex), &(vertices[0].scattering));

			SPAssert(lastImage);
			lastImage->Bind(IGLDevice::Texture2D);

			device.DrawElements(IGLDevice::Triangles, static_cast<IGLDevice::Sizei>(indices.size()),
			                    IGLDevice::UnsignedInt, indices.data());

			vertices.clear();
			indices.clear();
		}
	} // namespace draw
} // namespace spades
