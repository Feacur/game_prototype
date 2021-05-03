#if !defined(GAME_ASSET_FONT_INTERNAL)
#define GAME_ASSET_FONT_INTERNAL

#include "framework/common.h"

// @note: a weird way to decouple asset, image, glyphs?
struct Glyph_Params {
	int32_t rect[4]; // left, bottom, right, top
	float full_size_x;
	bool is_empty;
};

#endif
