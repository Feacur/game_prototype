#if !defined(FRAMEWORK_ASSETS_GLYPH_ATLAS)
#define FRAMEWORK_ASSETS_GLYPH_ATLAS

#include "framework/assets/typeface_glyph_params.h"

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

struct Glyph_Atlas;
struct Image;
struct Typeface;

struct Typeface_Glyph {
	struct Typeface_Glyph_Params params;
	struct rect uv;
	uint32_t id;
	uint8_t gc_timeout;
};

struct Glyph_Atlas * glyph_atlas_init(void);
void glyph_atlas_free(struct Glyph_Atlas * glyph_atlas);

struct Typeface const * glyph_atlas_get_typeface(struct Glyph_Atlas const * glyph_atlas, uint32_t codepoint);
void glyph_atlas_set_typeface(struct Glyph_Atlas * glyph_atlas, struct Typeface const * typeface, uint32_t codepoint_from, uint32_t codepoint_to);

void glyph_atlas_add_glyph(struct Glyph_Atlas * glyph_atlas, uint32_t codepoint, float size);
void glyph_atlas_add_default_glyphs(struct Glyph_Atlas * glyph_atlas, float size);
void glyph_atlas_render(struct Glyph_Atlas * glyph_atlas);

struct Image const * glyph_atlas_get_asset(struct Glyph_Atlas const * glyph_atlas);
struct Typeface_Glyph const * glyph_atlas_get_glyph(struct Glyph_Atlas * const glyph_atlas, uint32_t codepoint, float size);

float glyph_atlas_get_scale(struct Glyph_Atlas const * glyph_atlas, float size);
float glyph_atlas_get_ascent(struct Glyph_Atlas const * glyph_atlas, float scale);
float glyph_atlas_get_descent(struct Glyph_Atlas const * glyph_atlas, float scale);
float glyph_atlas_get_gap(struct Glyph_Atlas const * glyph_atlas, float scale);
float glyph_atlas_get_kerning(struct Glyph_Atlas const * glyph_atlas, uint32_t codepoint1, uint32_t codepoint2, float scale);

#endif
