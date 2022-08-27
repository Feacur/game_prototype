#if !defined(FRAMEWORK_ASSETS_FONT_GLYPH)
#define FRAMEWORK_ASSETS_FONT_GLYPH

#include "framework/vector_types.h"

// @note: a weird way to decouple asset, image, glyphs?
struct Glyph_Params {
	struct srect rect;
	float full_size_x;
	bool is_empty;
};

#endif
