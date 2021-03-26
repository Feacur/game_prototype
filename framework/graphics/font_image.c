#include "framework/containers/array_byte.h"
#include "framework/containers/hash_table.h"
#include "framework/assets/asset_image.h"
#include "framework/assets/asset_font.h"
#include "framework/assets/asset_font_glyph.h"
#include "framework/memory.h"

// #include "framework/maths.h"
uint32_t round_up_to_PO2_u32(uint32_t value);

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

struct Font_Image {
	struct Asset_Image buffer;
	struct Asset_Font * asset_font; // weak reference
	struct Hash_Table * table;
	float scale;
};

//
#include "font_image.h"
#include "font_image_glyph.h"

struct Font_Symbol {
	struct Font_Glyph glyph;
	uint32_t codepoint;
};

struct Font_Image * font_image_init(struct Asset_Font * asset_font, int32_t size) {
	struct Font_Image * font_image = MEMORY_ALLOCATE(struct Font_Image);
	*font_image = (struct Font_Image){
		.buffer = {
			.parameters = {
				.texture_type = TEXTURE_TYPE_COLOR,
				.data_type = DATA_TYPE_U8,
				.flags = TEXTURE_FLAG_MUTABLE | TEXTURE_FLAG_WRITE,
				.channels = 1,
			},
		},
		.table = hash_table_init(sizeof(struct Font_Glyph)),
		.scale = asset_font_get_scale(asset_font, (float)size),
		.asset_font = asset_font,
	};
	return font_image;
}

void font_image_free(struct Font_Image * font_image) {
	MEMORY_FREE(font_image->buffer.data);
	hash_table_free(font_image->table);

	memset(font_image, 0, sizeof(*font_image));
	MEMORY_FREE(font_image);
}

void font_image_clear(struct Font_Image * font_image) {
	hash_table_clear(font_image->table);
	memset(font_image->buffer.data, 0, font_image->buffer.size_x * font_image->buffer.size_y * sizeof(*font_image->buffer.data));
}

struct Asset_Image * font_image_get_asset(struct Font_Image * font_image) {
	return &font_image->buffer;
}

static int font_image_sort_comparison(void const * v1, void const * v2);
void font_image_build(struct Font_Image * font_image, uint32_t const * codepoint_ranges) {
	if (codepoint_ranges == NULL) { return; }

	uint32_t codepoints_count = 0;
	for (uint32_t const *range = codepoint_ranges; *range != 0; range += 2) {
		codepoints_count += 1 + (range[1] - range[0]);
	}

	if (codepoints_count == 0) { return; }

	uint32_t symbols_count = 0;
	struct Font_Symbol * symbols = MEMORY_ALLOCATE_ARRAY(struct Font_Symbol, codepoints_count + 1);

	// collect glyphs
	for (uint32_t const *range = codepoint_ranges; *range != 0; range += 2) {
		for (uint32_t codepoint = *range, codepoint_to = range[1]; codepoint <= codepoint_to; codepoint++) {
			uint32_t const glyph_id = asset_font_get_glyph_id(font_image->asset_font, codepoint);
			if (glyph_id == 0) { continue; }

			struct Glyph_Params glyph_params;
			asset_font_get_glyph_parameters(font_image->asset_font, &glyph_params, glyph_id, font_image->scale);

			symbols[symbols_count++] = (struct Font_Symbol){
				.glyph = {
					.params = glyph_params,
					.id = glyph_id,
				},
				.codepoint = codepoint,
			};
		}
	}
	symbols[symbols_count].glyph.id = 0;

	// sort glyphs by height, then by width
	qsort(symbols, symbols_count, sizeof(*symbols), font_image_sort_comparison);

	// resize the atlas
	{
		// estimate the very minimum area
		uint32_t minimum_area = 0;
		for (struct Font_Symbol const * symbol = symbols; symbol->glyph.id != 0; symbol++) {
			int32_t const * rect = symbol->glyph.params.rect;
			uint32_t const size_x = (uint32_t)(rect[2] - rect[0]);
			uint32_t const size_y = (uint32_t)(rect[3] - rect[1]);
			minimum_area += (size_x + 1) * (size_y + 1);
		}

		// estimate required atlas dimesions
		uint32_t atlas_size_x = (uint32_t)sqrtf((float)minimum_area);
		atlas_size_x = round_up_to_PO2_u32(atlas_size_x);
		if (atlas_size_x > 0x1000) {
			atlas_size_x = 0x1000;
			fprintf(stderr, "too many codepoints or symbols are too large\n"); DEBUG_BREAK(); return;
		}

		uint32_t atlas_size_y = atlas_size_x;
		if (atlas_size_x * (atlas_size_y / 2) > minimum_area) {
			atlas_size_y = atlas_size_y / 2;
		}

		// @todo: keep the maximum size without shrinking?
		font_image->buffer.size_x = atlas_size_x;
		font_image->buffer.size_y = atlas_size_y;
		font_image->buffer.data = MEMORY_REALLOCATE_ARRAY(font_image->buffer.data, atlas_size_x * atlas_size_y);
	}

	// pack glyphs into the atlas, assuming they are sorted by height
	font_image_clear(font_image);

	{
		uint32_t const padding = 1;

		struct Array_Byte scratch_buffer;
		array_byte_init(&scratch_buffer);

		uint32_t line_height = 0;
		uint32_t offset_x = padding, offset_y = padding;
		for (struct Font_Symbol * symbol = symbols; symbol->glyph.id != 0; symbol++) {
			struct Glyph_Params const * params = &symbol->glyph.params;
			if (params->is_empty) { continue; }

			uint32_t const size_x = (uint32_t)(params->rect[2] - params->rect[0]);
			uint32_t const size_y = (uint32_t)(params->rect[3] - params->rect[1]);

			if (line_height == 0) { line_height = size_y; }

			if (offset_x + size_x + padding > font_image->buffer.size_x) {
				offset_x = padding;
				offset_y += line_height + padding;
				line_height = size_y;
			}

			if (offset_y + line_height + padding > font_image->buffer.size_y) {
				// @todo: fallback once to a larger area?
				fprintf(stderr, "atlas's too small\n"); DEBUG_BREAK(); break;
			}

			//
			if (font_image->buffer.size_x < size_x) { fprintf(stderr, "atlas's too small\n"); DEBUG_BREAK(); break; }
			if (font_image->buffer.size_y < size_y) { fprintf(stderr, "atlas's too small\n"); DEBUG_BREAK(); break; }

			symbol->glyph.uv[0] = (float)(offset_x)          / (float)font_image->buffer.size_x;
			symbol->glyph.uv[1] = (float)(offset_y)          / (float)font_image->buffer.size_y;
			symbol->glyph.uv[2] = (float)(offset_x + size_x) / (float)font_image->buffer.size_x;
			symbol->glyph.uv[3] = (float)(offset_y + size_y) / (float)font_image->buffer.size_y;

			//
			if (scratch_buffer.capacity < size_x * size_y) {
				array_byte_resize(&scratch_buffer, size_x * size_y);
			}

			// @todo: receive an `invert_y` flag?
			asset_font_fill_buffer(
				font_image->asset_font,
				scratch_buffer.data, size_x,
				symbol->glyph.id, size_x, size_y, font_image->scale
			);

			for (uint32_t y = 0; y < size_y; y++) {
				memcpy(
					font_image->buffer.data + ((offset_y + y) * font_image->buffer.size_x + offset_x),
					scratch_buffer.data + ((size_y - y - 1) * size_x),
					size_x * sizeof(*scratch_buffer.data)
				);
			}

			offset_x += size_x + padding;
		}

		array_byte_free(&scratch_buffer);
	}

	// track glyphs
	hash_table_ensure_minimum_capacity(font_image->table, symbols_count);
	for (struct Font_Symbol const * symbol = symbols; symbol->glyph.id != 0; symbol++) {
		hash_table_set(font_image->table, symbol->codepoint, symbol); // treat symbol as glyph
	}

	MEMORY_FREE(symbols);
}

struct Font_Glyph const * font_image_get_glyph(struct Font_Image * const font_image, uint32_t codepoint) {
	return hash_table_get(font_image->table, codepoint);
}

float font_image_get_height(struct Font_Image * font_image) {
	int32_t value = asset_font_get_height(font_image->asset_font);
	return ((float)value) * font_image->scale;
}

float font_image_get_gap(struct Font_Image * font_image) {
	int32_t value = asset_font_get_gap(font_image->asset_font);
	return ((float)value) * font_image->scale;
}

float font_image_get_kerning(struct Font_Image * font_image, uint32_t id1, uint32_t id2) {
	int32_t value = asset_font_get_kerning(font_image->asset_font, id1, id2);
	return ((float)value) * font_image->scale;
}

//

static int font_image_sort_comparison(void const * v1, void const * v2) {
	struct Font_Symbol const * s1 = v1;
	struct Font_Symbol const * s2 = v2;
	
	uint32_t const size_y_1 = (uint32_t)(s1->glyph.params.rect[3] - s1->glyph.params.rect[1]);
	uint32_t const size_y_2 = (uint32_t)(s2->glyph.params.rect[3] - s2->glyph.params.rect[1]);

	if (size_y_1 < size_y_2) { return  1; }
	if (size_y_1 > size_y_2) { return -1; }

	uint32_t const size_x_1 = (uint32_t)(s1->glyph.params.rect[2] - s1->glyph.params.rect[0]);
	uint32_t const size_x_2 = (uint32_t)(s2->glyph.params.rect[2] - s2->glyph.params.rect[0]);

	if (size_x_1 < size_x_2) { return  1; }
	if (size_x_1 > size_x_2) { return -1; }

	return 0;
}
