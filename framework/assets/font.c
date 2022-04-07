#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/maths.h"
#include "framework/containers/buffer.h"

#include "framework/platform_file.h"


// @idea: compile third-parties as separate units
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

struct Font {
	struct Buffer file;
	stbtt_fontinfo font;
	int ascent, descent, line_gap;
};

struct Font * font_init(struct Buffer * buffer) {
	struct Font * font = MEMORY_ALLOCATE(NULL, struct Font);

	// @note: memory ownership transfer
	font->file = *buffer;
	*buffer = (struct Buffer){0};

	if (!stbtt_InitFont(&font->font, font->file.data, stbtt_GetFontOffsetForIndex(font->file.data, 0))) {
		logger_to_console("failure: can't read font data\n"); DEBUG_BREAK();
	}

	if (!stbtt_GetFontVMetricsOS2(&font->font, &font->ascent, &font->descent, &font->line_gap)) {
		stbtt_GetFontVMetrics(&font->font, &font->ascent, &font->descent, &font->line_gap);
	}

	return font;
}

void font_free(struct Font * font) {
	buffer_free(&font->file);
	common_memset(font, 0, sizeof(*font));
	MEMORY_FREE(NULL, font);
}

uint32_t font_get_glyph_id(struct Font const * font, uint32_t codepoint) {
	return (uint32_t)stbtt_FindGlyphIndex(&font->font, (int)codepoint);
}

struct Glyph_Params font_get_glyph_parameters(struct Font const * font, uint32_t glyph_id, float scale) {
	int advance_width, left_side_bearing;
	stbtt_GetGlyphHMetrics(&font->font, (int)glyph_id, &advance_width, &left_side_bearing);

	int rect[4]; // left, bottom, right, top
	if (!stbtt_GetGlyphBox(&font->font, (int)glyph_id, rect + 0, rect + 1, rect + 2, rect + 3)) {
		return (struct Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
	}

	if ((rect[2] <= rect[0]) || (rect[3] <= rect[1])) {
		return (struct Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
	}

	if (stbtt_IsGlyphEmpty(&font->font, (int)glyph_id)) {
		return (struct Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
	}

	return (struct Glyph_Params){
		.rect = {
			[0] = (int32_t)maths_floor(((float)rect[0]) * scale),
			[1] = (int32_t)maths_floor(((float)rect[1]) * scale),
			[2] = (int32_t)maths_ceil (((float)rect[2]) * scale),
			[3] = (int32_t)maths_ceil (((float)rect[3]) * scale),
		},
		.full_size_x = ((float)advance_width) * scale,
	};
}

void font_fill_buffer(
	struct Font const * font,
	uint32_t glyph_id, float scale,
	uint8_t * buffer, uint32_t buffer_width,
	uint32_t glyph_size_x, uint32_t glyph_size_y,
	uint32_t offset_x, uint32_t offset_y
) {
	if (glyph_size_x == 0) { logger_to_console("'glyph_size_x == 0' doesn't make sense\n"); DEBUG_BREAK(); return; }
	if (glyph_size_y == 0) { logger_to_console("'glyph_size_y == 0' doesn't make sense\n"); DEBUG_BREAK(); return; }

	if (glyph_id == 0) {
		common_memset(buffer, 0xff, glyph_size_x * glyph_size_y * sizeof(*buffer));
		return;
	}

	if (buffer_width == 0) { logger_to_console("'buffer_width == 0' doesn't make sense\n"); DEBUG_BREAK(); return; }

	// @note: ensure glyphs data layout inside the atlas
	// stbtt_set_flip_vertically_on_load(1);

	stbtt_MakeGlyphBitmap(
		&font->font, buffer + offset_y * buffer_width + offset_x,
		(int)glyph_size_x, (int)glyph_size_y, (int)buffer_width,
		scale, scale,
		(int)glyph_id
	);
}

float font_get_scale(struct Font const * font, float pixels_size) {
	if (pixels_size > 0) { return stbtt_ScaleForPixelHeight(&font->font, pixels_size); }
	if (pixels_size < 0) { return stbtt_ScaleForMappingEmToPixels(&font->font, -pixels_size); }
	return 0;
}

int32_t font_get_ascent(struct Font const * font) {
	return (int32_t)font->ascent;
}

int32_t font_get_descent(struct Font const * font) {
	return (int32_t)font->descent;
}

int32_t font_get_gap(struct Font const * font) {
	return (int32_t)font->line_gap;
}

int32_t font_get_kerning(struct Font const * font, uint32_t glyph_id1, uint32_t glyph_id2) {
	return (int32_t)stbtt_GetGlyphKernAdvance(&font->font, (int)glyph_id1, (int)glyph_id2);
}
