#if !defined(FRAMEWORK_ASSETS_TYPEFACE)
#define FRAMEWORK_ASSETS_TYPEFACE

#include "typeface_glyph_params.h"

// @note: `typeface_init` takes memory ownership of the buffer

struct Typeface;
struct Buffer;

struct Typeface * typeface_init(struct Buffer * buffer);
void typeface_free(struct Typeface * typeface);

uint32_t typeface_get_glyph_id(struct Typeface const * typeface, uint32_t codepoint);
struct Typeface_Glyph_Params typeface_get_glyph_parameters(struct Typeface const * typeface, uint32_t glyph_id, float scale);

void typeface_render_glyph(
	struct Typeface const * typeface,
	uint32_t glyph_id, float scale,
	uint8_t * buffer, uint32_t buffer_width,
	uint32_t glyph_size_x, uint32_t glyph_size_y,
	uint32_t offset_x, uint32_t offset_y
);

float typeface_get_scale(struct Typeface const * typeface, float size);
int32_t typeface_get_ascent(struct Typeface const * typeface);
int32_t typeface_get_descent(struct Typeface const * typeface);
int32_t typeface_get_gap(struct Typeface const * typeface);
int32_t typeface_get_kerning(struct Typeface const * typeface, uint32_t glyph_id1, uint32_t glyph_id2);

#endif
