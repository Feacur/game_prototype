#if !defined(GAME_GRAPHICS_FONT_IMAGE)
#define GAME_GRAPHICS_FONT_IMAGE

#include "framework/common.h"

struct Font_Image;
struct Image;
struct Font;
struct Glyph_Params;
struct Font_Glyph;

struct Font_Image * font_image_init(struct Font * asset_font, int32_t size);
void font_image_free(struct Font_Image * font_image);

void font_image_clear(struct Font_Image * font_image);
struct Image * font_image_get_asset(struct Font_Image * font_image);

void font_image_build(struct Font_Image * font_image, uint32_t ranges_count, uint32_t const * codepoint_ranges);
struct Font_Glyph const * font_image_get_glyph(struct Font_Image * const font_image, uint32_t codepoint);

float font_image_get_height(struct Font_Image * font_image);
float font_image_get_gap(struct Font_Image * font_image);
float font_image_get_kerning(struct Font_Image * font_image, uint32_t codepoint1, uint32_t codepoint2);

#endif
