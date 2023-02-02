#if !defined(FRAMEWORK_ASSETS_GLYPH_PARAMS)
#define FRAMEWORK_ASSETS_GLYPH_PARAMS

#include "framework/vector_types.h"

// @purpose: decouple `typeface.h` and `glyph_atlas.h`

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
