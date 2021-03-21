#include "framework/containers/array_byte.h"
#include "framework/containers/hash_table.h"
#include "framework/assets/asset_image.h"
#include "framework/assets/asset_font.h"
#include "framework/memory.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Glyph {
	struct Glyph_Params params;
	uint32_t offset_x, offset_y;
};

struct Font_Image {
	struct Asset_Image buffer;
	struct Hash_Table * table;
	struct Asset_Font * asset_font; // weak reference
};

//
#include "font_image.h"

struct Font_Image * font_image_init(struct Asset_Font * asset_font, uint32_t size_x, uint32_t size_y) {
	struct Font_Image * font_image = MEMORY_ALLOCATE(struct Font_Image);

	font_image->buffer = (struct Asset_Image){
		.size_x = size_x,
		.size_y = size_y,
		.data = (size_x > 0 && size_y > 0) ? MEMORY_ALLOCATE_ARRAY(uint8_t, size_x * size_y) : NULL,
		.parameters = {
			.texture_type = TEXTURE_TYPE_COLOR,
			.data_type = DATA_TYPE_U8,
			.channels = 1,
		},
	};

	font_image->table = hash_table_init(sizeof(struct Glyph));

	font_image->asset_font = asset_font;
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
	memset(font_image->buffer.data, 0, font_image->buffer.size_x * font_image->buffer.size_x * sizeof(uint8_t));
}

struct Asset_Image * font_image_get_asset(struct Font_Image * font_image) {
	return &font_image->buffer;
}

void font_image_build(struct Font_Image * font_image, uint32_t const * codepoint_ranges) {
	font_image_clear(font_image);

	uint32_t const line_height = 32;
	float const font_scale = asset_font_get_scale(font_image->asset_font, (float)line_height);

	struct Array_Byte scratch_buffer;
	array_byte_init(&scratch_buffer);

	uint32_t offset_x = 0, offset_y = 0;
	for (uint32_t const *range = codepoint_ranges; *range != 0; range += 2) {
		for (uint32_t codepoint = *range, codepoint_to = range[1]; codepoint <= codepoint_to; codepoint++) {
			if (font_image->buffer.size_y < offset_y + line_height) { fprintf(stderr, "atlas's too small\n"); DEBUG_BREAK(); break; }

			uint32_t const glyph_id = asset_font_get_glyph_id(font_image->asset_font, codepoint);
			char asd = (char)codepoint; (void)asd;

			struct Glyph_Params glyph_params;
			asset_font_get_glyph_parameters(font_image->asset_font, &glyph_params, glyph_id, font_scale);

			hash_table_set(font_image->table, glyph_id, &(struct Glyph){
				.params = glyph_params,
				.offset_x = offset_x,
				.offset_y = offset_y,
			});

			if (glyph_params.is_empty) { continue; }

			if (font_image->buffer.size_x < glyph_params.bmp_size_x) { fprintf(stderr, "atlas's too small\n"); DEBUG_BREAK(); continue; }
			if (font_image->buffer.size_y < glyph_params.bmp_size_y) { fprintf(stderr, "atlas's too small\n"); DEBUG_BREAK(); continue; }

			if (scratch_buffer.capacity < glyph_params.bmp_size_x * glyph_params.bmp_size_y) {
				array_byte_resize(&scratch_buffer, glyph_params.bmp_size_x * glyph_params.bmp_size_y);
			}

			asset_font_fill_buffer(
				font_image->asset_font,
				scratch_buffer.data, glyph_params.bmp_size_x,
				glyph_id, glyph_params.bmp_size_x, glyph_params.bmp_size_y, font_scale
			);

			for (uint32_t y = 0; y < glyph_params.bmp_size_y; y++) {
				memcpy(
					font_image->buffer.data + ((offset_y + y) * font_image->buffer.size_x + offset_x),
					scratch_buffer.data + ((glyph_params.bmp_size_y - y - 1) * glyph_params.bmp_size_x),
					glyph_params.bmp_size_x * sizeof(*scratch_buffer.data)
				);
			}

			offset_x += glyph_params.bmp_size_x;
			if (offset_x > font_image->buffer.size_x - line_height) {
				offset_x = 0;
				offset_y += line_height;
			}
		}
	}

	array_byte_free(&scratch_buffer);
}

void font_image_get_data(struct Font_Image * font_image, uint32_t codepoint, struct Font_Glyph_Data * data) {
	uint32_t const glyph_id = asset_font_get_glyph_id(font_image->asset_font, codepoint);
	struct Glyph const * glyph = hash_table_get(font_image->table, glyph_id);
	if (glyph->params.is_empty) {
		*data = (struct Font_Glyph_Data){
			.size_x = (float)glyph->params.size_x,
		};
	}
	else {
		*data = (struct Font_Glyph_Data){
			.rect[0] = 0,
			.rect[1] = 0,
			.rect[2] = (float)glyph->params.bmp_size_x,
			.rect[3] = (float)glyph->params.bmp_size_y,
			.uv[0] = (float)glyph->offset_x / (float)font_image->buffer.size_x,
			.uv[1] = (float)glyph->offset_y / (float)font_image->buffer.size_y,
			.uv[2] = (float)(glyph->offset_x + glyph->params.bmp_size_x) / (float)font_image->buffer.size_x,
			.uv[3] = (float)(glyph->offset_y + glyph->params.bmp_size_y) / (float)font_image->buffer.size_y,
			.size_x = (float)glyph->params.size_x,
		};
	}
}
