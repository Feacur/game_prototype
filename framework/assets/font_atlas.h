#if !defined(FRAMEWORK_ASSETS_FONT_ATLAS)
#define FRAMEWORK_ASSETS_FONT_ATLAS

#include "framework/assets/font_glyph_params.h"

// @todo: tune/expose glyphs GC

// atlas and glyphs layout
// +----------------+
// |atlas        1,1|
// |  +----------+  |
// |  |glyph  1,1|  |
// |  |          |  |
// |  |0,0       |  |
// |  +----------+  |
// |0,0             |
// +----------------+

struct Font_Atlas;
struct Image;
struct Font;

struct Font_Glyph {
	struct Font_Glyph_Params params;
	struct rect uv;
	uint32_t id;
	uint8_t usage;
};

struct Font_Atlas * font_atlas_init(void);
void font_atlas_free(struct Font_Atlas * font_atlas);

struct Font const * font_atlas_get_font(struct Font_Atlas const * font_atlas, uint32_t codepoint);
void font_atlas_set_font(struct Font_Atlas * font_atlas, struct Font const * font, uint32_t from, uint32_t to);

void font_atlas_add_glyph(struct Font_Atlas * font_atlas, uint32_t codepoint, float size);
void font_atlas_add_default_glyphs(struct Font_Atlas * font_atlas, float size);
void font_atlas_render(struct Font_Atlas * font_atlas);

struct Image const * font_atlas_get_asset(struct Font_Atlas const * font_atlas);
struct Font_Glyph const * font_atlas_get_glyph(struct Font_Atlas * const font_atlas, uint32_t codepoint, float size);

float font_atlas_get_scale(struct Font_Atlas const * font_atlas, float size);
float font_atlas_get_ascent(struct Font_Atlas const * font_atlas, float scale);
float font_atlas_get_descent(struct Font_Atlas const * font_atlas, float scale);
float font_atlas_get_gap(struct Font_Atlas const * font_atlas, float scale);
float font_atlas_get_kerning(struct Font_Atlas const * font_atlas, uint32_t codepoint1, uint32_t codepoint2, float scale);

#endif
