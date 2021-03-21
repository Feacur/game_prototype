#include "framework/containers/array_byte.h"
#include "framework/containers/hash_table.h"
#include "framework/assets/asset_image.h"
#include "framework/assets/asset_font.h"
#include "framework/memory.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Glyph {
	uint32_t id;
	struct Glyph_Params params;
	float uv[4];
};

struct Font_Image {
	struct Asset_Image buffer;
	struct Asset_Font * asset_font; // weak reference
	struct Hash_Table * table;
	uint32_t size;
	float scale;
};

//
#include "font_image.h"

struct Font_Image * font_image_init(struct Asset_Font * asset_font, uint32_t size, uint32_t size_x, uint32_t size_y) {
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
	font_image->size = size;
	font_image->scale = asset_font_get_scale(asset_font, (float)size);

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
	uint32_t codepoints_count = 0;
	for (uint32_t const *range = codepoint_ranges; *range != 0; range += 2) {
		codepoints_count += 1 + (range[1] - range[0]);
	}

	uint32_t glyphs_count = 0;
	struct Glyph * glyphs = MEMORY_ALLOCATE_ARRAY(struct Glyph, codepoints_count + 1);
	glyphs[codepoints_count].id = 0;

	// collect glyphs
	for (uint32_t const *range = codepoint_ranges; *range != 0; range += 2) {
		for (uint32_t codepoint = *range, codepoint_to = range[1]; codepoint <= codepoint_to; codepoint++) {
			uint32_t const glyph_id = asset_font_get_glyph_id(font_image->asset_font, codepoint);

			struct Glyph_Params glyph_params;
			asset_font_get_glyph_parameters(font_image->asset_font, &glyph_params, glyph_id, font_image->scale);

			glyphs[glyphs_count++] = (struct Glyph){
				.id = glyph_id,
				.params = glyph_params,
			};
		}
	}

	// pack glyphs into the atlas
	font_image_clear(font_image);

	{
		struct Array_Byte scratch_buffer;
		array_byte_init(&scratch_buffer);

		uint32_t offset_x = 0, offset_y = 0;
		for (struct Glyph * glyph = glyphs; glyph->id != 0; glyph++) {
			if (font_image->buffer.size_y < offset_y + font_image->size) {
				fprintf(stderr, "atlas's too small\n"); DEBUG_BREAK(); break;
			}

			//
			struct Glyph_Params const * params = &glyph->params;
			if (params->is_empty) { continue; }

			if (font_image->buffer.size_x < params->bmp_size_x) { fprintf(stderr, "atlas's too small\n"); DEBUG_BREAK(); break; }
			if (font_image->buffer.size_y < params->bmp_size_y) { fprintf(stderr, "atlas's too small\n"); DEBUG_BREAK(); break; }

			glyph->uv[0] = (float)(offset_x)                      / (float)font_image->buffer.size_x;
			glyph->uv[1] = (float)(offset_y)                      / (float)font_image->buffer.size_y;
			glyph->uv[2] = (float)(offset_x + params->bmp_size_x) / (float)font_image->buffer.size_x;
			glyph->uv[3] = (float)(offset_y + params->bmp_size_y) / (float)font_image->buffer.size_y;

			//
			if (scratch_buffer.capacity < params->bmp_size_x * params->bmp_size_y) {
				array_byte_resize(&scratch_buffer, params->bmp_size_x * params->bmp_size_y);
			}

			asset_font_fill_buffer(
				font_image->asset_font,
				scratch_buffer.data, params->bmp_size_x,
				glyph->id, params->bmp_size_x, params->bmp_size_y, font_image->scale
			);

			for (uint32_t y = 0; y < params->bmp_size_y; y++) {
				memcpy(
					font_image->buffer.data + ((offset_y + y) * font_image->buffer.size_x + offset_x),
					scratch_buffer.data + ((params->bmp_size_y - y - 1) * params->bmp_size_x),
					params->bmp_size_x * sizeof(*scratch_buffer.data)
				);
			}

			//
			offset_x += params->bmp_size_x;
			if (offset_x > font_image->buffer.size_x - font_image->size) {
				offset_x = 0;
				offset_y += font_image->size;
			}
		}

		array_byte_free(&scratch_buffer);
	}

	// track glyphs
	{
		struct Glyph const * glyph = glyphs;
		for (uint32_t const * range = codepoint_ranges; *range != 0; range += 2) {
			for (uint32_t codepoint = *range, codepoint_to = range[1]; codepoint <= codepoint_to; codepoint++) {
				hash_table_set(font_image->table, codepoint, glyph++);
			}
		}
	}

	MEMORY_FREE(glyphs);
}

void font_image_get_data(struct Font_Image * font_image, uint32_t codepoint, struct Font_Glyph * data) {
	struct Glyph const * glyph = hash_table_get(font_image->table, codepoint);
	if (glyph == NULL) { data->id = 0; return; }
	*data = (struct Font_Glyph){
		.id = glyph->id,
		.params = &glyph->params,
		.uv = glyph->uv,
	};
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
