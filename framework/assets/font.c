#include "framework/maths.h"
#include "framework/formatter.h"
#include "framework/unicode.h"

#include "framework/containers/buffer.h"
#include "framework/containers/array.h"
#include "framework/containers/hashmap.h"

#include "framework/systems/memory.h"

#include "framework/assets/image.h"
#include "framework/assets/typeface.h"


#define GLYPH_GC_TIMEOUT_MAX UINT8_MAX

struct Typeface_Range {
	struct Typeface const * typeface;
	uint32_t from, to;
};

struct Font {
	struct Image buffer;
	//
	struct Array ranges;  // `struct Typeface_Range`
	struct Hashmap table; // `struct Typeface_Key` : `struct Glyph`
	bool rendered;
};

struct Typeface_Key {
	uint32_t codepoint;
	float size; // @note: differs from scale
};

static HASHER(hash_typeface_key) {
	struct Typeface_Key const * k = value;
	// kinda FNV-1
	uint32_t const prime = 16777619u;
	uint32_t hash = 2166136261u;
	hash = (hash * prime) ^ k->codepoint;
	hash = (hash * prime) ^ bits_r32_u32(k->size);
	return hash;
}

//
#include "font.h"

struct Symbol_To_Render {
	struct Typeface_Key key;
	struct Glyph * glyph; // @note: a short-lived pointer into a `font` table
};

struct Glyph_Codepoint {
	uint32_t glyph, codepoint;
};

struct Font * font_init(void) {
	struct Font * font = ALLOCATE(struct Font);
	*font = (struct Font){
		.buffer = {
			.format = {
				.flags = TEXTURE_FLAG_COLOR,
				.type = GFX_TYPE_R8_UNORM,
			},
			.settings = {
				.swizzle = {
					SWIZZLE_OP_1,
					SWIZZLE_OP_1,
					SWIZZLE_OP_1,
					SWIZZLE_OP_R,
				},
			},
		},
		.ranges = array_init(sizeof(struct Typeface_Range)),
		.table = hashmap_init(&hash_typeface_key, sizeof(struct Typeface_Key), sizeof(struct Glyph)),
	};
	return font;
}

void font_free(struct Font * font) {
	if (font == NULL) { WRN("freeing NULL glyph atlas"); return; }
	image_free(&font->buffer);
	array_free(&font->ranges);
	hashmap_free(&font->table);

	zero_out(AMP_(font));
	FREE(font);
}

struct Typeface const * font_get_typeface(struct Font const * font, uint32_t codepoint) {
	for (uint32_t i = font->ranges.count; i > 0; i--) {
		struct Typeface_Range const * range = array_at(&font->ranges, i - 1);
		if (codepoint < range->from) { continue; }
		if (codepoint > range->to) { continue; }
		return range->typeface;
	}
	return NULL;
}

void font_set_typeface(struct Font * font, struct Typeface const * typeface, uint32_t codepoint_from, uint32_t codepoint_to) {
	if (codepoint_from > codepoint_to) { return; }
	array_push_many(&font->ranges, 1, &(struct Typeface_Range){
		.typeface = typeface,
		.from = codepoint_from,
		.to = codepoint_to,
	});
}

void font_add_glyph(struct Font * font, uint32_t codepoint, float size) {
	struct Typeface_Key const key = {
		.codepoint = codepoint,
		.size = size,
	};
	struct Glyph * glyph = hashmap_get(&font->table, &key);
	if (glyph != NULL) { glyph->gc_timeout = GLYPH_GC_TIMEOUT_MAX; return; }

	struct Typeface const * typeface = font_get_typeface(font, codepoint);
	if (typeface == NULL) { WRN("glyph atlas misses a typeface for codepoint '%#x'", codepoint); return; }

	uint32_t const glyph_id = typeface_get_glyph_id(typeface, codepoint);
	if (glyph_id == 0) { WRN("glyph atlas misses a glyph for codepoint '%#x'", codepoint); }

	struct Glyph_Params const glyph_params = typeface_get_glyph_parameters(
		typeface, glyph_id, typeface_get_scale(typeface, size)
	);

	hashmap_set(&font->table, &key, &(struct Glyph){
		.params = glyph_params,
		.id = glyph_id,
		.gc_timeout = GLYPH_GC_TIMEOUT_MAX,
	});

	font->rendered = false;
}

static void font_add_default(struct Font *font, uint32_t codepoint, float size, float full_size_x, struct srect rect) {
	struct Typeface_Key const key = {
		.codepoint = codepoint,
		.size = size,
	};

	struct Glyph * glyph = hashmap_get(&font->table, &key);
	if (glyph != NULL) { glyph->gc_timeout = GLYPH_GC_TIMEOUT_MAX; return; }

	hashmap_set(&font->table, &key, &(struct Glyph){
		.params = (struct Glyph_Params){
			.full_size_x = full_size_x,
			.rect = rect,
		},
		.gc_timeout = GLYPH_GC_TIMEOUT_MAX,
	});

	font->rendered = false;
}

void font_add_defaults(struct Font *font, float size) {
	float const scale       = font_get_scale(font, size);
	float const ascent      = font_get_ascent(font, scale);
	float const descent     = font_get_descent(font, scale);
	float const line_gap    = font_get_gap(font, scale);
	float const line_height = ascent - descent + line_gap;

	font_add_default(font, '\0', size, line_height / 2, (struct srect){
		.min = {
			(int32_t)r32_floor(line_height * 0.1f),
			(int32_t)r32_floor(line_height * 0.0f),
		},
		.max = {
			(int32_t)r32_ceil (line_height * 0.45f),
			(int32_t)r32_ceil (line_height * 0.8f),
		},
	});

	font_add_glyph(font, ' ', size);
	struct Glyph const * glyph_space = font_get_glyph(font, ' ', size);
	float const glyph_space_size = (glyph_space != NULL) ? glyph_space->params.full_size_x : 0;

	font_add_default(font, '\t',                         size, glyph_space_size * 4, (struct srect){0}); // @todo: expose tab scale
	font_add_default(font, '\r',                         size, 0,                    (struct srect){0});
	font_add_default(font, '\n',                         size, 0,                    (struct srect){0});
	font_add_default(font, CODEPOINT_ZERO_WIDTH_SPACE,   size, 0,                    (struct srect){0});
	font_add_default(font, CODEPOINT_NON_BREAKING_SPACE, size, glyph_space_size,     (struct srect){0});
}

static int compare_symbol_to_render(void const * v1, void const * v2);
void font_render(struct Font * font) {
	uint32_t const padding = 1;

	// track glyphs usage
	FOR_HASHMAP(&font->table, it) {
		struct Glyph * glyph = it.value;
		if (glyph->gc_timeout == 0) {
			font->rendered = false;
			hashmap_del_at(&font->table, it.curr);
			continue;
		}

		glyph->gc_timeout--;
	}

	if (font->rendered) { return; }
	font->rendered = true;

	// collect visible glyphs
	uint32_t symbols_count = 0;
	struct Symbol_To_Render * symbols_to_render = ARENA_ALLOCATE_ARRAY(struct Symbol_To_Render, font->table.count);

	FOR_HASHMAP(&font->table, it) {
		struct Glyph * glyph = it.value;
		if (glyph->params.is_empty) { continue; }
		if (glyph->id == 0) { continue; }

		struct Typeface_Key const * key = it.key;
		if (codepoint_is_invisible(key->codepoint)) { continue; }

		symbols_to_render[symbols_count++] = (struct Symbol_To_Render){
			.key = *key,
			.glyph = glyph,
		};
	}

	// sort glyphs by height, then by width
	common_qsort(symbols_to_render, symbols_count, sizeof(*symbols_to_render), compare_symbol_to_render);

	// append with a virtual error glyph
	struct Glyph error_glyph = {
		.params.rect.max = {1, 1},
	};
	symbols_to_render[symbols_count++] = (struct Symbol_To_Render){
		.glyph = &error_glyph,
	};

	// resize the atlas
	{
		// estimate the very minimum area
		uint32_t minimum_area = 0;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Symbol_To_Render const * symbol = symbols_to_render + i;
			struct Glyph_Params const * params = &symbol->glyph->params;
			struct uvec2 const glyph_size = {
				(uint32_t)(params->rect.max.x - params->rect.min.x),
				(uint32_t)(params->rect.max.y - params->rect.min.y),
			};
			minimum_area += (glyph_size.x + padding) * (glyph_size.y + padding);
		}

		// estimate required atlas dimesions
		struct uvec2 atlas_size;
		if (font->buffer.size.x == 0 || font->buffer.size.y == 0) {
			atlas_size.x = (uint32_t)r32_sqrt((float)minimum_area);
			atlas_size.x = po2_next_u32(atlas_size.x);

			atlas_size.y = atlas_size.x;
			if (atlas_size.x * (atlas_size.y / 2) > minimum_area) {
				atlas_size.y = atlas_size.y / 2;
			}
		}
		else {
			atlas_size.y = font->buffer.size.y;

			atlas_size.x = minimum_area / atlas_size.y;
			atlas_size.x = po2_next_u32(atlas_size.y);
			if (atlas_size.x < font->buffer.size.x) {
				atlas_size.x = font->buffer.size.x;
			}
		}

		// verify estimated atlas dimensions
		verify_dimensions:; // zig cc
		uint32_t line_height = 0;
		struct uvec2 offset = {padding, padding};
		for (uint32_t i = 0; i < symbols_count; i++) {

			struct Symbol_To_Render const * symbol = symbols_to_render + i;
			struct Glyph_Params const * params = &symbol->glyph->params;

			struct uvec2 const glyph_size = {
				(uint32_t)(params->rect.max.x - params->rect.min.x),
				(uint32_t)(params->rect.max.y - params->rect.min.y),
			};

			if (line_height == 0) { line_height = glyph_size.y; }

			if (offset.x + glyph_size.x + padding > atlas_size.x) {
				offset.x = padding;
				offset.y += line_height + padding;
				line_height = glyph_size.y;
			}

			if (offset.y + line_height + padding > atlas_size.y) {
				if (atlas_size.x > atlas_size.y) {
					atlas_size.y = atlas_size.y * 2;
				}
				else {
					// @note: change of width requires reevaluation from scratch
					atlas_size.x = atlas_size.x * 2;
					goto verify_dimensions;
				}
			}

			offset.x += glyph_size.x + padding;
		}

		image_ensure(&font->buffer, atlas_size);
	}

	// render glyphs into the atlas, assuming they shall fit
	uint32_t const buffer_data_size = gfx_type_get_size(font->buffer.format.type);
	zero_out((struct CArray_Mut){
		.size = font->buffer.size.x * font->buffer.size.y * buffer_data_size,
		.data = font->buffer.data,
	});
	{
		uint32_t line_height = 0;
		struct uvec2 offset = {padding, padding};
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Symbol_To_Render const * symbol = symbols_to_render + i;
			struct Glyph_Params const * params = &symbol->glyph->params;

			struct Typeface const * typeface = font_get_typeface(font, symbol->key.codepoint);
			if (typeface == NULL) { continue; }

			struct uvec2 const glyph_size = {
				(uint32_t)(params->rect.max.x - params->rect.min.x),
				(uint32_t)(params->rect.max.y - params->rect.min.y),
			};

			if (line_height == 0) { line_height = glyph_size.y; }

			if (offset.x + glyph_size.x + padding > font->buffer.size.x) {
				offset.x = padding;
				offset.y += line_height + padding;
				line_height = glyph_size.y;
			}

			if (font->buffer.size.x < offset.x + glyph_size.x - 1) {
				WRN("can't fit a glyph into the buffer");
				REPORT_CALLSTACK(); DEBUG_BREAK(); continue;
			}
			if (font->buffer.size.y < offset.y + glyph_size.y - 1) {
				WRN("can't fit a glyph into the buffer");
				REPORT_CALLSTACK(); DEBUG_BREAK(); continue;
			}

			//
			struct Glyph * glyph = symbol->glyph;
			glyph->uv = (struct rect){
				.min = {
					(float)(offset.x)                / (float)font->buffer.size.x,
					(float)(offset.y)                / (float)font->buffer.size.y,
				},
				.max = {
					(float)(offset.x + glyph_size.x) / (float)font->buffer.size.x,
					(float)(offset.y + glyph_size.y) / (float)font->buffer.size.y,
				},
			};

			typeface_render_glyph(
				typeface,
				glyph->id, typeface_get_scale(typeface, symbol->key.size),
				font->buffer.data, font->buffer.size.x,
				glyph_size, offset
			);

			offset.x += glyph_size.x + padding;
		}
	}

	ARENA_FREE(symbols_to_render);

	// reuse error glyph UVs
	FOR_HASHMAP(&font->table, it) {
		struct Glyph * glyph = it.value;
		if (glyph->params.is_empty) { continue; }
		if (glyph->id != 0) { continue; }

		struct Typeface_Key const *key = it.key;
		if (codepoint_is_invisible(key->codepoint)) { continue; }

		glyph->uv = error_glyph.uv;
	}
}

struct Image const * font_get_asset(struct Font const * font) {
	return &font->buffer;
}

struct Glyph const * font_get_glyph(struct Font * const font, uint32_t codepoint, float size) {
	return hashmap_get(&font->table, &(struct Typeface_Key){
		.codepoint = codepoint,
		.size = size,
	});
}

// 

float font_get_scale(struct Font const * font, float size) {
	uint32_t const codepoint = 0;
	struct Typeface const * typeface = font_get_typeface(font, codepoint);
	if (typeface == NULL) { return 1; }
	return typeface_get_scale(typeface, size);
}

float font_get_ascent(struct Font const * font, float scale) {
	uint32_t const codepoint = 0;
	struct Typeface const * typeface = font_get_typeface(font, codepoint);
	if (typeface == NULL) { return 0; }
	int32_t const value = typeface_get_ascent(typeface);
	return ((float)value) * scale;
}

float font_get_descent(struct Font const * font, float scale) {
	uint32_t const codepoint = 0;
	struct Typeface const * typeface = font_get_typeface(font, codepoint);
	if (typeface == NULL) { return 0; }
	int32_t const value = typeface_get_descent(typeface);
	return ((float)value) * scale;
}

float font_get_gap(struct Font const * font, float scale) {
	uint32_t const codepoint = 0;
	struct Typeface const * typeface = font_get_typeface(font, codepoint);
	if (typeface == NULL) { return 0; }
	int32_t const value = typeface_get_gap(typeface);
	return ((float)value) * scale;
}

float font_get_kerning(struct Font const * font, uint32_t codepoint1, uint32_t codepoint2, float scale) {
	uint32_t const codepoint = 0;
	struct Typeface const * typeface = font_get_typeface(font, codepoint);
	if (typeface == NULL) { return 0; }
	uint32_t const glyph_id1 = typeface_get_glyph_id(typeface, codepoint1);
	uint32_t const glyph_id2 = typeface_get_glyph_id(typeface, codepoint2);
	int32_t  const kerning   = typeface_get_kerning(typeface, glyph_id1, glyph_id2);
	return (float)kerning * scale;
}

//

static COMPARATOR(compare_symbol_to_render) {
	struct Symbol_To_Render const * s1 = v1;
	struct Symbol_To_Render const * s2 = v2;

	struct srect const r1 = s1->glyph->params.rect;
	struct srect const r2 = s2->glyph->params.rect;

	uint32_t const size_y_1 = (uint32_t)(r1.max.y - r1.min.y);
	uint32_t const size_y_2 = (uint32_t)(r2.max.y - r2.min.y);

	if (size_y_1 < size_y_2) { return  1; }
	if (size_y_1 > size_y_2) { return -1; }

	uint32_t const size_x_1 = (uint32_t)(r1.max.x - r1.min.x);
	uint32_t const size_x_2 = (uint32_t)(r2.max.x - r2.min.x);

	if (size_x_1 < size_x_2) { return  1; }
	if (size_x_1 > size_x_2) { return -1; }

	if (s1->glyph->id < s2->glyph->id) { return  1; }
	if (s1->glyph->id > s2->glyph->id) { return -1; }

	return 0;
}

#undef GLYPH_GC_TIMEOUT_MAX
