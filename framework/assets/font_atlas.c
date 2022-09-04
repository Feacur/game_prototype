#include "framework/maths.h"

#include "framework/containers/buffer.h"
#include "framework/containers/array_any.h"
#include "framework/containers/hash_table_u64.h"

#include "framework/assets/image.h"
#include "framework/assets/font.h"
#include "framework/assets/font_glyph.h"

#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/unicode.h"

#define GLYPH_USAGE_MAX UINT8_MAX

struct Font_Image {
	struct Image buffer;
	//
	struct Font const * font;
	struct Hash_Table_U64 table;   // `struct Font_Key` : `struct Font_Glyph`
	struct Array_Any scratch;
	bool rendered;
};

// @note: supposed to be the same size as `uint64_t`
// @todo: static assert
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

struct Font_Image * font_atlas_init(struct Font const * font) {
	struct Font_Image * font_atlas = MEMORY_ALLOCATE(struct Font_Image);
	*font_atlas = (struct Font_Image){
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
		.font = font,
		.table = hash_table_u64_init(sizeof(struct Font_Glyph)),
		.scratch = array_any_init(sizeof(struct Font_Symbol)),
	};
	return font_atlas;
}

void font_atlas_free(struct Font_Image * font_atlas) {
	image_free(&font_atlas->buffer);
	hash_table_u64_free(&font_atlas->table);
	array_any_free(&font_atlas->scratch);

	common_memset(font_atlas, 0, sizeof(*font_atlas));
	MEMORY_FREE(font_atlas);
}

inline static uint64_t font_atlas_get_key_hash(struct Font_Key value);
void font_atlas_add_glyph(struct Font_Image * font_atlas, uint32_t codepoint, float size) {
	uint64_t const key_hash = font_atlas_get_key_hash((struct Font_Key){
		.codepoint = codepoint,
		.size = size,
	});

	struct Font_Glyph * glyph = hash_table_u64_get(&font_atlas->table, key_hash);
	if (glyph != NULL) { glyph->usage = GLYPH_USAGE_MAX; return; }

	uint32_t const glyph_id = font_get_glyph_id(font_atlas->font, codepoint);
	if (glyph_id == 0) { logger_to_console("font misses a glyph for codepoint '0x%x'\n", codepoint); DEBUG_BREAK(); return; }

	struct Glyph_Params const glyph_params = font_get_glyph_parameters(
		font_atlas->font, glyph_id, font_get_scale(font_atlas->font, size)
	);

	hash_table_u64_set(&font_atlas->table, key_hash, &(struct Font_Glyph){
		.params = glyph_params,
		.id = glyph_id,
		.usage = GLYPH_USAGE_MAX,
	});

	font_atlas->rendered = false;
}

void font_atlas_add_glyph_error(struct Font_Image *font_atlas, float size) {
	// @idea: reuse image space between differently sized error glyphs?
	float const scale        = font_get_scale(font_atlas->font, size);
	float const size_error_y = font_atlas_get_ascent(font_atlas, scale);
	float const size_error_x = size_error_y / 2;

	uint64_t const key_hash = font_atlas_get_key_hash((struct Font_Key){
		.codepoint = CODEPOINT_EMPTY,
		.size = size,
	});

	hash_table_u64_set(&font_atlas->table, key_hash, &(struct Font_Glyph){
		.params = (struct Glyph_Params){
			.full_size_x = size_error_x,
			.rect = {
				.min = {
					(int32_t)r32_floor(size_error_x * 0.2f),
					(int32_t)r32_floor(size_error_y * 0.0f),
				},
				.max = {
					(int32_t)r32_ceil (size_error_x * 0.9f),
					(int32_t)r32_ceil (size_error_y * 0.8f),
				},
			},
		},
		.usage = GLYPH_USAGE_MAX,
	});
}

void font_atlas_add_glyphs_from_range(struct Font_Image * font_atlas, uint32_t from, uint32_t to, float size) {
	for (uint32_t codepoint = from; codepoint <= to; codepoint++) {
		font_atlas_add_glyph(font_atlas, codepoint, size);
	}
}

void font_atlas_add_glyphs_from_text(struct Font_Image * font_atlas, uint32_t length, uint8_t const * data, float size) {
	FOR_UTF8 (length, data, it) {
		switch (it.codepoint) {
			case CODEPOINT_ZERO_WIDTH_SPACE: break;

			case ' ':
			case '\t':
			case CODEPOINT_NON_BREAKING_SPACE:
				font_atlas_add_glyph(font_atlas, ' ', size);
				break;

			default: if (it.codepoint > ' ') {
				font_atlas_add_glyph(font_atlas, it.codepoint, size);
			} break;
		}
	}
}

inline static struct Font_Key font_atlas_get_key(uint64_t value);
static int font_atlas_sort_comparison(void const * v1, void const * v2);
void font_atlas_render(struct Font_Image * font_atlas) {
	uint32_t const padding = 1;

	// track glyphs usage
	FOR_HASH_TABLE_U64 (&font_atlas->table, it) {
		if (it.key_hash == CODEPOINT_EMPTY) { continue; }
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
		struct Font_Glyph const * glyph = it.value;
		if (glyph->params.is_empty) { continue; }

		symbols_to_render[symbols_count] = (struct Font_Symbol){
			.key = font_atlas_get_key(it.key_hash),
			.glyph = it.value,
		};
		symbols_count++;
	}

	// sort glyphs by height, then by width
	common_qsort(symbols_to_render, symbols_count, sizeof(*symbols_to_render), font_atlas_sort_comparison);

	// resize the atlas
	{
		// estimate the very minimum area
		uint32_t minimum_area = 0;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Font_Symbol const * symbol = symbols_to_render + i;
			struct Glyph_Params const * params = &symbol->glyph->params;
			uint32_t const glyph_size_x = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect.max.x - params->rect.min.x) : 1;
			uint32_t const glyph_size_y = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect.max.y - params->rect.min.y) : 1;
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
			struct Glyph_Params const * params = &symbol->glyph->params;

			uint32_t const glyph_size_x = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect.max.x - params->rect.min.x) : 1;
			uint32_t const glyph_size_y = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect.max.y - params->rect.min.y) : 1;

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
			struct Glyph_Params const * params = &symbol->glyph->params;

			uint32_t const glyph_size_x = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect.max.x - params->rect.min.x) : 1;
			uint32_t const glyph_size_y = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect.max.y - params->rect.min.y) : 1;

			if (line_height == 0) { line_height = glyph_size_y; }

			if (offset_x + glyph_size_x + padding > font_atlas->buffer.size_x) {
				offset_x = padding;
				offset_y += line_height + padding;
				line_height = glyph_size_y;
			}

			if (font_atlas->buffer.size_x < offset_x + glyph_size_x - 1) {
				logger_to_console("can't fit a glyph into the buffer"); DEBUG_BREAK(); continue;
			}
			if (font_atlas->buffer.size_y < offset_y + glyph_size_y - 1) {
				logger_to_console("can't fit a glyph into the buffer"); DEBUG_BREAK(); continue;
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
				font_atlas->font,
				glyph->id, font_get_scale(font_atlas->font, symbol->key.size),
				font_atlas->buffer.data, font_atlas->buffer.size_x,
				glyph_size_x, glyph_size_y,
				offset_x, offset_y
			);

			offset_x += glyph_size_x + padding;
		}
	}

	MEMORY_FREE(symbols_to_render);
}

struct Image const * font_atlas_get_asset(struct Font_Image const * font_atlas) {
	return &font_atlas->buffer;
}

struct Font_Glyph const * font_atlas_get_glyph(struct Font_Image * const font_atlas, uint32_t codepoint, float size) {
	return hash_table_u64_get(&font_atlas->table, font_atlas_get_key_hash((struct Font_Key){
		.codepoint = codepoint,
		.size = size,
	}));
}

// 

float font_atlas_get_scale(struct Font_Image const * font_atlas, float size) {
	return font_get_scale(font_atlas->font, size);
}

float font_atlas_get_ascent(struct Font_Image const * font_atlas, float scale) {
	int32_t const value = font_get_ascent(font_atlas->font);
	return ((float)value) * scale;
}

float font_atlas_get_descent(struct Font_Image const * font_atlas, float scale) {
	int32_t const value = font_get_descent(font_atlas->font);
	return ((float)value) * scale;
}

float font_atlas_get_gap(struct Font_Image const * font_atlas, float scale) {
	int32_t const value = font_get_gap(font_atlas->font);
	return ((float)value) * scale;
}

float font_atlas_get_kerning(struct Font_Image const * font_atlas, uint32_t codepoint1, uint32_t codepoint2, float scale) {
	uint32_t const glyph_id1 = font_get_glyph_id(font_atlas->font, codepoint1);
	uint32_t const glyph_id2 = font_get_glyph_id(font_atlas->font, codepoint2);
	int32_t  const kerning   = font_get_kerning(font_atlas->font, glyph_id1, glyph_id2);
	return (float)kerning * scale;
}

//

static int font_atlas_sort_comparison(void const * v1, void const * v2) {
	struct Font_Symbol const * s1 = v1;
	struct Font_Symbol const * s2 = v2;

	if (s1->key.codepoint == CODEPOINT_EMPTY) { return  1; }
	if (s2->key.codepoint == CODEPOINT_EMPTY) { return -1; }

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

	if (s1->key.codepoint < s2->key.codepoint) { return  1; }
	if (s1->key.codepoint > s2->key.codepoint) { return -1; }

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
