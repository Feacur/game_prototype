#if !defined(FRAMEWORK_ASSETS_TYPEFACE_GLYPH)
#define FRAMEWORK_ASSETS_TYPEFACE_GLYPH

#include "framework/vector_types.h"

// @purpose: decouple `typeface.h` and `font_atlas.h`

// glyph layout
// +----------+
// |glyph  1,1|
// |          |
// |0,0       |
// +----------+

struct Typeface_Glyph_Params {
	struct srect rect;
	float full_size_x;
	bool is_empty;
};

#endif
