#if !defined(FRAMEWORK_ASSETS_FONT_GLYPH)
#define FRAMEWORK_ASSETS_FONT_GLYPH

#include "framework/vector_types.h"

// @purpose: decouple `typeface.h` and `font_atlas.h`

// glyph layout
// +----------+
// |glyph  1,1|
// |          |
// |0,0       |
// +----------+

struct Font_Glyph_Params {
	struct srect rect;
	float full_size_x;
	bool is_empty;
};

#endif
