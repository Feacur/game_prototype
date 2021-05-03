#if !defined(GAME_ASSET_FONT)
#define GAME_ASSET_FONT

#include "framework/common.h"

struct Font;
struct Glyph_Params;

struct Font * asset_font_init(char const * path);
void asset_font_free(struct Font * asset_font);

float asset_font_get_scale(struct Font * asset_font, float pixels_size);

uint32_t asset_font_get_glyph_id(struct Font * asset_font, uint32_t codepoint);
void asset_font_get_glyph_parameters(struct Font * asset_font, struct Glyph_Params * params, uint32_t glyph_id, float scale);
void asset_font_fill_buffer(
	struct Font * asset_font,
	uint8_t * buffer, uint32_t buffer_rect_width,
	uint32_t glyph_id, uint32_t glyph_size_x, uint32_t glyph_size_y, float scale
);

int32_t asset_font_get_height(struct Font * asset_font);
int32_t asset_font_get_gap(struct Font * asset_font);
int32_t asset_font_get_kerning(struct Font * asset_font, uint32_t glyph_id1, uint32_t glyph_id2);

#endif
