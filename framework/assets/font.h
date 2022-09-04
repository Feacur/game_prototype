#if !defined(FRAMEWORK_ASSETS_FONT)
#define FRAMEWORK_ASSETS_FONT

#include "font_glyph.h"

// @note: `font_init` takes memory ownership of the buffer

struct Font;
struct Buffer;

struct Font * font_init(struct Buffer * buffer);
void font_free(struct Font * font);

uint32_t font_get_glyph_id(struct Font const * font, uint32_t codepoint);
struct Glyph_Params font_get_glyph_parameters(struct Font const * font, uint32_t glyph_id, float scale);

void font_render_glyph(
	struct Font const * font,
	uint32_t glyph_id, float scale,
	uint8_t * buffer, uint32_t buffer_width,
	uint32_t glyph_size_x, uint32_t glyph_size_y,
	uint32_t offset_x, uint32_t offset_y
);

float font_get_scale(struct Font const * font, float size);
int32_t font_get_ascent(struct Font const * font);
int32_t font_get_descent(struct Font const * font);
int32_t font_get_gap(struct Font const * font);
int32_t font_get_kerning(struct Font const * font, uint32_t glyph_id1, uint32_t glyph_id2);

#endif
