#include "framework/memory.h"
#include "framework/containers/array_byte.h"

#include "framework/platform_file.h"

#include <string.h>

// better to compile third-parties as separate units
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Weverything"
#endif

#define STBI_NO_STDIO
#define STBI_NO_JPEG
// #define STBI_NO_PNG
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM

#define STBI_MALLOC(size)           MEMORY_ALLOCATE_SIZE(size)
#define STBI_REALLOC(pointer, size) MEMORY_REALLOCATE_SIZE(pointer, size)
#define STBI_FREE(pointer)          MEMORY_FREE(pointer)

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#if defined(__clang__)
	#pragma clang diagnostic pop
#endif

//
#include "asset_image.h"

void asset_image_init(struct Asset_Image * asset_image, char const * path) {
	struct Array_Byte file;
	platform_file_init(&file, path);

	// @todo: check if it's opengl-specific?
	stbi_set_flip_vertically_on_load(1);

	int size_x, size_y, channels;
	uint8_t * image_bytes = (uint8_t *)stbi_load_from_memory(file.data, (int)file.count, &size_x, &size_y, &channels, 0);

	array_byte_free(&file);

	*asset_image = (struct Asset_Image){
		.size_x = (uint32_t)size_x,
		.size_y = (uint32_t)size_y,
		.channels = (uint32_t)channels,
		.data = image_bytes,
	};
}

void asset_image_free(struct Asset_Image * asset_image) {
	MEMORY_FREE(asset_image->data);
	memset(asset_image, 0, sizeof(*asset_image));
}
