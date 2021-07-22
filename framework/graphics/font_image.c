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
	struct Font * font; // weak reference
	struct Hash_Table_U32 table;
	struct Hash_Table_U64 kerning;
	float scale;
};

//
#include "font_image.h"
#include "font_image_glyph.h"

struct Font_Symbol {
	struct Font_Glyph * glyph;
	uint32_t codepoint;
};

struct Font_Image * font_image_init(struct Font * font, int32_t size) {
	struct Font_Image * font_image = MEMORY_ALLOCATE(NULL, struct Font_Image);
	*font_image = (struct Font_Image){
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
	return font_image;
}

void font_image_free(struct Font_Image * font_image) {
	MEMORY_FREE(font_image, font_image->buffer.data);
	hash_table_u32_free(&font_image->table);
	hash_table_u64_free(&font_image->kerning);

	memset(font_image, 0, sizeof(*font_image));
	MEMORY_FREE(font_image, font_image);
}

void font_image_clear(struct Font_Image * font_image) {
	hash_table_u32_clear(&font_image->table);
	memset(font_image->buffer.data, 0, sizeof(*font_image->buffer.data) * font_image->buffer.size_x * font_image->buffer.size_y);

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
	});
}

struct Image * font_image_get_asset(struct Font_Image * font_image) {
	return &font_image->buffer;
}

void font_image_add_range(struct Font_Image * font_image, uint32_t from, uint32_t to) {
	for (uint32_t codepoint = from; codepoint <= to; codepoint++) {
		if (hash_table_u32_get(&font_image->table, codepoint) != NULL) { continue; }

		uint32_t const glyph_id = font_get_glyph_id(font_image->font, codepoint);
		if (glyph_id == 0) { continue; }

		struct Glyph_Params glyph_params;
		font_get_glyph_parameters(font_image->font, &glyph_params, glyph_id, font_image->scale);

		hash_table_u32_set(&font_image->table, codepoint, &(struct Font_Glyph){
			.params = glyph_params,
			.id = glyph_id,
		});
	}
}

void font_image_add_text(struct Font_Image * font_image, uint32_t length, uint8_t const * data) {
	for (uint32_t i = 0; i < length; /*see `continue_loop:`*/) {
		uint32_t const octets_count = utf8_length(data + i);
		if (octets_count == 0) { goto continue_loop; }
	
		uint32_t const codepoint = utf8_decode(data + i, octets_count);
		if (codepoint == CODEPOINT_EMPTY) { goto continue_loop; }

		// control
		switch (codepoint) {
			case 0x200b: goto continue_loop; // zero-width space
		}

		if (codepoint < ' ') { goto continue_loop; }

		switch (codepoint) {
			case 0xa0: font_image_add_range(font_image, ' ', ' '); continue; // non-breaking space
			default: font_image_add_range(font_image, codepoint, codepoint); break;
		}

		//
		continue_loop:
		i += (octets_count > 0) ? octets_count : 1;
	}
}

static int font_image_sort_comparison(void const * v1, void const * v2);
void font_image_build(struct Font_Image * font_image) {
	uint32_t const padding = 1;

	//
	uint32_t symbols_count = 0;
	struct Font_Symbol * symbols = MEMORY_ALLOCATE_ARRAY(font_image, struct Font_Symbol, font_image->table.count);

	struct Hash_Table_U32_Entry it = {0};
	while (hash_table_u32_iterate(&font_image->table, &it)) {
		symbols[symbols_count] = (struct Font_Symbol){
			.glyph = it.value,
			.codepoint = it.key_hash,
		};
		symbols_count++;
	}

	//
	for (uint32_t i1 = 0; i1 < symbols_count; i1++) {
		struct Font_Symbol const * symbol1 = symbols + i1;
		if (symbol1->codepoint == CODEPOINT_EMPTY) { continue; }
		for (uint32_t i2 = 0; i2 < symbols_count; i2++) {
			struct Font_Symbol const * symbol2 = symbols + i2;
			if (symbol2->codepoint == CODEPOINT_EMPTY) { continue; }
			int32_t const value = font_get_kerning(font_image->font, symbol1->glyph->id, symbol2->glyph->id);
			if (value == 0) { continue; }

			uint64_t const key_hash = ((uint64_t)symbol2->codepoint << 32) | (uint64_t)symbol1->codepoint;
			float const value_float = ((float)value) * font_image->scale;
			hash_table_u64_set(&font_image->kerning, key_hash, &value_float);
		}
	}

	// sort glyphs by height, then by width
	qsort(symbols, symbols_count, sizeof(*symbols), font_image_sort_comparison);

	// resize the atlas
	uint32_t atlas_size_x, atlas_size_y;
	{
		// estimate the very minimum area
		uint32_t minimum_area = 0;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Font_Symbol const * symbol = symbols + i;
			struct Glyph_Params const * params = &symbol->glyph->params;
			uint32_t const size_x = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[2] - params->rect[0]) : 1;
			uint32_t const size_y = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[3] - params->rect[1]) : 1;
			minimum_area += (size_x + 1) * (size_y + 1);
		}

		// estimate required atlas dimesions
		atlas_size_x = (uint32_t)sqrtf((float)minimum_area);
		atlas_size_x = round_up_to_PO2_u32(atlas_size_x);
		if (atlas_size_x > 0x1000) {
			atlas_size_x = 0x1000;
			logger_to_console("too many codepoints or symbols are too large\n"); DEBUG_BREAK(); return;
		}

		atlas_size_y = atlas_size_x;
		if (atlas_size_x * (atlas_size_y / 2) > minimum_area) {
			atlas_size_y = atlas_size_y / 2;
		}

		// verify estimated atlas dimensions
		uint32_t line_height = 0;
		uint32_t offset_x = padding, offset_y = padding;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Font_Symbol const * symbol = symbols + i;
			struct Glyph_Params const * params = &symbol->glyph->params;
			if (params->is_empty) { continue; }

			uint32_t const size_x = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[2] - params->rect[0]) : 1;
			uint32_t const size_y = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[3] - params->rect[1]) : 1;

			if (atlas_size_x < size_x) { logger_to_console("atlas's too small\n"); DEBUG_BREAK(); break; }
			if (atlas_size_y < size_y) { logger_to_console("atlas's too small\n"); DEBUG_BREAK(); break; }

			if (line_height == 0) { line_height = size_y; }

			if (offset_x + size_x + padding > atlas_size_x) {
				offset_x = padding;
				offset_y += line_height + padding;
				line_height = size_y;
			}

			if (offset_y + line_height + padding > atlas_size_y) {
				atlas_size_y = atlas_size_y * 2; break;
			}

			offset_x += size_x + padding;
		}

		// @todo: keep the maximum size without shrinking?
		font_image->buffer.size_x = atlas_size_x;
		font_image->buffer.size_y = atlas_size_y;
		font_image->buffer.data = MEMORY_REALLOCATE_ARRAY(font_image, font_image->buffer.data, atlas_size_x * atlas_size_y);
	}

	// pack glyphs into the atlas, assuming they are sorted by height
	{
		struct Array_Byte scratch_buffer;
		array_byte_init(&scratch_buffer);

		uint32_t line_height = 0;
		uint32_t offset_x = padding, offset_y = padding;
		for (uint32_t i = 0; i < symbols_count; i++) {
			struct Font_Symbol const * symbol = symbols + i;
			struct Glyph_Params const * params = &symbol->glyph->params;
			if (params->is_empty) { continue; }

			uint32_t const size_x = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[2] - params->rect[0]) : 1;
			uint32_t const size_y = (symbol->codepoint != CODEPOINT_EMPTY) ? (uint32_t)(params->rect[3] - params->rect[1]) : 1;

			if (line_height == 0) { line_height = size_y; }

			if (offset_x + size_x + padding > atlas_size_x) {
				offset_x = padding;
				offset_y += line_height + padding;
				line_height = size_y;
			}

			//
			symbol->glyph->uv[0] = (float)(offset_x)          / (float)atlas_size_x;
			symbol->glyph->uv[1] = (float)(offset_y)          / (float)atlas_size_y;
			symbol->glyph->uv[2] = (float)(offset_x + size_x) / (float)atlas_size_x;
			symbol->glyph->uv[3] = (float)(offset_y + size_y) / (float)atlas_size_y;

			//
			if (scratch_buffer.capacity < size_x * size_y) {
				array_byte_resize(&scratch_buffer, size_x * size_y);
			}

			// @todo: receive an `invert_y` flag?
			if (symbol->codepoint != CODEPOINT_EMPTY) {
				font_fill_buffer(
					font_image->font,
					scratch_buffer.data, size_x,
					symbol->glyph->id, size_x, size_y, font_image->scale
				);
			}
			else {
				scratch_buffer.data[0] = 0xff;
			}

			for (uint32_t y = 0; y < size_y; y++) {
				memcpy(
					font_image->buffer.data + ((offset_y + y) * atlas_size_x + offset_x),
					scratch_buffer.data + ((size_y - y - 1) * size_x),
					sizeof(*scratch_buffer.data) * size_x
				);
			}

			offset_x += size_x + padding;
		}

		array_byte_free(&scratch_buffer);
	}

	MEMORY_FREE(font_image, symbols);
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

	return 0;
}
