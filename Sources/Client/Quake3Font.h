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

#pragma once

#include <vector>

#include "IFont.h"
#include "IImage.h"
#include <Core/Bitmap.h>
#include <Core/RefCountedObject.h>

#define PROP_SPACE_WIDTH -2

namespace spades {
	namespace client {
		class IRenderer;
		class Quake3Font : public IFont {
			enum GlyphType { Invalid = 0, Image, Space };
			struct GlyphInfo {
				GlyphType type;
				AABB2 imageRect;
				float advance;
				// Source cell in the font bitmap (pixels). Valid when type == Image.
				int srcX = 0, srcY = 0, srcW = 0;
				// Cell in the outline atlas, including the outline margin.
				AABB2 outlineRect;
			};

			IRenderer* renderer;
			IImage* tex;
			int glyphHeight;
			std::vector<GlyphInfo> glyphs;
			float spaceWidth;

			// Needed to build the outline atlas.
			Handle<Bitmap> source;
			Handle<IImage> outlineImage;

			float yMin, yMax;

			void BuildOutline();
			void DrawOutlined(const std::string&, Vector2 offset, float scale, Vector4 color);

		protected:
			~Quake3Font();

		public:
			Quake3Font(IRenderer*, IImage* texture, Bitmap* source, const int* map,
			           int glyphHeight, float spaceWidth, bool extended = false);

			Vector2 Measure(const std::string&) override;
			void Draw(const std::string&, Vector2 offset, float scale, Vector4 color) override;
			void DrawOutline(const std::string&, const Vector2& offset, float scale,
			                 const Vector4& color, const Vector4& outlineColor) override;
			void SetGlyphYRange(float yMin, float yMax);
		};
	} // namespace client
} // namespace spades
