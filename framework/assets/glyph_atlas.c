#include "framework/maths.h"

#include "framework/containers/buffer.h"
#include "framework/containers/array_any.h"
#include "framework/containers/hash_table_u64.h"

#include "framework/assets/image.h"
#include "framework/assets/typeface.h"

#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/unicode.h"

#define GLYPH_GC_TIMEOUT_MAX UINT8_MAX

struct Glyph_Atlas_Range {
	uint32_t from, to;
	struct Typeface const * typeface;
};

struct Glyph_Atlas {
	struct Image buffer;
	//
	struct Array_Any ranges; // `struct Glyph_Atlas_Range`
	struct Hash_Table_U64 table; // `struct Typeface_Key` : `struct Typeface_Glyph`
	bool rendered;
};

// @note: supposed to be the same size as `uint64_t`
struct Typeface_Key {
	uint32_t codepoint;
	float size; // @note: differs from scale
};

//
#include "glyph_atlas.h"

struct Typeface_Symbol {
	struct Typeface_Key key;
	struct Typeface_Glyph * glyph; // @note: a short-lived pointer into a `Glyph_Atlas` table
};

struct Glyph_Codepoint {
	uint32_t glyph, codepoint;
};

struct Glyph_Atlas * glyph_atlas_init(void) {
	struct Glyph_Atlas * glyph_atlas = MEMORY_ALLOCATE(struct Glyph_Atlas);
	*glyph_atlas = (struct Glyph_Atlas){
		.buffer = {
			.parameters = {
				.texture_type = TEXTURE_TYPE_COLOR,
				.data_type = DATA_TYPE_R8_UNORM,
				.flags = TEXTURE_FLAG_MUTABLE | TEXTURE_FLAG_WRITE,
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
		.ranges = array_any_init(sizeof(struct Glyph_Atlas_Range)),
		.table = hash_table_u64_init(sizeof(struct Typeface_Glyph)),
	};
	return glyph_atlas;
}

void glyph_atlas_free(struct Glyph_Atlas * glyph_atlas) {
	if (glyph_atlas == NULL) { logger_to_console("freeing NULL glyph atlas\n"); return; }
	image_free(&glyph_atlas->buffer);
	array_any_free(&glyph_atlas->ranges);
	hash_table_u64_free(&glyph_atlas->table);

	common_memset(glyph_atlas, 0, sizeof(*glyph_atlas));
	MEMORY_FREE(glyph_atlas);
}

struct Typeface const * glyph_atlas_get_typeface(struct Glyph_Atlas const * glyph_atlas, uint32_t codepoint) {
	for (uint32_t i = glyph_atlas->ranges.count; i > 0; i--) {
		struct Glyph_Atlas_Range const * range = array_any_at(&glyph_atlas->ranges, i - 1);
		if (codepoint < range->from) { continue; }
		if (codepoint > range->to) { continue; }
		return range->typeface;
	}
	return NULL;
}

void glyph_atlas_set_typeface(struct Glyph_Atlas * glyph_atlas, struct Typeface const * typeface, uint32_t codepoint_from, uint32_t codepoint_to) {
	array_any_push_many(&glyph_atlas->ranges, 1, &(struct Glyph_Atlas_Range){
		.from = codepoint_from,
		.to = codepoint_to,
		.typeface = typeface,
	});
}

inline static uint64_t glyph_atlas_get_key_hash(struct Typeface_Key value);
void glyph_atlas_add_glyph(struct Glyph_Atlas * glyph_atlas, uint32_t codepoint, float size) {
	uint64_t const key_hash = glyph_atlas_get_key_hash((struct Typeface_Key){
		.codepoint = codepoint,
		.size = size,
	});

	struct Typeface_Glyph * glyph = hash_table_u64_get(&glyph_atlas->table, key_hash);
	if (glyph != NULL) { glyph->gc_timeout = GLYPH_GC_TIMEOUT_MAX; return; }

	struct Typeface const * typeface = glyph_atlas_get_typeface(glyph_atlas, codepoint);
	if (typeface == NULL) { logger_to_console("glyph atlas misses a typeface for codepoint '0x%x'\n", codepoint); return; }

	uint32_t const glyph_id = typeface_get_glyph_id(typeface, codepoint);
	if (glyph_id == 0) { logger_to_console("glyph atlas misses a glyph for codepoint '0x%x'\n", codepoint); }

	struct Typeface_Glyph_Params const glyph_params = typeface_get_glyph_parameters(
		typeface, glyph_id, typeface_get_scale(typeface, size)
	);

	hash_table_u64_set(&glyph_atlas->table, key_hash, &(struct Typeface_Glyph){
		.params = glyph_params,
		.id = glyph_id,
		.gc_timeout = GLYPH_GC_TIMEOUT_MAX,
	});

	glyph_atlas->rendered = false;
}

static void glyph_atlas_add_default_glyph(struct Glyph_Atlas *glyph_atlas, uint32_t codepoint, float size, float full_size_x, struct srect rect) {
	uint64_t const key_hash = glyph_atlas_get_key_hash((struct Typeface_Key){
		.codepoint = codepoint,
		.size = size,
	});

	struct Typeface_Glyph * glyph = hash_table_u64_get(&glyph_atlas->table, key_hash);
	if (glyph != NULL) { glyph->gc_timeout = GLYPH_GC_TIMEOUT_MAX; return; }

	hash_table_u64_set(&glyph_atlas->table, key_hash, &(struct Typeface_Glyph){
		.params = (struct Typeface_Glyph_Params){
			.full_size_x = full_size_x,
			.rect = rect,
		},
		.gc_timeout = GLYPH_GC_TIMEOUT_MAX,
	});

	glyph_atlas->rendered = false;
}

void glyph_atlas_add_default_glyphs(struct Glyph_Atlas *glyph_atlas, float size) {
	float const scale       = glyph_atlas_get_scale(glyph_atlas, size);
	float const ascent      = glyph_atlas_get_ascent(glyph_atlas, scale);
	float const descent     = glyph_atlas_get_descent(glyph_atlas, scale);
	float const line_gap    = glyph_atlas_get_gap(glyph_atlas, scale);
	float const line_height = ascent - descent + line_gap;

	glyph_atlas_add_default_glyph(glyph_atlas, '\0', size, line_height / 2, (struct srect){
		.min = {
			(int32_t)r32_floor(line_height * 0.1f),
			(int32_t)r32_floor(line_height * 0.0f),
		},
		.max = {
			(int32_t)r32_ceil (line_height * 0.45f),
			(int32_t)r32_ceil (line_height * 0.8f),
		},
	});

	glyph_atlas_add_glyph(glyph_atlas, ' ', size);
	struct Typeface_Glyph const * glyph_space = glyph_atlas_get_glyph(glyph_atlas, ' ', size);
	float const glyph_space_size = (glyph_space != NULL) ? glyph_space->params.full_size_x : 0;

	glyph_atlas_add_default_glyph(glyph_atlas, '\t',                         size, glyph_space_size * 4, (struct srect){0}); // @todo: expose tab scale
	glyph_atlas_add_default_glyph(glyph_atlas, '\r',                         size, 0,                    (struct srect){0});
	glyph_atlas_add_default_glyph(glyph_atlas, '\n',                         size, 0,                    (struct srect){0});
	glyph_atlas_add_default_glyph(glyph_atlas, CODEPOINT_ZERO_WIDTH_SPACE,   size, 0,                    (struct srect){0});
	glyph_atlas_add_default_glyph(glyph_atlas, CODEPOINT_NON_BREAKING_SPACE, size, glyph_space_size,     (struct srect){0});
}

inline static struct Typeface_Key glyph_atlas_get_key(uint64_t value);
static int glyph_atlas_sort_comparison(void const * v1, void const * v2);
void glyph_atlas_render(struct Glyph_Atlas * glyph_atlas) {
	uint32_t const padding = 1;

	// track glyphs usage
	FOR_HASH_TABLE_U64 (&glyph_atlas->table, it) {
		struct Typeface_Glyph * glyph = it.value;
		if (glyph->gc_timeout == 0) {
			glyph_atlas->rendered = false;
			hash_table_u64_del_at(&glyph_atlas->table, it.current);
			continue;
		}

		glyph->gc_timeout--;
	}

	if (glyph_atlas->rendered) { return; }
	glyph_atlas->rendered = true;

	// collect visible glyphs
	uint32_t symbols_count = 0;
	struct Typeface_Symbol * symbols_to_render = MEMORY_ALLOCATE_ARRAY(struct Typeface_Symbol, glyph_atlas->table.count);

	FOR_HASH_TABLE_U64 (&glyph_atlas->table, it) {
		struct Typeface_Glyph * glyph = it.value;
		if (glyph->params.is_empty) { continue; }
		if (glyph->id == 0) { continue; }

		struct Typeface_Key const key = glyph_atlas_get_key(it.key_hash);
		if (codepoint_is_invisible(key.codepoint)) { continue; }

		symbols_to_render[symbols_count++] = (struct Typeface_Symbol){
			.key = key,
			.glyph = glyph,
		};
	}

	// sort glyphs by height, then by width
	common_qsort(symbols_to_render, symbols_count, sizeof(*symbols_to_render), glyph_atlas_sort_comparison);

	// append with a virtual error glyph
	struct Typeface_Glyph error_glyph = {
		.params.rect.max = {1, 1},
	};
	symbols_to_render[symbols_count++] = (struct Typeface_Symbol){
		.glyph = &error_glyph,
	};

	// resize the atlas
	{
		// estimate the very minimum area
		uint32_t minimum_area = 0;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Typeface_Symbol const * symbol = symbols_to_render + i;
			struct Typeface_Glyph_Params const * params = &symbol->glyph->params;
			uint32_t const glyph_size_x = (uint32_t)(params->rect.max.x - params->rect.min.x);
			uint32_t const glyph_size_y = (uint32_t)(params->rect.max.y - params->rect.min.y);
			minimum_area += (glyph_size_x + padding) * (glyph_size_y + padding);
		}

		// estimate required atlas dimesions
		uint32_t atlas_size_x;
		uint32_t atlas_size_y;
		if (glyph_atlas->buffer.size_x == 0 || glyph_atlas->buffer.size_y == 0) {
			atlas_size_x = (uint32_t)r32_sqrt((float)minimum_area);
			atlas_size_x = round_up_to_PO2_u32(atlas_size_x);

			atlas_size_y = atlas_size_x;
			if (atlas_size_x * (atlas_size_y / 2) > minimum_area) {
				atlas_size_y = atlas_size_y / 2;
			}
		}
		else {
			atlas_size_y = glyph_atlas->buffer.size_y;

			atlas_size_x = minimum_area / atlas_size_y;
			atlas_size_x = round_up_to_PO2_u32(atlas_size_y);
			if (atlas_size_x < glyph_atlas->buffer.size_x) {
				atlas_size_x = glyph_atlas->buffer.size_x;
			}
		}

		// verify estimated atlas dimensions
		verify_dimensions:
		uint32_t line_height = 0;
		uint32_t offset_x = padding, offset_y = padding;
		for (uint32_t i = 0; i < symbols_count; i++) {

			struct Typeface_Symbol const * symbol = symbols_to_render + i;
			struct Typeface_Glyph_Params const * params = &symbol->glyph->params;

			uint32_t const glyph_size_x = (uint32_t)(params->rect.max.x - params->rect.min.x);
			uint32_t const glyph_size_y = (uint32_t)(params->rect.max.y - params->rect.min.y);

			if (line_height == 0) { line_height = glyph_size_y; }

			if (offset_x + glyph_size_x + padding > atlas_size_x) {
				offset_x = padding;
				offset_y += line_height + padding;
				line_height = glyph_size_y;
			}

			if (offset_y + line_height + padding > atlas_size_y) {
				if (atlas_size_x > atlas_size_y) {
					atlas_size_y = atlas_size_y * 2;
				}
				else {
					// @note: change of width requires reevaluation from scratch
					atlas_size_x = atlas_size_x * 2;
					goto verify_dimensions;
				}
			}

			offset_x += glyph_size_x + padding;
		}

		image_ensure(&glyph_atlas->buffer, atlas_size_x, atlas_size_y);
	}

	// render glyphs into the atlas, assuming they shall fit
	uint32_t const buffer_data_size = data_type_get_size(glyph_atlas->buffer.parameters.data_type);
	common_memset(glyph_atlas->buffer.data, 0, glyph_atlas->buffer.size_x * glyph_atlas->buffer.size_y * buffer_data_size);
	{
		uint32_t line_height = 0;
		uint32_t offset_x = padding, offset_y = padding;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Typeface_Symbol const * symbol = symbols_to_render + i;
			struct Typeface_Glyph_Params const * params = &symbol->glyph->params;

			struct Typeface const * typeface = glyph_atlas_get_typeface(glyph_atlas, symbol->key.codepoint);
			if (typeface == NULL) { continue; }

			uint32_t const glyph_size_x = (uint32_t)(params->rect.max.x - params->rect.min.x);
			uint32_t const glyph_size_y = (uint32_t)(params->rect.max.y - params->rect.min.y);

			if (line_height == 0) { line_height = glyph_size_y; }

			if (offset_x + glyph_size_x + padding > glyph_atlas->buffer.size_x) {
				offset_x = padding;
				offset_y += line_height + padding;
				line_height = glyph_size_y;
			}

			if (glyph_atlas->buffer.size_x < offset_x + glyph_size_x - 1) {
				logger_to_console("can't fit a glyph into the buffer\n"); DEBUG_BREAK(); continue;
			}
			if (glyph_atlas->buffer.size_y < offset_y + glyph_size_y - 1) {
				logger_to_console("can't fit a glyph into the buffer\n"); DEBUG_BREAK(); continue;
			}

			//
			struct Typeface_Glyph * glyph = symbol->glyph;
			glyph->uv = (struct rect){
				.min = {
					(float)(offset_x)                / (float)glyph_atlas->buffer.size_x,
					(float)(offset_y)                / (float)glyph_atlas->buffer.size_y,
				},
				.max = {
					(float)(offset_x + glyph_size_x) / (float)glyph_atlas->buffer.size_x,
					(float)(offset_y + glyph_size_y) / (float)glyph_atlas->buffer.size_y,
				},
			};

			typeface_render_glyph(
				typeface,
				glyph->id, typeface_get_scale(typeface, symbol->key.size),
				glyph_atlas->buffer.data, glyph_atlas->buffer.size_x,
				glyph_size_x, glyph_size_y,
				offset_x, offset_y
			);

			offset_x += glyph_size_x + padding;
		}
	}

	MEMORY_FREE(symbols_to_render);

	// reuse error glyph UVs
	FOR_HASH_TABLE_U64 (&glyph_atlas->table, it) {
		struct Typeface_Glyph * glyph = it.value;
		if (glyph->params.is_empty) { continue; }
		if (glyph->id != 0) { continue; }

		struct Typeface_Key const key = glyph_atlas_get_key(it.key_hash);
		if (codepoint_is_invisible(key.codepoint)) { continue; }

		glyph->uv = error_glyph.uv;
	}
}

struct Image const * glyph_atlas_get_asset(struct Glyph_Atlas const * glyph_atlas) {
	return &glyph_atlas->buffer;
}

struct Typeface_Glyph const * glyph_atlas_get_glyph(struct Glyph_Atlas * const glyph_atlas, uint32_t codepoint, float size) {
	return hash_table_u64_get(&glyph_atlas->table, glyph_atlas_get_key_hash((struct Typeface_Key){
		.codepoint = codepoint,
		.size = size,
	}));
}

// 

float glyph_atlas_get_scale(struct Glyph_Atlas const * glyph_atlas, float size) {
	uint32_t const codepoint = 0;
	struct Typeface const * typeface = glyph_atlas_get_typeface(glyph_atlas, codepoint);
	if (typeface == NULL) { return 1; }
	return typeface_get_scale(typeface, size);
}

float glyph_atlas_get_ascent(struct Glyph_Atlas const * glyph_atlas, float scale) {
	uint32_t const codepoint = 0;
	struct Typeface const * typeface = glyph_atlas_get_typeface(glyph_atlas, codepoint);
	if (typeface == NULL) { return 0; }
	int32_t const value = typeface_get_ascent(typeface);
	return ((float)value) * scale;
}

float glyph_atlas_get_descent(struct Glyph_Atlas const * glyph_atlas, float scale) {
	uint32_t const codepoint = 0;
	struct Typeface const * typeface = glyph_atlas_get_typeface(glyph_atlas, codepoint);
	if (typeface == NULL) { return 0; }
	int32_t const value = typeface_get_descent(typeface);
	return ((float)value) * scale;
}

float glyph_atlas_get_gap(struct Glyph_Atlas const * glyph_atlas, float scale) {
	uint32_t const codepoint = 0;
	struct Typeface const * typeface = glyph_atlas_get_typeface(glyph_atlas, codepoint);
	if (typeface == NULL) { return 0; }
	int32_t const value = typeface_get_gap(typeface);
	return ((float)value) * scale;
}

float glyph_atlas_get_kerning(struct Glyph_Atlas const * glyph_atlas, uint32_t codepoint1, uint32_t codepoint2, float scale) {
	uint32_t const codepoint = 0;
	struct Typeface const * typeface = glyph_atlas_get_typeface(glyph_atlas, codepoint);
	if (typeface == NULL) { return 0; }
	uint32_t const glyph_id1 = typeface_get_glyph_id(typeface, codepoint1);
	uint32_t const glyph_id2 = typeface_get_glyph_id(typeface, codepoint2);
	int32_t  const kerning   = typeface_get_kerning(typeface, glyph_id1, glyph_id2);
	return (float)kerning * scale;
}

//

static int glyph_atlas_sort_comparison(void const * v1, void const * v2) {
	struct Typeface_Symbol const * s1 = v1;
	struct Typeface_Symbol const * s2 = v2;

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

inline static struct Typeface_Key glyph_atlas_get_key(uint64_t value) {
	union {
		uint64_t value_u64;
		struct Typeface_Key value_key;
	} data;
	data.value_u64 = value;
	return data.value_key;
}

inline static uint64_t glyph_atlas_get_key_hash(struct Typeface_Key value) {
	union {
		uint64_t value_u64;
		struct Typeface_Key value_key;
	} data;
	data.value_key = value;
	return data.value_u64;
}

#undef GLYPH_GC_TIMEOUT_MAX
