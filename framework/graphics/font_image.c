#include "framework/containers/array_byte.h"
#include "framework/containers/hash_table_u32.h"
#include "framework/containers/hash_table_u64.h"

#include "framework/assets/image.h"
#include "framework/assets/font.h"
#include "framework/assets/font_glyph.h"

#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/unicode.h"

// #include "framework/maths.h"
uint32_t round_up_to_PO2_u32(uint32_t value);

#include <string.h>
#include <stdlib.h>
#include <math.h>

struct Font_Image {
	struct Image buffer;
	//
	WEAK_PTR(struct Font const) font;
	struct Hash_Table_U32 table;
	struct Hash_Table_U64 kerning;
	float scale;
	bool rendered;
};

//
#include "font_image.h"

struct Font_Symbol {
	WEAK_PTR(struct Font_Glyph) glyph;
	uint32_t codepoint;
};

struct Glyph_Codepoint {
	uint32_t glyph, codepoint;
};

struct Font_Image * font_image_init(struct Font const * font, int32_t size) {
	struct Font_Image * font_image = MEMORY_ALLOCATE(NULL, struct Font_Image);
	*font_image = (struct Font_Image){
		// @todo: provide a contructor?
		.buffer = {
			.parameters = {
				.texture_type = TEXTURE_TYPE_COLOR,
				.data_type = DATA_TYPE_U8,
				.flags = TEXTURE_FLAG_MUTABLE | TEXTURE_FLAG_WRITE,
				.channels = 1,
			},
		},
		.scale = font_get_scale(font, (float)size),
		.font = font,
	};
	hash_table_u32_init(&font_image->table, sizeof(struct Font_Glyph));
	hash_table_u64_init(&font_image->kerning, sizeof(float));

	// setup an error glyph
	float const size_error_y = font_image_get_height(font_image);
	float const size_error_x = size_error_y / 2;
	float const scale_error[] = {0.1f, 0.0f, 0.9f, 0.7f};

	hash_table_u32_set(&font_image->table, CODEPOINT_EMPTY, &(struct Font_Glyph){
		.params = (struct Glyph_Params){
			.full_size_x = size_error_x,
			.rect[0] = (int32_t)(size_error_x * scale_error[0]),
			.rect[1] = (int32_t)(size_error_y * scale_error[1]),
			.rect[2] = (int32_t)(size_error_x * scale_error[2]),
			.rect[3] = (int32_t)(size_error_y * scale_error[3]),
		},
		// .usage = UINT8_MAX,
	});

	return font_image;
}

void font_image_free(struct Font_Image * font_image) {
	image_free(&font_image->buffer);
	hash_table_u32_free(&font_image->table);
	hash_table_u64_free(&font_image->kerning);

	memset(font_image, 0, sizeof(*font_image));
	MEMORY_FREE(font_image, font_image);
}

inline static void font_image_add_glyph(struct Font_Image * font_image, uint32_t codepoint);
void font_image_add_glyphs_from_range(struct Font_Image * font_image, uint32_t from, uint32_t to) {
	for (uint32_t codepoint = from; codepoint <= to; codepoint++) {
		font_image_add_glyph(font_image, codepoint);
	}
}

void font_image_add_glyphs_from_text(struct Font_Image * font_image, uint32_t length, uint8_t const * data) {
	for (struct UTF8_Iterator it = {0}; utf8_iterate(length, data, &it); /*empty*/) {
		switch (it.codepoint) {
			case CODEPOINT_ZERO_WIDTH_SPACE: break;

			case ' ':
			case CODEPOINT_NON_BREAKING_SPACE:
				font_image_add_glyph(font_image, ' ');
				break;

			default: if (it.codepoint > ' ') {
				font_image_add_glyph(font_image, it.codepoint);
			} break;
		}
	}
}

inline static void font_image_add_kerning(struct Font_Image * font_image, uint32_t codepoint1, uint32_t codepoint2, uint32_t glyph1, uint32_t glyph2);
void font_image_add_kerning_from_range(struct Font_Image * font_image, uint32_t from, uint32_t to) {
	uint32_t * glyphs = MEMORY_ALLOCATE_ARRAY(font_image, uint32_t, to - from + 1);

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

	MEMORY_FREE(font_image, glyphs);
}

void font_image_add_kerning_from_text(struct Font_Image * font_image, uint32_t length, uint8_t const * data) {
	uint32_t codepoints_count = 0;
	for (struct UTF8_Iterator it = {0}; utf8_iterate(length, data, &it); /*empty*/) { codepoints_count++; }

	struct Glyph_Codepoint * pairs = MEMORY_ALLOCATE_ARRAY(font_image, struct Glyph_Codepoint, codepoints_count);

	uint32_t count = 0;
	for (struct UTF8_Iterator it = {0}; utf8_iterate(length, data, &it); /*empty*/) {
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

	MEMORY_FREE(font_image, pairs);
}

void font_image_add_kerning_all(struct Font_Image * font_image) {
	hash_table_u64_clear(&font_image->kerning);
	for (struct Hash_Table_U32_Iterator it1 = {0}; hash_table_u32_iterate(&font_image->table, &it1); /*empty*/) {
		struct Font_Glyph const * glyph1 = it1.value;
		for (struct Hash_Table_U32_Iterator it2 = {0}; hash_table_u32_iterate(&font_image->table, &it2); /*empty*/) {
			struct Font_Glyph const * glyph2 = it2.value;
			font_image_add_kerning(font_image, it1.key_hash, it2.key_hash, glyph1->id, glyph2->id);
		}
	}
}

static int font_image_sort_comparison(void const * v1, void const * v2);
void font_image_render(struct Font_Image * font_image) {
	uint32_t const padding = 1;

	// track glyphs usage
	for (struct Hash_Table_U32_Iterator it = {0}; hash_table_u32_iterate(&font_image->table, &it); /*empty*/) {
		if (it.key_hash == CODEPOINT_EMPTY) { continue; }
		struct Font_Glyph * glyph = it.value;

		if (glyph->usage == 0) {
			font_image->rendered = false;
			hash_table_u32_del_at(&font_image->table, it.current);
			continue;
		}

		glyph->usage--;
	}

	if (font_image->rendered) { return; }
	font_image->rendered = true;

	// collect visible glyphs
	uint32_t symbols_count = 0;
	struct Font_Symbol * symbols = MEMORY_ALLOCATE_ARRAY(font_image, struct Font_Symbol, font_image->table.count);

	for (struct Hash_Table_U32_Iterator it = {0}; hash_table_u32_iterate(&font_image->table, &it); /*empty*/) {
		struct Font_Glyph const * glyph = it.value;
		if (glyph->params.is_empty) { continue; }

		symbols[symbols_count] = (struct Font_Symbol){
			.glyph = it.value,
			.codepoint = it.key_hash,
		};
		symbols_count++;
	}

	// sort glyphs by height, then by width
	qsort(symbols, symbols_count, sizeof(*symbols), font_image_sort_comparison);

	// resize the atlas
	{
		// estimate the very minimum area
		uint32_t minimum_area = 0;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Font_Symbol const * symbol = symbols + i;
			struct Glyph_Params const * params = &symbol->glyph->params;
			uint32_t const glyph_size_x = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[2] - params->rect[0]) : 1;
			uint32_t const glyph_size_y = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[3] - params->rect[1]) : 1;
			minimum_area += (glyph_size_x + padding) * (glyph_size_y + padding);
		}

		// estimate required atlas dimesions
		uint32_t atlas_size_x;
		uint32_t atlas_size_y;
		if (font_image->buffer.size_x == 0 || font_image->buffer.size_y == 0) {
			atlas_size_x = (uint32_t)sqrtf((float)minimum_area);
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
		// oh no, a `goto` label, think of the children!
		verify_dimensions: // `goto` is this way vvvvv;
		uint32_t line_height = 0;
		uint32_t offset_x = padding, offset_y = padding;
		for (uint32_t i = 0; i < symbols_count; i++) {

			struct Font_Symbol const * symbol = symbols + i;
			struct Glyph_Params const * params = &symbol->glyph->params;

			uint32_t const glyph_size_x = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[2] - params->rect[0]) : 1;
			uint32_t const glyph_size_y = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[3] - params->rect[1]) : 1;

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
					atlas_size_x = atlas_size_x * 2;
					goto verify_dimensions; // the label is that way ^^^^^
				}
			}

			offset_x += glyph_size_x + padding;
		}

		if (font_image->buffer.capacity < atlas_size_x * atlas_size_y) {
			// @todo: provide a `resize` method
			font_image->buffer.data = MEMORY_REALLOCATE_ARRAY(font_image, font_image->buffer.data, atlas_size_x * atlas_size_y);
			font_image->buffer.capacity = atlas_size_x * atlas_size_y;
		}
		font_image->buffer.size_x = atlas_size_x;
		font_image->buffer.size_y = atlas_size_y;
	}

	// render glyphs into the atlas, assuming they shall fit
	memset(font_image->buffer.data, 0, sizeof(*font_image->buffer.data) * font_image->buffer.size_x * font_image->buffer.size_y);
	{
		struct Array_Byte scratch_buffer;
		array_byte_init(&scratch_buffer);

		uint32_t line_height = 0;
		uint32_t offset_x = padding, offset_y = padding;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Font_Symbol const * symbol = symbols + i;
			struct Glyph_Params const * params = &symbol->glyph->params;

			uint32_t const glyph_size_x = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[2] - params->rect[0]) : 1;
			uint32_t const glyph_size_y = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[3] - params->rect[1]) : 1;

			if (font_image->buffer.capacity < ((offset_y + glyph_size_y - 1) * font_image->buffer.size_x + offset_x + glyph_size_x - 1)) {
				logger_to_console("can't fit a glyph into the buffer"); DEBUG_BREAK(); continue;
			}

			if (line_height == 0) { line_height = glyph_size_y; }

			if (offset_x + glyph_size_x + padding > font_image->buffer.size_x) {
				offset_x = padding;
				offset_y += line_height + padding;
				line_height = glyph_size_y;
			}

			//
			symbol->glyph->uv[0] = (float)(offset_x)          / (float)font_image->buffer.size_x;
			symbol->glyph->uv[1] = (float)(offset_y)          / (float)font_image->buffer.size_y;
			symbol->glyph->uv[2] = (float)(offset_x + glyph_size_x) / (float)font_image->buffer.size_x;
			symbol->glyph->uv[3] = (float)(offset_y + glyph_size_y) / (float)font_image->buffer.size_y;

			//
			if (scratch_buffer.capacity < glyph_size_x * glyph_size_y) {
				array_byte_resize(&scratch_buffer, glyph_size_x * glyph_size_y);
			}

			font_fill_buffer(
				font_image->font,
				scratch_buffer.data, glyph_size_x,
				symbol->glyph->id, glyph_size_x, glyph_size_y, font_image->scale
			);

			for (uint32_t glyph_y = 0; glyph_y < glyph_size_y; glyph_y++) {
				// @note: expects glyphs to be rendered bottom-left -> top-right
				memcpy(
					font_image->buffer.data + ((offset_y + glyph_y) * font_image->buffer.size_x + offset_x),
					scratch_buffer.data + glyph_y * glyph_size_x,
					sizeof(*scratch_buffer.data) * glyph_size_x
				);
			}

			offset_x += glyph_size_x + padding;
		}

		array_byte_free(&scratch_buffer);
	}

	MEMORY_FREE(font_image, symbols);
}

struct Image const * font_image_get_asset(struct Font_Image * font_image) {
	return &font_image->buffer;
}

struct Font_Glyph const * font_image_get_glyph(struct Font_Image * const font_image, uint32_t codepoint) {
	return hash_table_u32_get(&font_image->table, codepoint);
}

float font_image_get_height(struct Font_Image * font_image) {
	int32_t value = font_get_height(font_image->font);
	return ((float)value) * font_image->scale;
}

float font_image_get_gap(struct Font_Image * font_image) {
	int32_t value = font_get_gap(font_image->font);
	return ((float)value) * font_image->scale;
}

float font_image_get_kerning(struct Font_Image * font_image, uint32_t codepoint1, uint32_t codepoint2) {
	uint64_t const key_hash = ((uint64_t)codepoint2 << 32) | (uint64_t)codepoint1;
	float const * value = hash_table_u64_get(&font_image->kerning, key_hash);
	return (value != NULL) ? *value : 0;
}

//

static int font_image_sort_comparison(void const * v1, void const * v2) {
	struct Font_Symbol const * s1 = v1;
	struct Font_Symbol const * s2 = v2;

	if (s1->codepoint == CODEPOINT_EMPTY) { return  1; }
	if (s2->codepoint == CODEPOINT_EMPTY) { return -1; }

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

	if (s1->codepoint < s2->codepoint) { return  1; }
	if (s1->codepoint > s2->codepoint) { return -1; }

	return 0;
}

inline static void font_image_add_glyph(struct Font_Image * font_image, uint32_t codepoint) {
	struct Font_Glyph * glyph = hash_table_u32_get(&font_image->table, codepoint);
	if (glyph != NULL) { if (glyph->usage < UINT8_MAX) { glyph->usage++; } return; }

	uint32_t const glyph_id = font_get_glyph_id(font_image->font, codepoint);
	if (glyph_id == 0) { logger_to_console("font misses a glyph for codepoint 0x%x\n", codepoint); DEBUG_BREAK(); return; }

	struct Glyph_Params glyph_params;
	font_get_glyph_parameters(font_image->font, &glyph_params, glyph_id, font_image->scale);

	font_image->rendered = false;
	hash_table_u32_set(&font_image->table, codepoint, &(struct Font_Glyph){
		.params = glyph_params,
		.id = glyph_id,
		.usage = UINT8_MAX,
	});
}

inline static void font_image_add_kerning(struct Font_Image * font_image, uint32_t codepoint1, uint32_t codepoint2, uint32_t glyph1, uint32_t glyph2) {
	uint64_t const key_hash = ((uint64_t)codepoint2 << 32) | (uint64_t)codepoint1;
	if (hash_table_u64_get(&font_image->kerning, key_hash) != NULL) { return; }

	int32_t const value = font_get_kerning(font_image->font, glyph1, glyph2);
	if (value == 0) { return; }

	float const value_float = ((float)value) * font_image->scale;
	hash_table_u64_set(&font_image->kerning, key_hash, &value_float);
}
