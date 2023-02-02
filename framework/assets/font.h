#if !defined(FRAMEWORK_ASSETS_FONT)
#define FRAMEWORK_ASSETS_FONT

#include "framework/assets/glyph_params.h"

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

struct Font;
struct Image;
struct Typeface;

struct Glyph {
	struct Glyph_Params params;
	struct rect uv;
	uint32_t id;
	uint8_t gc_timeout;
};

struct Font * font_init(void);
void font_free(struct Font * font);

struct Typeface const * font_get_typeface(struct Font const * font, uint32_t codepoint);
void font_set_typeface(struct Font * font, struct Typeface const * typeface, uint32_t codepoint_from, uint32_t codepoint_to);

void font_add_glyph(struct Font * font, uint32_t codepoint, float size);
void font_add_defaults(struct Font * font, float size);
void font_render(struct Font * font);

struct Image const * font_get_asset(struct Font const * font);
struct Glyph const * font_get_glyph(struct Font * const font, uint32_t codepoint, float size);

float font_get_scale(struct Font const * font, float size);
float font_get_ascent(struct Font const * font, float scale);
float font_get_descent(struct Font const * font, float scale);
float font_get_gap(struct Font const * font, float scale);
float font_get_kerning(struct Font const * font, uint32_t codepoint1, uint32_t codepoint2, float scale);

#endif
