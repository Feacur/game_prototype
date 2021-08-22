#if !defined(GAME_ASSETS_FONT)
#define GAME_ASSETS_FONT

#include "framework/common.h"

struct Font;
struct Glyph_Params;

struct Font * font_init(char const * path);
void font_free(struct Font * font);

uint32_t font_get_glyph_id(struct Font const * font, uint32_t codepoint);
void font_get_glyph_parameters(struct Font const * font, struct Glyph_Params * params, uint32_t glyph_id, float scale);
void font_fill_buffer(
	struct Font const * font,
	uint8_t * buffer, uint32_t buffer_rect_width,
	uint32_t glyph_id, uint32_t glyph_size_x, uint32_t glyph_size_y, float scale
);

float font_get_scale(struct Font const * font, float pixels_size);
int32_t font_get_height(struct Font const * font);
int32_t font_get_gap(struct Font const * font);
int32_t font_get_kerning(struct Font const * font, uint32_t glyph_id1, uint32_t glyph_id2);

#endif
