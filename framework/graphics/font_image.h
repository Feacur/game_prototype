#if !defined(GAME_GRAPHICS_FONT_IMAGE)
#define GAME_GRAPHICS_FONT_IMAGE

#include "framework/assets/font_glyph.h"

// @todo: tune/expose glyphs GC

struct Font_Image;
struct Image;
struct Font;

struct Font_Glyph {
	struct Glyph_Params params;
	float uv[4];
	uint32_t id;
	uint8_t usage;
};

struct Font_Image * font_image_init(struct Font const * font);
void font_image_free(struct Font_Image * font_image);

void font_image_add_glyph_error(struct Font_Image *font_image, float size);
void font_image_add_glyphs_from_range(struct Font_Image * font_image, uint32_t from, uint32_t to, float size);
void font_image_add_glyphs_from_text(struct Font_Image * font_image, uint32_t length, uint8_t const * data, float size);
void font_image_add_kerning_from_range(struct Font_Image * font_image, uint32_t from, uint32_t to);
void font_image_add_kerning_from_text(struct Font_Image * font_image, uint32_t length, uint8_t const * data);
void font_image_add_kerning_all(struct Font_Image * font_image);
void font_image_render(struct Font_Image * font_image);

struct Image const * font_image_get_asset(struct Font_Image const * font_image);
struct Font_Glyph const * font_image_get_glyph(struct Font_Image * const font_image, uint32_t codepoint, float size);

float font_image_get_scale(struct Font_Image const * font_image, float size);
float font_image_get_ascent(struct Font_Image const * font_image, float scale);
float font_image_get_descent(struct Font_Image const * font_image, float scale);
float font_image_get_gap(struct Font_Image const * font_image, float scale);
float font_image_get_kerning(struct Font_Image const * font_image, uint32_t codepoint1, uint32_t codepoint2, float scale);

#endif
