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
	struct Hash_Table_U64 kerning; // codepoints pair : offset
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
#include "font_image.h"

struct Font_Symbol {
	struct Font_Key key;
	struct Font_Glyph * glyph; // @note: a short-lived pointer into a `Font_Image` table
};

struct Glyph_Codepoint {
	uint32_t glyph, codepoint;
};

struct Font_Image * font_image_init(struct Font const * font) {
	struct Font_Image * font_image = MEMORY_ALLOCATE(struct Font_Image);
	*font_image = (struct Font_Image){
		.buffer = {
			.parameters = {
				.texture_type = TEXTURE_TYPE_COLOR,
				.data_type = DATA_TYPE_R8_UNORM,
				.flags = TEXTURE_FLAG_MUTABLE | TEXTURE_FLAG_WRITE,
			},
			.settings = {
				.swizzle[0] = SWIZZLE_OP_1,
				.swizzle[1] = SWIZZLE_OP_1,
				.swizzle[2] = SWIZZLE_OP_1,
				.swizzle[3] = SWIZZLE_OP_R,
			},
		},
		.font = font,
		.table = hash_table_u64_init(sizeof(struct Font_Glyph)),
		.kerning = hash_table_u64_init(sizeof(int32_t)),
		.scratch = array_any_init(sizeof(struct Font_Symbol)),
	};
	return font_image;
}

void font_image_free(struct Font_Image * font_image) {
	image_free(&font_image->buffer);
	hash_table_u64_free(&font_image->table);
	hash_table_u64_free(&font_image->kerning);
	array_any_free(&font_image->scratch);

	common_memset(font_image, 0, sizeof(*font_image));
	MEMORY_FREE(font_image);
}

void font_image_add_glyph_error(struct Font_Image *font_image, float size) {
	// @idea: reuse image space between differently sized error glyphs?
	float const scale        = font_get_scale(font_image->font, size);
	float const size_error_y = font_image_get_ascent(font_image, scale);
	float const size_error_x = size_error_y / 2;

	hash_table_u64_set(&font_image->table, CODEPOINT_EMPTY, &(struct Font_Glyph){
		.params = (struct Glyph_Params){
			.full_size_x = size_error_x,
			.rect[0] = (int32_t)maths_floor(size_error_x * 0.2f),
			.rect[1] = (int32_t)maths_floor(size_error_y * 0.0f),
			.rect[2] = (int32_t)maths_ceil (size_error_x * 0.9f),
			.rect[3] = (int32_t)maths_ceil (size_error_y * 0.8f),
		},
		.usage = GLYPH_USAGE_MAX,
	});
}

inline static void font_image_add_glyph(struct Font_Image * font_image, uint32_t codepoint, float size);
void font_image_add_glyphs_from_range(struct Font_Image * font_image, uint32_t from, uint32_t to, float size) {
	for (uint32_t codepoint = from; codepoint <= to; codepoint++) {
		font_image_add_glyph(font_image, codepoint, size);
	}
}

void font_image_add_glyphs_from_text(struct Font_Image * font_image, uint32_t length, uint8_t const * data, float size) {
	FOR_UTF8 (length, data, it) {
		switch (it.codepoint) {
			case CODEPOINT_ZERO_WIDTH_SPACE: break;

			case ' ':
			case '\t':
			case CODEPOINT_NON_BREAKING_SPACE:
				font_image_add_glyph(font_image, ' ', size);
				break;

			default: if (it.codepoint > ' ') {
				font_image_add_glyph(font_image, it.codepoint, size);
			} break;
		}
	}
}

inline static void font_image_add_kerning(struct Font_Image * font_image, uint32_t codepoint1, uint32_t codepoint2, uint32_t glyph1, uint32_t glyph2);
void font_image_add_kerning_from_range(struct Font_Image * font_image, uint32_t from, uint32_t to) {
	uint32_t * glyphs = MEMORY_ALLOCATE_ARRAY(uint32_t, to - from + 1);

	uint32_t count = 0;
	for (uint32_t codepoint = from; codepoint <= to; codepoint++) {
		uint32_t const glyph_id = font_get_glyph_id(font_image->font, codepoint);
		if (glyph_id == 0) { continue; }
		glyphs[count] = glyph_id;
		count++;
	}

	for (uint32_t i1 = 0; i1 < count; i1++) {
		uint32_t const codepoint1 = from + i1;
		uint32_t const glyph1 = glyphs[i1];
		for (uint32_t i2 = 0; i2 < count; i2++) {
			font_image_add_kerning(font_image, codepoint1, from + i2, glyph1, glyphs[i2]);
		}
	}

	MEMORY_FREE(glyphs);
}

void font_image_add_kerning_from_text(struct Font_Image * font_image, uint32_t length, uint8_t const * data) {
	uint32_t codepoints_count = 0;
	FOR_UTF8 (length, data, it) { codepoints_count++; }

	array_any_ensure(&font_image->scratch, codepoints_count);
	struct Glyph_Codepoint * pairs = font_image->scratch.data;

	uint32_t count = 0;
	FOR_UTF8 (length, data, it) {
		uint32_t const glyph_id = font_get_glyph_id(font_image->font, it.codepoint);
		if (glyph_id == 0) { continue; }
		pairs[count] = (struct Glyph_Codepoint){
			.glyph = glyph_id,
			.codepoint = it.codepoint,
		};
		count++;
	}

	for (uint32_t i1 = 0; i1 < count; i1++) {
		struct Glyph_Codepoint const pair1 = pairs[i1];
		for (uint32_t i2 = 0; i2 < count; i2++) {
			struct Glyph_Codepoint const pair2 = pairs[i2];
			font_image_add_kerning(font_image, pair1.codepoint, pair2.codepoint, pair1.glyph, pair2.glyph);
		}
	}
}

inline static struct Font_Key font_image_get_key(uint64_t value);
void font_image_add_kerning_all(struct Font_Image * font_image) {
	hash_table_u64_clear(&font_image->kerning);
	FOR_HASH_TABLE_U64 (&font_image->table, it1) {
		struct Font_Glyph const * glyph1 = it1.value;
		struct Font_Key const key1 = font_image_get_key(it1.key_hash);
		FOR_HASH_TABLE_U64 (&font_image->table, it2) {
			struct Font_Glyph const * glyph2 = it2.value;
			struct Font_Key const key2 = font_image_get_key(it2.key_hash);
			font_image_add_kerning(font_image, key1.codepoint, key2.codepoint, glyph1->id, glyph2->id);
		}
	}
}

static int font_image_sort_comparison(void const * v1, void const * v2);
void font_image_render(struct Font_Image * font_image) {
	uint32_t const padding = 1;

	// track glyphs usage
	FOR_HASH_TABLE_U64 (&font_image->table, it) {
		if (it.key_hash == CODEPOINT_EMPTY) { continue; }
		struct Font_Glyph * glyph = it.value;

		if (glyph->usage == 0) {
			font_image->rendered = false;
			hash_table_u64_del_at(&font_image->table, it.current);
			continue;
		}

		glyph->usage--;
	}

	if (font_image->rendered) { return; }
	font_image->rendered = true;

	// collect visible glyphs
	uint32_t symbols_count = 0;
	struct Font_Symbol * symbols_to_render = MEMORY_ALLOCATE_ARRAY(struct Font_Symbol, font_image->table.count);

	FOR_HASH_TABLE_U64 (&font_image->table, it) {
		struct Font_Glyph const * glyph = it.value;
		if (glyph->params.is_empty) { continue; }

		symbols_to_render[symbols_count] = (struct Font_Symbol){
			.key = font_image_get_key(it.key_hash),
			.glyph = it.value,
		};
		symbols_count++;
	}

	// sort glyphs by height, then by width
	common_qsort(symbols_to_render, symbols_count, sizeof(*symbols_to_render), font_image_sort_comparison);

	// resize the atlas
	{
		// estimate the very minimum area
		uint32_t minimum_area = 0;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Font_Symbol const * symbol = symbols_to_render + i;
			struct Glyph_Params const * params = &symbol->glyph->params;
			uint32_t const glyph_size_x = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[2] - params->rect[0]) : 1;
			uint32_t const glyph_size_y = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[3] - params->rect[1]) : 1;
			minimum_area += (glyph_size_x + padding) * (glyph_size_y + padding);
		}

		// estimate required atlas dimesions
		uint32_t atlas_size_x;
		uint32_t atlas_size_y;
		if (font_image->buffer.size_x == 0 || font_image->buffer.size_y == 0) {
			atlas_size_x = (uint32_t)maths_sqrt((float)minimum_area);
			atlas_size_x = round_up_to_PO2_u32(atlas_size_x);

			atlas_size_y = atlas_size_x;
			if (atlas_size_x * (atlas_size_y / 2) > minimum_area) {
				atlas_size_y = atlas_size_y / 2;
			}
		}
		else {
			atlas_size_y = font_image->buffer.size_y;

			atlas_size_x = minimum_area / atlas_size_y;
			atlas_size_x = round_up_to_PO2_u32(atlas_size_y);
			if (atlas_size_x < font_image->buffer.size_x) {
				atlas_size_x = font_image->buffer.size_x;
			}
		}

		// verify estimated atlas dimensions
		verify_dimensions:
		uint32_t line_height = 0;
		uint32_t offset_x = padding, offset_y = padding;
		for (uint32_t i = 0; i < symbols_count; i++) {

			struct Font_Symbol const * symbol = symbols_to_render + i;
			struct Glyph_Params const * params = &symbol->glyph->params;

			uint32_t const glyph_size_x = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[2] - params->rect[0]) : 1;
			uint32_t const glyph_size_y = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[3] - params->rect[1]) : 1;

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

		image_ensure(&font_image->buffer, atlas_size_x, atlas_size_y);
	}

	// render glyphs into the atlas, assuming they shall fit
	uint32_t const buffer_data_size = data_type_get_size(font_image->buffer.parameters.data_type);
	common_memset(font_image->buffer.data, 0, font_image->buffer.size_x * font_image->buffer.size_y * buffer_data_size);
	{
		uint32_t line_height = 0;
		uint32_t offset_x = padding, offset_y = padding;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Font_Symbol const * symbol = symbols_to_render + i;
			struct Glyph_Params const * params = &symbol->glyph->params;

			uint32_t const glyph_size_x = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[2] - params->rect[0]) : 1;
			uint32_t const glyph_size_y = (symbol->key.codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[3] - params->rect[1]) : 1;

			if (line_height == 0) { line_height = glyph_size_y; }

			if (offset_x + glyph_size_x + padding > font_image->buffer.size_x) {
				offset_x = padding;
				offset_y += line_height + padding;
				line_height = glyph_size_y;
			}

			if (font_image->buffer.size_x < offset_x + glyph_size_x - 1) {
				logger_to_console("can't fit a glyph into the buffer"); DEBUG_BREAK(); continue;
			}
			if (font_image->buffer.size_y < offset_y + glyph_size_y - 1) {
				logger_to_console("can't fit a glyph into the buffer"); DEBUG_BREAK(); continue;
			}

			//
			struct Font_Glyph * glyph = symbol->glyph;
			glyph->uv[0] = (float)(offset_x)                / (float)font_image->buffer.size_x;
			glyph->uv[1] = (float)(offset_y)                / (float)font_image->buffer.size_y;
			glyph->uv[2] = (float)(offset_x + glyph_size_x) / (float)font_image->buffer.size_x;
			glyph->uv[3] = (float)(offset_y + glyph_size_y) / (float)font_image->buffer.size_y;

			font_fill_buffer(
				font_image->font,
				glyph->id, font_get_scale(font_image->font, symbol->key.size),
				font_image->buffer.data, font_image->buffer.size_x,
				glyph_size_x, glyph_size_y,
				offset_x, offset_y
			);

			offset_x += glyph_size_x + padding;
		}
	}

	MEMORY_FREE(symbols_to_render);
}

struct Image const * font_image_get_asset(struct Font_Image const * font_image) {
	return &font_image->buffer;
}

inline static uint64_t font_image_get_key_hash(struct Font_Key value);
struct Font_Glyph const * font_image_get_glyph(struct Font_Image * const font_image, uint32_t codepoint, float size) {
	return hash_table_u64_get(&font_image->table, font_image_get_key_hash((struct Font_Key){
		.codepoint = codepoint,
		.size = size,
	}));
}

// 

float font_image_get_scale(struct Font_Image const * font_image, float size) {
	return font_get_scale(font_image->font, size);
}

float font_image_get_ascent(struct Font_Image const * font_image, float scale) {
	int32_t const value = font_get_ascent(font_image->font);
	return ((float)value) * scale;
}

float font_image_get_descent(struct Font_Image const * font_image, float scale) {
	int32_t const value = font_get_descent(font_image->font);
	return ((float)value) * scale;
}

float font_image_get_gap(struct Font_Image const * font_image, float scale) {
	int32_t const value = font_get_gap(font_image->font);
	return ((float)value) * scale;
}

float font_image_get_kerning(struct Font_Image const * font_image, uint32_t codepoint1, uint32_t codepoint2, float scale) {
	uint64_t const key_hash = ((uint64_t)codepoint2 << 32) | (uint64_t)codepoint1;
	int32_t const * value = hash_table_u64_get(&font_image->kerning, key_hash);
	return (value != NULL) 
		? ((float)*value) * scale
		: 0;
}

//

static int font_image_sort_comparison(void const * v1, void const * v2) {
	struct Font_Symbol const * s1 = v1;
	struct Font_Symbol const * s2 = v2;

	if (s1->key.codepoint == CODEPOINT_EMPTY) { return  1; }
	if (s2->key.codepoint == CODEPOINT_EMPTY) { return -1; }

	int32_t const * r1 = s1->glyph->params.rect;
	int32_t const * r2 = s2->glyph->params.rect;

	uint32_t const size_y_1 = (uint32_t)(r1[3] - r1[1]);
	uint32_t const size_y_2 = (uint32_t)(r2[3] - r2[1]);

	if (size_y_1 < size_y_2) { return  1; }
	if (size_y_1 > size_y_2) { return -1; }

	uint32_t const size_x_1 = (uint32_t)(r1[2] - r1[0]);
	uint32_t const size_x_2 = (uint32_t)(r2[2] - r2[0]);

	if (size_x_1 < size_x_2) { return  1; }
	if (size_x_1 > size_x_2) { return -1; }

	if (s1->key.codepoint < s2->key.codepoint) { return  1; }
	if (s1->key.codepoint > s2->key.codepoint) { return -1; }

	return 0;
}

inline static struct Font_Key font_image_get_key(uint64_t value) {
	union {
		uint64_t value_u64;
		struct Font_Key value_key;
	} data;
	data.value_u64 = value;
	return data.value_key;
}

inline static uint64_t font_image_get_key_hash(struct Font_Key value) {
	union {
		uint64_t value_u64;
		struct Font_Key value_key;
	} data;
	data.value_key = value;
	return data.value_u64;
}

inline static void font_image_add_glyph(struct Font_Image * font_image, uint32_t codepoint, float size) {
	uint64_t const key_hash = font_image_get_key_hash((struct Font_Key){
		.codepoint = codepoint,
		.size = size,
	});

	struct Font_Glyph * glyph = hash_table_u64_get(&font_image->table, key_hash);
	if (glyph != NULL) { glyph->usage = GLYPH_USAGE_MAX; return; }

	uint32_t const glyph_id = font_get_glyph_id(font_image->font, codepoint);
	if (glyph_id == 0) { logger_to_console("font misses a glyph for codepoint '0x%x'\n", codepoint); DEBUG_BREAK(); return; }

	struct Glyph_Params const glyph_params = font_get_glyph_parameters(
		font_image->font, glyph_id, font_get_scale(font_image->font, size)
	);

	hash_table_u64_set(&font_image->table, key_hash, &(struct Font_Glyph){
		.params = glyph_params,
		.id = glyph_id,
		.usage = GLYPH_USAGE_MAX,
	});

	font_image->rendered = false;
}

inline static void font_image_add_kerning(struct Font_Image * font_image, uint32_t codepoint1, uint32_t codepoint2, uint32_t glyph1, uint32_t glyph2) {
	uint64_t const key_hash = ((uint64_t)codepoint2 << 32) | (uint64_t)codepoint1;
	if (hash_table_u64_get(&font_image->kerning, key_hash) != NULL) { return; }

	int32_t const value = font_get_kerning(font_image->font, glyph1, glyph2);
	if (value == 0) { return; }

	hash_table_u64_set(&font_image->kerning, key_hash, &value);
}

#undef GLYPH_USAGE_MAX
