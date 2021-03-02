#include "memory.h"
#include "asset_image.h"

#include "platform_file.h"

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

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#if defined(__clang__)
	#pragma clang diagnostic pop
#endif

uint8_t * asset_image_read(char const * path, uint32_t * out_size_x, uint32_t * out_size_y, uint32_t * out_channels) {
	size_t file_size;
	uint8_t * file_bytes = platform_file_read(path, &file_size);

	int size_x, size_y, channels;
	uint8_t * image_bytes = (uint8_t *)stbi_load_from_memory(file_bytes, (int)file_size, &size_x, &size_y, &channels, 0);

	*out_size_x = (uint32_t)size_x;
	*out_size_y = (uint32_t)size_y;
	*out_channels = (uint32_t)channels;
	return image_bytes;
}
