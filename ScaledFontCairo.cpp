/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ScaledFontCairo.h"
#include "Logging.h"

#include <string>
#include <vector>

#ifdef MOZ_ENABLE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H

#include "cairo-ft.h"
#endif

#if defined(USE_SKIA) && defined(MOZ_ENABLE_FREETYPE)
#include "skia/SkTypeface.h"
#include "skia/SkTypeface_cairo.h"
#endif

#ifdef USE_CAIRO
#include "PathCairo.h"
#endif

typedef struct FT_FaceRec_* FT_Face;

using namespace std;

namespace mozilla {
namespace gfx {

// On Linux and Android our "platform" font is a cairo_scaled_font_t and we use
// an SkFontHost implementation that allows Skia to render using this.
// This is mainly because FT_Face is not good for sharing between libraries, which
// is a requirement when we consider runtime switchable backends and so on
ScaledFontCairo::ScaledFontCairo(cairo_scaled_font_t* aScaledFont, Float aSize)
  : ScaledFontBase(aSize)
#ifdef MOZ_ENABLE_FREETYPE
  , mFTFace(nullptr)
#endif
{
  SetCairoScaledFont(aScaledFont);
}

ScaledFontCairo::ScaledFontCairo(const uint8_t* aData, uint32_t aFileSize, uint32_t aIndex, Float aSize)
  : ScaledFontBase(aSize)
{
#ifdef MOZ_ENABLE_FREETYPE
  FT_Error error = FT_New_Memory_Face(Factory::GetFreetypeLibrary(), aData, aFileSize, aIndex, &mFTFace);

  cairo_font_face_t *face = cairo_ft_font_face_create_for_ft_face(mFTFace, FT_LOAD_DEFAULT);
  
  InitScaledFontFromFace(face);
  
  cairo_font_face_destroy(face);
#else
  // Implement me!
  MOZ_ASSERT(false);
#endif
}

ScaledFontCairo::~ScaledFontCairo()
{
#ifdef MOZ_ENABLE_FREETYPE
  if (mFTFace) {
    FT_Done_Face(mFTFace);
  }
#endif
}

void
ScaledFontCairo::CopyGlyphsToBuilder(const GlyphBuffer &aBuffer, PathBuilder *aBuilder)
{
  PathBuilderCairo *builder = static_cast<PathBuilderCairo*>(aBuilder);

  // Convert our GlyphBuffer into an array of Cairo glyphs.
  RefPtr<CairoPathContext> context = builder->GetPathContext();
  vector<cairo_glyph_t> glyphs(aBuffer.mNumGlyphs);
  for (uint32_t i = 0; i < aBuffer.mNumGlyphs; ++i) {
    glyphs[i].index = aBuffer.mGlyphs[i].mIndex;
    glyphs[i].x = aBuffer.mGlyphs[i].mPosition.x;
    glyphs[i].y = aBuffer.mGlyphs[i].mPosition.y;
  }

  cairo_set_scaled_font(*context, mScaledFont);
  cairo_glyph_path(*context, &glyphs.front(), aBuffer.mNumGlyphs);
}

#if defined(USE_SKIA) && defined(MOZ_ENABLE_FREETYPE)
SkTypeface* ScaledFontCairo::GetSkTypeface()
{
  if (!mTypeface) {
    cairo_font_face_t* fontFace = cairo_scaled_font_get_font_face(mScaledFont);
    FT_Face face = cairo_ft_scaled_font_lock_face(mScaledFont);

    int style = SkTypeface::kNormal;

    if (face->style_flags & FT_STYLE_FLAG_ITALIC)
    style |= SkTypeface::kItalic;

    if (face->style_flags & FT_STYLE_FLAG_BOLD)
      style |= SkTypeface::kBold;

    bool isFixedWidth = face->face_flags & FT_FACE_FLAG_FIXED_WIDTH;
    cairo_ft_scaled_font_unlock_face(mScaledFont);

    mTypeface = SkCreateTypefaceFromCairoFont(fontFace, (SkTypeface::Style)style, isFixedWidth);
  }

  return mTypeface;
}
#endif

}
}
