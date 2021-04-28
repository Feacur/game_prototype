#if !defined(GAME_GRAPHICS_FONT_IMAGE_GLYPH)
#define GAME_GRAPHICS_FONT_IMAGE_GLYPH

#include "framework/assets/asset_font_glyph.h"

// @note: a weird way to decouple asset, image, glyphs?
struct Font_Glyph {
	struct Glyph_Params params;
	uint32_t id;
	float uv[4];
};

#endif
