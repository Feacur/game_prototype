#if !defined(GAME_ASSET_FONT)
#define GAME_ASSET_FONT

#include "framework/common.h"

struct Asset_Font;

struct Asset_Font * asset_font_init(char const * path);
void asset_font_free(struct Asset_Font * asset_font);

#endif
