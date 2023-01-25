#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/maths.h"
#include "framework/containers/buffer.h"

#include "framework/platform_file.h"

static void * font_memory_allocate(size_t size, struct Buffer * scratch);
static void font_memory_free(void * pointer, struct Buffer * scratch);

// @idea: compile third-parties as separate units
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Weverything"
#elif defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#define STBTT_malloc(size, user_data)  font_memory_allocate(size, user_data)
#define STBTT_free(pointer, user_data) font_memory_free(pointer, user_data)

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

struct Font {
	void * data;
	stbtt_fontinfo font;
	int ascent, descent, line_gap;
	struct Buffer * scratch;
};

struct Font * font_init(struct Buffer * buffer) {
	struct Font * font = MEMORY_ALLOCATE(struct Font);
	*font = (struct Font){0};

	font->scratch = MEMORY_ALLOCATE(struct Buffer);
	*font->scratch = buffer_init(NULL);

	// @note: memory ownership transfer
	font->data = buffer->data;
	*buffer = (struct Buffer){0};

	font->font.userdata = font->scratch;
	if (!stbtt_InitFont(&font->font, font->data, stbtt_GetFontOffsetForIndex(font->data, 0))) {
		logger_to_console("failure: can't read font data\n"); DEBUG_BREAK();
	}

	if (!stbtt_GetFontVMetricsOS2(&font->font, &font->ascent, &font->descent, &font->line_gap)) {
		stbtt_GetFontVMetrics(&font->font, &font->ascent, &font->descent, &font->line_gap);
	}

	return font;
}

void font_free(struct Font * font) {
	if (font == NULL) { logger_to_console("freeing NULL font\n"); return; }
	buffer_free(font->scratch); MEMORY_FREE(font->scratch);
	MEMORY_FREE(font->data);
	common_memset(font, 0, sizeof(*font));
	MEMORY_FREE(font);
}

uint32_t font_get_glyph_id(struct Font const * font, uint32_t codepoint) {
	return (uint32_t)stbtt_FindGlyphIndex(&font->font, (int)codepoint);
}

struct Font_Glyph_Params font_get_glyph_parameters(struct Font const * font, uint32_t glyph_id, float scale) {
	int advance_width, left_side_bearing;
	stbtt_GetGlyphHMetrics(&font->font, (int)glyph_id, &advance_width, &left_side_bearing);

	struct srect rect;
	if (!stbtt_GetGlyphBox(&font->font, (int)glyph_id, &rect.min.x, &rect.min.y, &rect.max.x, &rect.max.y)) {
		return (struct Font_Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
	}

	if ((rect.max.x <= rect.min.x) || (rect.max.y <= rect.min.y)) {
		return (struct Font_Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
	}

	if (stbtt_IsGlyphEmpty(&font->font, (int)glyph_id)) {
		return (struct Font_Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
	}

	return (struct Font_Glyph_Params){
		.rect = {
			.min = {
				(int32_t)r32_floor(((float)rect.min.x) * scale),
				(int32_t)r32_floor(((float)rect.min.y) * scale),
			},
			.max = {
				(int32_t)r32_ceil (((float)rect.max.x) * scale),
				(int32_t)r32_ceil (((float)rect.max.y) * scale),
			},
		},
		.full_size_x = ((float)advance_width) * scale,
	};
}

void font_render_glyph(
	struct Font const * font,
	uint32_t glyph_id, float scale,
	uint8_t * buffer, uint32_t buffer_width,
	uint32_t glyph_size_x, uint32_t glyph_size_y,
	uint32_t offset_x, uint32_t offset_y
) {
	if (glyph_size_x == 0) { logger_to_console("'glyph_size_x == 0' doesn't make sense\n"); DEBUG_BREAK(); return; }
	if (glyph_size_y == 0) { logger_to_console("'glyph_size_y == 0' doesn't make sense\n"); DEBUG_BREAK(); return; }

	if (glyph_id == 0) {
		uint8_t * base_target = buffer + offset_y * buffer_width + offset_x;
		uint32_t const bytes_to_set = glyph_size_x * sizeof(*buffer);
		for (uint32_t y = 0; y < glyph_size_y; y++) {
			common_memset(base_target + y * buffer_width, 0xff, bytes_to_set);
		}
		return;
	}

	if (buffer_width == 0) { logger_to_console("'buffer_width == 0' doesn't make sense\n"); DEBUG_BREAK(); return; }

	// @note: ensure glyphs layout
	// stbtt_set_flip_vertically_on_load(1);

	stbtt_MakeGlyphBitmap(
		&font->font, buffer + offset_y * buffer_width + offset_x,
		(int)glyph_size_x, (int)glyph_size_y, (int)buffer_width,
		scale, scale,
		(int)glyph_id
	);

	// @todo: arena/stack allocator
	buffer_ensure(font->scratch, font->scratch->size);
	buffer_clear(font->scratch);
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

//

static void * font_memory_allocate(size_t size, struct Buffer * scratch) {
	// @note: grow count, even past capacity
	size_t const offset = scratch->size;
	scratch->size += size;

	// @note: use the scratch buffer until it's memory should be reallocated
	if (scratch->size > scratch->capacity) { return MEMORY_ALLOCATE_SIZE(size); }
	return (uint8_t *)scratch->data + offset;
}

static void font_memory_free(void * pointer, struct Buffer * scratch) {
	// @note: free only non-buffered allocations
	void const * scratch_end = (uint8_t *)scratch->data + scratch->capacity;
	if (pointer < scratch->data || scratch_end < pointer) {
		MEMORY_FREE(pointer);
	}
}
