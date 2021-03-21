#if !defined(GAME_FONT_IMAGE)
#define GAME_FONT_IMAGE

#include "framework/common.h"

struct Font_Image;
struct Asset_Image;
struct Asset_Font;
struct Glyph_Params;

struct Font_Glyph {
	struct Glyph_Params params;
	uint32_t id;
	float uv[4];
};

struct Font_Image * font_image_init(struct Asset_Font * asset_font, uint32_t size, uint32_t size_x, uint32_t size_y);
void font_image_free(struct Font_Image * font_image);

void font_image_clear(struct Font_Image * font_image);
struct Asset_Image * font_image_get_asset(struct Font_Image * font_image);

void font_image_build(struct Font_Image * font_image, uint32_t const * codepoint_ranges);
struct Font_Glyph const * font_image_get_glyph(struct Font_Image * const font_image, uint32_t codepoint);

float font_image_get_height(struct Font_Image * font_image);
float font_image_get_gap(struct Font_Image * font_image);
float font_image_get_kerning(struct Font_Image * font_image, uint32_t id1, uint32_t id2);

#endif
