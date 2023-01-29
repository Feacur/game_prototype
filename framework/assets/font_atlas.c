#include "framework/maths.h"

#include "framework/containers/buffer.h"
#include "framework/containers/array_any.h"
#include "framework/containers/hash_table_u64.h"

#include "framework/assets/image.h"
#include "framework/assets/font.h"

#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/unicode.h"

#define GLYPH_USAGE_MAX UINT8_MAX

struct Font_Atlas {
	struct Image buffer;
	//
	struct Font const * font;
	struct Hash_Table_U64 table; // `struct Font_Key` : `struct Font_Glyph`
	struct Array_Any scratch;
	bool rendered;
};

// @note: supposed to be the same size as `uint64_t`
struct Font_Key {
	uint32_t codepoint;
	float size; // @note: differs from scale
};

//
#include "font_atlas.h"

struct Font_Symbol {
	struct Font_Key key;
	struct Font_Glyph * glyph; // @note: a short-lived pointer into a `Font_Image` table
};

struct Glyph_Codepoint {
	uint32_t glyph, codepoint;
};

struct Font_Atlas * font_atlas_init(void) {
	struct Font_Atlas * font_atlas = MEMORY_ALLOCATE(struct Font_Atlas);
	*font_atlas = (struct Font_Atlas){
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
		.table = hash_table_u64_init(sizeof(struct Font_Glyph)),
		.scratch = array_any_init(sizeof(struct Font_Symbol)),
	};
	return font_atlas;
}

void font_atlas_free(struct Font_Atlas * font_atlas) {
	if (font_atlas == NULL) { logger_to_console("freeing NULL font atlas\n"); return; }
	image_free(&font_atlas->buffer);
	hash_table_u64_free(&font_atlas->table);
	array_any_free(&font_atlas->scratch);

	common_memset(font_atlas, 0, sizeof(*font_atlas));
	MEMORY_FREE(font_atlas);
}

struct Font const * font_atlas_get_font(struct Font_Atlas const * font_atlas, uint32_t codepoint) {
	(void)codepoint;
	return font_atlas->font;
}

void font_atlas_set_font(struct Font_Atlas * font_atlas, struct Font const * font, uint32_t from, uint32_t to) {
	(void)(to - from);
	font_atlas->font = font;
}

inline static uint64_t font_atlas_get_key_hash(struct Font_Key value);
void font_atlas_add_glyph(struct Font_Atlas * font_atlas, uint32_t codepoint, float size) {
	uint64_t const key_hash = font_atlas_get_key_hash((struct Font_Key){
		.codepoint = codepoint,
		.size = size,
	});

	struct Font_Glyph * glyph = hash_table_u64_get(&font_atlas->table, key_hash);
	if (glyph != NULL) { glyph->usage = GLYPH_USAGE_MAX; return; }

	struct Font const * font = font_atlas_get_font(font_atlas, codepoint);
	if (font == NULL) { logger_to_console("font misses a font for codepoint '0x%x'\n", codepoint); return; }

	uint32_t const glyph_id = font_get_glyph_id(font, codepoint);
	if (glyph_id == 0) { logger_to_console("font misses a glyph for codepoint '0x%x'\n", codepoint); }

	struct Font_Glyph_Params const glyph_params = font_get_glyph_parameters(
		font, glyph_id, font_get_scale(font, size)
	);

	hash_table_u64_set(&font_atlas->table, key_hash, &(struct Font_Glyph){
		.params = glyph_params,
		.id = glyph_id,
		.usage = GLYPH_USAGE_MAX,
	});

	font_atlas->rendered = false;
}

static void font_atlas_add_default_glyph(struct Font_Atlas *font_atlas, uint32_t codepoint, float size, float full_size_x, struct srect rect) {
	uint64_t const key_hash = font_atlas_get_key_hash((struct Font_Key){
		.codepoint = codepoint,
		.size = size,
	});

	struct Font_Glyph * glyph = hash_table_u64_get(&font_atlas->table, key_hash);
	if (glyph != NULL) { glyph->usage = GLYPH_USAGE_MAX; return; }

	hash_table_u64_set(&font_atlas->table, key_hash, &(struct Font_Glyph){
		.params = (struct Font_Glyph_Params){
			.full_size_x = full_size_x,
			.rect = rect,
		},
		.usage = GLYPH_USAGE_MAX,
	});

	font_atlas->rendered = false;
}

void font_atlas_add_default_glyphs(struct Font_Atlas *font_atlas, float size) {
	float const scale        = font_atlas_get_scale(font_atlas, size);
	float const font_ascent  = font_atlas_get_ascent(font_atlas, scale);
	float const font_descent = font_atlas_get_descent(font_atlas, scale);
	float const line_gap     = font_atlas_get_gap(font_atlas, scale);
	float const line_height  = font_ascent - font_descent + line_gap;

	font_atlas_add_default_glyph(font_atlas, '\0', size, line_height / 2, (struct srect){
		.min = {
			(int32_t)r32_floor(line_height * 0.1f),
			(int32_t)r32_floor(line_height * 0.0f),
		},
		.max = {
			(int32_t)r32_ceil (line_height * 0.45f),
			(int32_t)r32_ceil (line_height * 0.8f),
		},
	});

	font_atlas_add_glyph(font_atlas, ' ', size);
	struct Font_Glyph const * glyph_space = font_atlas_get_glyph(font_atlas, ' ', size);
	float const glyph_space_size = (glyph_space != NULL) ? glyph_space->params.full_size_x : 0;

	font_atlas_add_default_glyph(font_atlas, '\t',                         size, glyph_space_size * 4, (struct srect){0}); // @todo: expose tab scale
	font_atlas_add_default_glyph(font_atlas, '\r',                         size, 0,                    (struct srect){0});
	font_atlas_add_default_glyph(font_atlas, '\n',                         size, 0,                    (struct srect){0});
	font_atlas_add_default_glyph(font_atlas, CODEPOINT_ZERO_WIDTH_SPACE,   size, 0,                    (struct srect){0});
	font_atlas_add_default_glyph(font_atlas, CODEPOINT_NON_BREAKING_SPACE, size, glyph_space_size,     (struct srect){0});
}

inline static struct Font_Key font_atlas_get_key(uint64_t value);
static int font_atlas_sort_comparison(void const * v1, void const * v2);
void font_atlas_render(struct Font_Atlas * font_atlas) {
	uint32_t const padding = 1;

	// track glyphs usage
	FOR_HASH_TABLE_U64 (&font_atlas->table, it) {
		struct Font_Glyph * glyph = it.value;
		if (glyph->usage == 0) {
			font_atlas->rendered = false;
			hash_table_u64_del_at(&font_atlas->table, it.current);
			continue;
		}

		glyph->usage--;
	}

	if (font_atlas->rendered) { return; }
	font_atlas->rendered = true;

	// collect visible glyphs
	uint32_t symbols_count = 0;
	struct Font_Symbol * symbols_to_render = MEMORY_ALLOCATE_ARRAY(struct Font_Symbol, font_atlas->table.count);

	FOR_HASH_TABLE_U64 (&font_atlas->table, it) {
		struct Font_Glyph * glyph = it.value;
		if (glyph->params.is_empty) { continue; }
		if (glyph->id == 0) { continue; }

		struct Font_Key const key = font_atlas_get_key(it.key_hash);
		if (codepoint_is_invisible(key.codepoint)) { continue; }

		symbols_to_render[symbols_count++] = (struct Font_Symbol){
			.key = key,
			.glyph = glyph,
		};
	}

	// sort glyphs by height, then by width
	common_qsort(symbols_to_render, symbols_count, sizeof(*symbols_to_render), font_atlas_sort_comparison);

	// append with a virtual error glyph
	struct Font_Glyph error_glyph = {
		.params.rect.max = {1, 1},
	};
	symbols_to_render[symbols_count++] = (struct Font_Symbol){
		.glyph = &error_glyph,
	};

	// resize the atlas
	{
		// estimate the very minimum area
		uint32_t minimum_area = 0;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Font_Symbol const * symbol = symbols_to_render + i;
			struct Font_Glyph_Params const * params = &symbol->glyph->params;
			uint32_t const glyph_size_x = (uint32_t)(params->rect.max.x - params->rect.min.x);
			uint32_t const glyph_size_y = (uint32_t)(params->rect.max.y - params->rect.min.y);
			minimum_area += (glyph_size_x + padding) * (glyph_size_y + padding);
		}

		// estimate required atlas dimesions
		uint32_t atlas_size_x;
		uint32_t atlas_size_y;
		if (font_atlas->buffer.size_x == 0 || font_atlas->buffer.size_y == 0) {
			atlas_size_x = (uint32_t)r32_sqrt((float)minimum_area);
			atlas_size_x = round_up_to_PO2_u32(atlas_size_x);

			atlas_size_y = atlas_size_x;
			if (atlas_size_x * (atlas_size_y / 2) > minimum_area) {
				atlas_size_y = atlas_size_y / 2;
			}
		}
		else {
			atlas_size_y = font_atlas->buffer.size_y;

			atlas_size_x = minimum_area / atlas_size_y;
			atlas_size_x = round_up_to_PO2_u32(atlas_size_y);
			if (atlas_size_x < font_atlas->buffer.size_x) {
				atlas_size_x = font_atlas->buffer.size_x;
			}
		}

		// verify estimated atlas dimensions
		verify_dimensions:
		uint32_t line_height = 0;
		uint32_t offset_x = padding, offset_y = padding;
		for (uint32_t i = 0; i < symbols_count; i++) {

			struct Font_Symbol const * symbol = symbols_to_render + i;
			struct Font_Glyph_Params const * params = &symbol->glyph->params;

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

		image_ensure(&font_atlas->buffer, atlas_size_x, atlas_size_y);
	}

	// render glyphs into the atlas, assuming they shall fit
	uint32_t const buffer_data_size = data_type_get_size(font_atlas->buffer.parameters.data_type);
	common_memset(font_atlas->buffer.data, 0, font_atlas->buffer.size_x * font_atlas->buffer.size_y * buffer_data_size);
	{
		uint32_t line_height = 0;
		uint32_t offset_x = padding, offset_y = padding;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Font_Symbol const * symbol = symbols_to_render + i;
			struct Font_Glyph_Params const * params = &symbol->glyph->params;

			struct Font const * font = font_atlas_get_font(font_atlas, symbol->key.codepoint);
			if (font == NULL) { continue; }

			uint32_t const glyph_size_x = (uint32_t)(params->rect.max.x - params->rect.min.x);
			uint32_t const glyph_size_y = (uint32_t)(params->rect.max.y - params->rect.min.y);

			if (line_height == 0) { line_height = glyph_size_y; }

			if (offset_x + glyph_size_x + padding > font_atlas->buffer.size_x) {
				offset_x = padding;
				offset_y += line_height + padding;
				line_height = glyph_size_y;
			}

			if (font_atlas->buffer.size_x < offset_x + glyph_size_x - 1) {
				logger_to_console("can't fit a glyph into the buffer\n"); DEBUG_BREAK(); continue;
			}
			if (font_atlas->buffer.size_y < offset_y + glyph_size_y - 1) {
				logger_to_console("can't fit a glyph into the buffer\n"); DEBUG_BREAK(); continue;
			}

			//
			struct Font_Glyph * glyph = symbol->glyph;
			glyph->uv = (struct rect){
				.min = {
					(float)(offset_x)                / (float)font_atlas->buffer.size_x,
					(float)(offset_y)                / (float)font_atlas->buffer.size_y,
				},
				.max = {
					(float)(offset_x + glyph_size_x) / (float)font_atlas->buffer.size_x,
					(float)(offset_y + glyph_size_y) / (float)font_atlas->buffer.size_y,
				},
			};

			font_render_glyph(
				font,
				glyph->id, font_get_scale(font, symbol->key.size),
				font_atlas->buffer.data, font_atlas->buffer.size_x,
				glyph_size_x, glyph_size_y,
				offset_x, offset_y
			);

			offset_x += glyph_size_x + padding;
		}
	}

	MEMORY_FREE(symbols_to_render);

	// reuse error glyph UVs
	FOR_HASH_TABLE_U64 (&font_atlas->table, it) {
		struct Font_Glyph * glyph = it.value;
		if (glyph->params.is_empty) { continue; }
		if (glyph->id != 0) { continue; }

		struct Font_Key const key = font_atlas_get_key(it.key_hash);
		if (codepoint_is_invisible(key.codepoint)) { continue; }

		glyph->uv = error_glyph.uv;
	}
}

struct Image const * font_atlas_get_asset(struct Font_Atlas const * font_atlas) {
	return &font_atlas->buffer;
}

struct Font_Glyph const * font_atlas_get_glyph(struct Font_Atlas * const font_atlas, uint32_t codepoint, float size) {
	return hash_table_u64_get(&font_atlas->table, font_atlas_get_key_hash((struct Font_Key){
		.codepoint = codepoint,
		.size = size,
	}));
}

// 

float font_atlas_get_scale(struct Font_Atlas const * font_atlas, float size) {
	uint32_t const codepoint = 0;
	struct Font const * font = font_atlas_get_font(font_atlas, codepoint);
	if (font == NULL) { return 1; }
	return font_get_scale(font, size);
}

float font_atlas_get_ascent(struct Font_Atlas const * font_atlas, float scale) {
	uint32_t const codepoint = 0;
	struct Font const * font = font_atlas_get_font(font_atlas, codepoint);
	if (font == NULL) { return 0; }
	int32_t const value = font_get_ascent(font);
	return ((float)value) * scale;
}

float font_atlas_get_descent(struct Font_Atlas const * font_atlas, float scale) {
	uint32_t const codepoint = 0;
	struct Font const * font = font_atlas_get_font(font_atlas, codepoint);
	if (font == NULL) { return 0; }
	int32_t const value = font_get_descent(font);
	return ((float)value) * scale;
}

float font_atlas_get_gap(struct Font_Atlas const * font_atlas, float scale) {
	uint32_t const codepoint = 0;
	struct Font const * font = font_atlas_get_font(font_atlas, codepoint);
	if (font == NULL) { return 0; }
	int32_t const value = font_get_gap(font);
	return ((float)value) * scale;
}

float font_atlas_get_kerning(struct Font_Atlas const * font_atlas, uint32_t codepoint1, uint32_t codepoint2, float scale) {
	uint32_t const codepoint = 0;
	struct Font const * font = font_atlas_get_font(font_atlas, codepoint);
	if (font == NULL) { return 0; }
	uint32_t const glyph_id1 = font_get_glyph_id(font, codepoint1);
	uint32_t const glyph_id2 = font_get_glyph_id(font, codepoint2);
	int32_t  const kerning   = font_get_kerning(font, glyph_id1, glyph_id2);
	return (float)kerning * scale;
}

//

static int font_atlas_sort_comparison(void const * v1, void const * v2) {
	struct Font_Symbol const * s1 = v1;
	struct Font_Symbol const * s2 = v2;

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

inline static struct Font_Key font_atlas_get_key(uint64_t value) {
	union {
		uint64_t value_u64;
		struct Font_Key value_key;
	} data;
	data.value_u64 = value;
	return data.value_key;
}

inline static uint64_t font_atlas_get_key_hash(struct Font_Key value) {
	union {
		uint64_t value_u64;
		struct Font_Key value_key;
	} data;
	data.value_key = value;
	return data.value_u64;
}

#undef GLYPH_USAGE_MAX
