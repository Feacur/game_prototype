#include "framework/memory.h"
#include "framework/containers/array_byte.h"

#include "framework/platform_file.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

// better to compile third-parties as separate units
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Weverything"
#elif defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#define STBTT_malloc(size, user_data)  MEMORY_ALLOCATE_SIZE(size)
#define STBTT_free(pointer, user_data) MEMORY_FREE(pointer)

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#if defined(__clang__)
	#pragma clang diagnostic pop
#elif defined(_MSC_VER)
	#pragma warning(pop)
#endif

//
#include "asset_font.h"

struct Asset_Font {
	struct Array_Byte file;
	stbtt_fontinfo font;
	int ascent, descent, line_gap;
};

struct Asset_Font * asset_font_init(char const * path) {
	struct Asset_Font * asset_font = MEMORY_ALLOCATE(struct Asset_Font);

	platform_file_init(&asset_font->file, path);
	stbtt_InitFont(&asset_font->font, asset_font->file.data, stbtt_GetFontOffsetForIndex(asset_font->file.data, 0));

	stbtt_GetFontVMetrics(&asset_font->font, &asset_font->ascent, &asset_font->descent, &asset_font->line_gap);

	return asset_font;
}

void asset_font_free(struct Asset_Font * asset_font) {
	platform_file_free(&asset_font->file);
	memset(asset_font, 0, sizeof(*asset_font));
	MEMORY_FREE(asset_font);
}

float asset_font_get_scale(struct Asset_Font * asset_font, float pixels_size) {
	return stbtt_ScaleForMappingEmToPixels(&asset_font->font, pixels_size);
}

uint32_t asset_font_get_glyph_id(struct Asset_Font * asset_font, uint32_t codepoint) {
	return (uint32_t)stbtt_FindGlyphIndex(&asset_font->font, (int)codepoint);
}

uint32_t asset_font_get_kerning(struct Asset_Font * asset_font, uint32_t id1, uint32_t id2) {
	int kerning = stbtt_GetGlyphKernAdvance(&asset_font->font, (int)id1, (int)id2);
	return (uint32_t)kerning;
}

void asset_font_get_glyph_parameters(struct Asset_Font * asset_font, struct Glyph_Params * params, uint32_t id) {
	int advance_width, left_side_bearing;
	stbtt_GetGlyphHMetrics(&asset_font->font, (int)id, &advance_width, &left_side_bearing);

	int rect[4];
	stbtt_GetGlyphBox(&asset_font->font, (int)id, rect + 0, rect + 1, rect + 2, rect + 3);

	params->bmp_size_x = (uint32_t)(rect[2] - rect[0]);
	params->bmp_size_y = (uint32_t)(rect[3] - rect[1]);
	params->offset_x = (int32_t)left_side_bearing;
	params->offset_y = (int32_t)rect[1];
	params->size_x = (uint32_t)advance_width;
	params->is_empty = stbtt_IsGlyphEmpty(&asset_font->font, (int)id);
}

void asset_font_fill_buffer(
	struct Asset_Font * asset_font,
	struct Array_Byte * buffer, uint32_t stride,
	uint32_t id,
	uint32_t size_x, uint32_t size_y, float scale
) {
	stbtt_MakeGlyphBitmap(
		&asset_font->font, buffer->data,
		(int)size_x, (int)size_y, (int)stride,
		scale, scale,
		(int)id
	);
}
