#include "framework/memory.h"
#include "framework/formatter.h"
#include "framework/maths.h"
#include "framework/containers/buffer.h"

#include "framework/platform/file.h"

static void * typeface_memory_allocate(size_t size, struct Buffer * scratch);
static void typeface_memory_free(void * pointer, struct Buffer * scratch);

#include "framework/__warnings_push.h"
	#define STBTT_malloc(size, user_data)  typeface_memory_allocate(size, user_data)
	#define STBTT_free(pointer, user_data) typeface_memory_free(pointer, user_data)

	#define STBTT_STATIC
	#define STB_TRUETYPE_IMPLEMENTATION
	#include <stb/stb_truetype.h>
#include "framework/__warnings_pop.h"

//
#include "typeface.h"

struct Typeface {
	void * data;
	stbtt_fontinfo api;
	int ascent, descent, line_gap;
	struct Buffer * scratch;
};

struct Typeface * typeface_init(struct Buffer * buffer) {
	struct Typeface * typeface = MEMORY_ALLOCATE(struct Typeface);
	*typeface = (struct Typeface){0};

	typeface->scratch = MEMORY_ALLOCATE(struct Buffer);
	*typeface->scratch = buffer_init(NULL);

	// @note: memory ownership transfer
	typeface->data = buffer->data;
	*buffer = (struct Buffer){0};

	typeface->api.userdata = typeface->scratch;
	if (!stbtt_InitFont(&typeface->api, typeface->data, stbtt_GetFontOffsetForIndex(typeface->data, 0))) {
		ERR("failure: can't read typeface data");
		REPORT_CALLSTACK(); DEBUG_BREAK();
	}

	if (!stbtt_GetFontVMetricsOS2(&typeface->api, &typeface->ascent, &typeface->descent, &typeface->line_gap)) {
		stbtt_GetFontVMetrics(&typeface->api, &typeface->ascent, &typeface->descent, &typeface->line_gap);
	}

	return typeface;
}

void typeface_free(struct Typeface * typeface) {
	if (typeface == NULL) { WRN("freeing NULL typeface"); return; }
	buffer_free(typeface->scratch); MEMORY_FREE(typeface->scratch);
	MEMORY_FREE(typeface->data);
	common_memset(typeface, 0, sizeof(*typeface));
	MEMORY_FREE(typeface);
}

uint32_t typeface_get_glyph_id(struct Typeface const * typeface, uint32_t codepoint) {
	return (uint32_t)stbtt_FindGlyphIndex(&typeface->api, (int)codepoint);
}

struct Glyph_Params typeface_get_glyph_parameters(struct Typeface const * typeface, uint32_t glyph_id, float scale) {
	int advance_width, left_side_bearing;
	stbtt_GetGlyphHMetrics(&typeface->api, (int)glyph_id, &advance_width, &left_side_bearing);

	struct srect rect;
	if (!stbtt_GetGlyphBox(&typeface->api, (int)glyph_id, &rect.min.x, &rect.min.y, &rect.max.x, &rect.max.y)) {
		return (struct Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
	}

	if ((rect.max.x <= rect.min.x) || (rect.max.y <= rect.min.y)) {
		return (struct Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
	}

	if (stbtt_IsGlyphEmpty(&typeface->api, (int)glyph_id)) {
		return (struct Glyph_Params){
			.full_size_x = ((float)advance_width) * scale,
			.is_empty = true,
		};
	}

	return (struct Glyph_Params){
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

void typeface_render_glyph(
	struct Typeface const * typeface,
	uint32_t glyph_id, float scale,
	uint8_t * buffer, uint32_t buffer_width,
	uint32_t glyph_size_x, uint32_t glyph_size_y,
	uint32_t offset_x, uint32_t offset_y
) {
	if (glyph_size_x == 0) {
		WRN("'glyph_size_x == 0' doesn't make sense");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}
	if (glyph_size_y == 0) {
		WRN("'glyph_size_y == 0' doesn't make sense");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}

	if (glyph_id == 0) {
		uint8_t * base_target = buffer + offset_y * buffer_width + offset_x;
		uint32_t const bytes_to_set = glyph_size_x * sizeof(*buffer);
		for (uint32_t y = 0; y < glyph_size_y; y++) {
			common_memset(base_target + y * buffer_width, 0xff, bytes_to_set);
		}
		return;
	}

	if (buffer_width == 0) {
		WRN("'buffer_width == 0' doesn't make sense");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}

	// @note: ensure glyphs layout
	// stbtt_set_flip_vertically_on_load(1);

	stbtt_MakeGlyphBitmap(
		&typeface->api, buffer + offset_y * buffer_width + offset_x,
		(int)glyph_size_x, (int)glyph_size_y, (int)buffer_width,
		scale, scale,
		(int)glyph_id
	);

	// @todo: arena/stack allocator
	buffer_ensure(typeface->scratch, typeface->scratch->size);
	buffer_clear(typeface->scratch);
}

float typeface_get_scale(struct Typeface const * typeface, float pixels_size) {
	if (pixels_size > 0) { return stbtt_ScaleForPixelHeight(&typeface->api, pixels_size); }
	if (pixels_size < 0) { return stbtt_ScaleForMappingEmToPixels(&typeface->api, -pixels_size); }
	return 0;
}

int32_t typeface_get_ascent(struct Typeface const * typeface) {
	return (int32_t)typeface->ascent;
}

int32_t typeface_get_descent(struct Typeface const * typeface) {
	return (int32_t)typeface->descent;
}

int32_t typeface_get_gap(struct Typeface const * typeface) {
	return (int32_t)typeface->line_gap;
}

int32_t typeface_get_kerning(struct Typeface const * typeface, uint32_t glyph_id1, uint32_t glyph_id2) {
	return (int32_t)stbtt_GetGlyphKernAdvance(&typeface->api, (int)glyph_id1, (int)glyph_id2);
}

//

static void * typeface_memory_allocate(size_t size, struct Buffer * scratch) {
	// @note: grow count, even past capacity
	size_t const offset = scratch->size;
	scratch->size += size;

	// @note: use the scratch buffer until it's memory should be reallocated
	if (scratch->size > scratch->capacity) { return MEMORY_ALLOCATE_SIZE(size); }
	return (uint8_t *)scratch->data + offset;
}

static void typeface_memory_free(void * pointer, struct Buffer * scratch) {
	// @note: free only non-buffered allocations
	void const * scratch_end = (uint8_t *)scratch->data + scratch->capacity;
	if (pointer < scratch->data || scratch_end < pointer) {
		MEMORY_FREE(pointer);
	}
}
