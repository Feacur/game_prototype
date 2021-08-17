#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/containers/array_byte.h"

#include "framework/platform_file.h"

#include <string.h>
#include <math.h>

// better to compile third-parties as separate units
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Weverything"
#elif defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#define STBTT_malloc(size, user_data)  MEMORY_ALLOCATE_SIZE(NULL, size)
#define STBTT_free(pointer, user_data) MEMORY_FREE(NULL, pointer)

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#if defined(__clang__)
	#pragma clang diagnostic pop
#elif defined(_MSC_VER)
	#pragma warning(pop)
#endif

//
#include "font.h"
#include "font_glyph.h"

// @idea: cache here basic metrics, without scaling?
struct Font {
	struct Array_Byte file;
	stbtt_fontinfo font;
	int ascent, descent, line_gap;
};

struct Font * font_init(char const * path) {
	struct Font * font = MEMORY_ALLOCATE(NULL, struct Font);

	platform_file_read_entire(path, &font->file);
	if (!stbtt_InitFont(&font->font, font->file.data, stbtt_GetFontOffsetForIndex(font->file.data, 0))) {
		logger_to_console("'stbtt_InitFont' failed\n"); DEBUG_BREAK();
	}

	stbtt_GetFontVMetrics(&font->font, &font->ascent, &font->descent, &font->line_gap);

	return font;
}

void font_free(struct Font * font) {
	array_byte_free(&font->file);
	memset(font, 0, sizeof(*font));
	MEMORY_FREE(font, font);
}

uint32_t font_get_glyph_id(struct Font const * font, uint32_t codepoint) {
	return (uint32_t)stbtt_FindGlyphIndex(&font->font, (int)codepoint);
}

void font_get_glyph_parameters(struct Font const * font, struct Glyph_Params * params, uint32_t glyph_id, float scale) {
	int advance_width, left_side_bearing;
	stbtt_GetGlyphHMetrics(&font->font, (int)glyph_id, &advance_width, &left_side_bearing);

	int rect[4]; // left, bottom, right, top
	if (!stbtt_GetGlyphBox(&font->font, (int)glyph_id, rect + 0, rect + 1, rect + 2, rect + 3)) {
		*params = (struct Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
		return;
	}

	if ((rect[2] <= rect[0]) || (rect[3] <= rect[1])) {
		*params = (struct Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
		return;
	}

	if (stbtt_IsGlyphEmpty(&font->font, (int)glyph_id)) {
		*params = (struct Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
		return;
	}

	*params = (struct Glyph_Params){
		.rect[0] = (int32_t)floorf(((float)rect[0]) * scale),
		.rect[1] = (int32_t)floorf(((float)rect[1]) * scale),
		.rect[2] = (int32_t)ceilf (((float)rect[2]) * scale),
		.rect[3] = (int32_t)ceilf (((float)rect[3]) * scale),
		.full_size_x = ((float)advance_width) * scale,
	};
}

void font_fill_buffer(
	struct Font const * font,
	uint8_t * buffer, uint32_t buffer_rect_width,
	uint32_t glyph_id, uint32_t glyph_size_x, uint32_t glyph_size_y, float scale
) {
	if (glyph_id == 0) {
		if (glyph_size_x == 0) { logger_to_console("'glyph_size_x == 0' doesn't make sense\n"); DEBUG_BREAK(); }
		if (glyph_size_y == 0) { logger_to_console("'glyph_size_y == 0' doesn't make sense\n"); DEBUG_BREAK(); }
		buffer[0] = 0xff;
		memset(buffer, 0xff, glyph_size_x * glyph_size_y * sizeof(*buffer));
		return;
	}
	// @note: there's no `stbtt_set_flip_vertically_on_load`
	//        currently code relies on "emulation" of it
	stbtt_MakeGlyphBitmap(
		&font->font, buffer,
		(int)glyph_size_x, (int)glyph_size_y, (int)buffer_rect_width,
		scale, scale,
		(int)glyph_id
	);
}

float font_get_scale(struct Font const * font, float pixels_size) {
	return (pixels_size > 0)
		? stbtt_ScaleForPixelHeight(&font->font, pixels_size)
		: stbtt_ScaleForMappingEmToPixels(&font->font, -pixels_size);
}

int32_t font_get_height(struct Font const * font) {
	return (int32_t)(font->ascent - font->descent);
}

int32_t font_get_gap(struct Font const * font) {
	return (int32_t)font->line_gap;
}

int32_t font_get_kerning(struct Font const * font, uint32_t glyph_id1, uint32_t glyph_id2) {
	return (int32_t)stbtt_GetGlyphKernAdvance(&font->font, (int)glyph_id1, (int)glyph_id2);
}
