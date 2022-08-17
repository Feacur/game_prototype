#if !defined(FRAMEWORK_ASSETS_FONT_ATLAS)
#define FRAMEWORK_ASSETS_FONT_ATLAS

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

struct Font_Image * font_atlas_init(struct Font const * font);
void font_atlas_free(struct Font_Image * font_atlas);

void font_atlas_add_glyph_error(struct Font_Image *font_atlas, float size);
void font_atlas_add_glyphs_from_range(struct Font_Image * font_atlas, uint32_t from, uint32_t to, float size);
void font_atlas_add_glyphs_from_text(struct Font_Image * font_atlas, uint32_t length, uint8_t const * data, float size);
void font_atlas_render(struct Font_Image * font_atlas);

struct Image const * font_atlas_get_asset(struct Font_Image const * font_atlas);
struct Font_Glyph const * font_atlas_get_glyph(struct Font_Image * const font_atlas, uint32_t codepoint, float size);

float font_atlas_get_scale(struct Font_Image const * font_atlas, float size);
float font_atlas_get_ascent(struct Font_Image const * font_atlas, float scale);
float font_atlas_get_descent(struct Font_Image const * font_atlas, float scale);
float font_atlas_get_gap(struct Font_Image const * font_atlas, float scale);
float font_atlas_get_kerning(struct Font_Image const * font_atlas, uint32_t codepoint1, uint32_t codepoint2, float scale);

#endif
