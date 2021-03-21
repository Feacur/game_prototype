#if !defined(GAME_FONT_IMAGE)
#define GAME_FONT_IMAGE

#include "framework/common.h"

struct Font_Image;
struct Asset_Image;
struct Asset_Font;

struct Font_Image * font_image_init(struct Asset_Font * asset_font, uint32_t size_x, uint32_t size_y);
void font_image_free(struct Font_Image * font_image);

void font_image_clear(struct Font_Image * font_image);
struct Asset_Image * font_image_get_asset(struct Font_Image * font_image);

void font_image_build(struct Font_Image * font_image);

#endif
