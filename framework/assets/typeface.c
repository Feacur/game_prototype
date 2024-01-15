#include "framework/formatter.h"
#include "framework/maths.h"

#include "framework/platform/file.h"
#include "framework/containers/buffer.h"
#include "framework/systems/memory.h"

#include "framework/__warnings_push.h"
	#define STBTT_malloc(size, user_data)  realloc_arena(NULL, size)
	#define STBTT_free(pointer, user_data) realloc_arena(pointer, 0)

	#define STBTT_STATIC
	#define STB_TRUETYPE_IMPLEMENTATION
	#include <stb/stb_truetype.h>
#include "framework/__warnings_pop.h"


//
#include "typeface.h"

struct Typeface {
	struct Buffer source;
	stbtt_fontinfo api;
	int ascent, descent, line_gap;
};

struct Typeface * typeface_init(struct Buffer * source) {
	struct Typeface * typeface = ALLOCATE(struct Typeface);
	*typeface = (struct Typeface){0};

	// @note: memory ownership transfer
	typeface->source = *source;
	*source = (struct Buffer){0};

	if (!stbtt_InitFont(&typeface->api, typeface->source.data, stbtt_GetFontOffsetForIndex(typeface->source.data, 0))) {
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
	buffer_free(&typeface->source);
	zero_out(CBMP_(typeface));
	FREE(typeface);
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
	struct uvec2 glyph_size,
	struct uvec2 offset
) {
	if (glyph_size.x == 0) {
		WRN("'glyph_size.x == 0' doesn't make sense");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}
	if (glyph_size.y == 0) {
		WRN("'glyph_size.y == 0' doesn't make sense");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}

	if (glyph_id == 0) {
		uint8_t * base_target = buffer + offset.y * buffer_width + offset.x;
		for (uint32_t y = 0; y < glyph_size.y; y++) {
			uint8_t * target = base_target + y * buffer_width;
			for (uint32_t x = 0; x < glyph_size.x; x++) {
				*target = 0xff;
			}
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
		&typeface->api, buffer + offset.y * buffer_width + offset.x,
		(int)glyph_size.x, (int)glyph_size.y, (int)buffer_width,
		scale, scale,
		(int)glyph_id
	);
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
