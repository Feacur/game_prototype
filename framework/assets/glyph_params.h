#if !defined(FRAMEWORK_ASSETS_GLYPH_PARAMS)
#define FRAMEWORK_ASSETS_GLYPH_PARAMS

#include "framework/maths_types.h"

// @purpose: decouple `typeface.h` and `font.h`

// glyph layout
// +----------+
// |glyph  1,1|
// |          |
// |0,0       |
// +----------+

struct Glyph_Params {
	struct srect rect;
	float full_size_x;
	bool is_empty;
};

#endif
