#include "framework/assets/asset_image.h"
#include "framework/assets/asset_font.h"
#include "framework/memory.h"

#include <stdlib.h>
#include <string.h>

struct Font_Image {
	struct Asset_Image buffer;
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

	font_image->asset_font = asset_font;
	return font_image;
}

void font_image_free(struct Font_Image * font_image) {
	MEMORY_FREE(font_image->buffer.data);

	memset(font_image, 0, sizeof(*font_image));
	MEMORY_FREE(font_image);
}

void font_image_clear(struct Font_Image * font_image) {
	memset(font_image->buffer.data, 0, font_image->buffer.size_x * font_image->buffer.size_x * sizeof(uint8_t));
}

struct Asset_Image * font_image_get_asset(struct Font_Image * font_image) {
	return &font_image->buffer;
}

void font_image_build(struct Font_Image * font_image) {
	float const font_scale = asset_font_get_scale(font_image->asset_font, 32);

	struct Glyph_Params glyph_params;
	uint32_t const glyph_id = asset_font_get_glyph_id(font_image->asset_font, (uint32_t)'h');
	asset_font_get_glyph_parameters(font_image->asset_font, &glyph_params, glyph_id);
	if (!glyph_params.is_empty) {
		uint32_t const font_size_x = (uint32_t)(((float)glyph_params.bmp_size_x) * font_scale);
		uint32_t const font_size_y = (uint32_t)(((float)glyph_params.bmp_size_y) * font_scale);
		if (font_image->buffer.size_x > font_size_x && font_image->buffer.size_y > font_size_y) {
			asset_font_fill_buffer(
				font_image->asset_font,
				font_image->buffer.data, font_image->buffer.size_x,
				glyph_id,
				font_size_x, font_size_y, font_scale
			);
		}
	}
}
