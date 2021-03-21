#if !defined(GAME_ASSET_FONT)
#define GAME_ASSET_FONT

#include "framework/common.h"

struct Asset_Font;

struct Glyph_Params {
	uint32_t bmp_size_x, bmp_size_y;
	int32_t offset_x, offset_y;
	uint32_t size_x;
	bool is_empty;
};

struct Asset_Font * asset_font_init(char const * path);
void asset_font_free(struct Asset_Font * asset_font);

float asset_font_get_scale(struct Asset_Font * asset_font, float pixels_size);

uint32_t asset_font_get_glyph_id(struct Asset_Font * asset_font, uint32_t codepoint);
int32_t asset_font_get_kerning(struct Asset_Font * asset_font, uint32_t glyph_id1, uint32_t glyph_id2);
void asset_font_get_glyph_parameters(struct Asset_Font * asset_font, struct Glyph_Params * params, uint32_t glyph_id, float scale);
void asset_font_fill_buffer(
	struct Asset_Font * asset_font,
	uint8_t * buffer, uint32_t buffer_rect_width,
	uint32_t glyph_id, uint32_t glyph_size_x, uint32_t glyph_size_y, float scale
);

#endif
