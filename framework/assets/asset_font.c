#include "framework/memory.h"
#include "framework/containers/array_byte.h"

#include "framework/platform_file.h"

#include <stdio.h>
#include <stdlib.h>
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

// @idea: cache here basic metrics, without scaling?
struct Asset_Font {
	struct Array_Byte file;
	stbtt_fontinfo font;
	int ascent, descent, line_gap;
};

struct Asset_Font * asset_font_init(char const * path) {
	struct Asset_Font * asset_font = MEMORY_ALLOCATE(struct Asset_Font);

	platform_file_read(path, &asset_font->file);
	if (!stbtt_InitFont(&asset_font->font, asset_font->file.data, stbtt_GetFontOffsetForIndex(asset_font->file.data, 0))) {
		fprintf(stderr, "'stbtt_InitFont' failed\n"); DEBUG_BREAK();
	}

	stbtt_GetFontVMetrics(&asset_font->font, &asset_font->ascent, &asset_font->descent, &asset_font->line_gap);

	return asset_font;
}

void asset_font_free(struct Asset_Font * asset_font) {
	array_byte_free(&asset_font->file);
	memset(asset_font, 0, sizeof(*asset_font));
	MEMORY_FREE(asset_font);
}

float asset_font_get_scale(struct Asset_Font * asset_font, float pixels_size) {
	return (pixels_size > 0)
		? stbtt_ScaleForPixelHeight(&asset_font->font, pixels_size)
		: stbtt_ScaleForMappingEmToPixels(&asset_font->font, -pixels_size);
}

uint32_t asset_font_get_glyph_id(struct Asset_Font * asset_font, uint32_t codepoint) {
	return (uint32_t)stbtt_FindGlyphIndex(&asset_font->font, (int)codepoint);
}

void asset_font_get_glyph_parameters(struct Asset_Font * asset_font, struct Glyph_Params * params, uint32_t glyph_id, float scale) {
	int advance_width, left_side_bearing;
	stbtt_GetGlyphHMetrics(&asset_font->font, (int)glyph_id, &advance_width, &left_side_bearing);

	int rect[4]; // left, bottom, right, top
	if (!stbtt_GetGlyphBox(&asset_font->font, (int)glyph_id, rect + 0, rect + 1, rect + 2, rect + 3)) {
		*params = (struct Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
		return;
	}

	int const size_x = rect[2] - rect[0];
	int const size_y = rect[3] - rect[1];

	*params = (struct Glyph_Params){
		.rect[0] = (int32_t)floorf(((float)rect[0]) * scale),
		.rect[1] = (int32_t)floorf(((float)rect[1]) * scale),
		.rect[2] = (int32_t)ceilf (((float)rect[2]) * scale),
		.rect[3] = (int32_t)ceilf (((float)rect[3]) * scale),
		.full_size_x = ((float)advance_width) * scale,
		.is_empty = (size_x > 0) && (size_y > 0) && stbtt_IsGlyphEmpty(&asset_font->font, (int)glyph_id),
	};
}

void asset_font_fill_buffer(
	struct Asset_Font * asset_font,
	uint8_t * buffer, uint32_t buffer_rect_width,
	uint32_t glyph_id, uint32_t glyph_size_x, uint32_t glyph_size_y, float scale
) {
	stbtt_MakeGlyphBitmap(
		&asset_font->font, buffer,
		(int)glyph_size_x, (int)glyph_size_y, (int)buffer_rect_width,
		scale, scale,
		(int)glyph_id
	);
}

int32_t asset_font_get_height(struct Asset_Font * asset_font) {
	return (int32_t)(asset_font->ascent - asset_font->descent);
}

int32_t asset_font_get_gap(struct Asset_Font * asset_font) {
	return (int32_t)asset_font->line_gap;
}

int32_t asset_font_get_kerning(struct Asset_Font * asset_font, uint32_t glyph_id1, uint32_t glyph_id2) {
	return (int32_t)stbtt_GetGlyphKernAdvance(&asset_font->font, (int)glyph_id1, (int)glyph_id2);
}
