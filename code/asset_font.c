#include "memory.h"
#include "array_byte.h"

#include "platform_file.h"

#include <string.h>

// better to compile third-parties as separate units
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Weverything"
#endif

#define STBTT_malloc(size, user_data)  MEMORY_ALLOCATE_SIZE(size)
#define STBTT_free(pointer, user_data) MEMORY_FREE(pointer)

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#if defined(__clang__)
	#pragma clang diagnostic pop
#endif

//
#include "asset_font.h"

struct Asset_Font {
	struct Array_Byte file;
	stbtt_fontinfo font;
};

struct Asset_Font * asset_font_init(char const * path) {
	struct Asset_Font * asset_font = MEMORY_ALLOCATE(struct Asset_Font);

	platform_file_init(&asset_font->file, path);
	stbtt_InitFont(&asset_font->font, asset_font->file.data, stbtt_GetFontOffsetForIndex(asset_font->file.data, 0));

	int ascent; int descent; int line_gap;
	stbtt_GetFontVMetrics(&asset_font->font, &ascent, &descent, &line_gap);

	return asset_font;
}

void asset_font_free(struct Asset_Font * asset_font) {
	platform_file_free(&asset_font->file);
	memset(asset_font, 0, sizeof(*asset_font));
	MEMORY_FREE(asset_font);
}
