#include "framework/memory.h"
#include "framework/containers/array_byte.h"

#include "framework/platform_file.h"

#include <string.h>

// better to compile third-parties as separate units
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Weverything"
#endif

#define STBI_ONLY_PNG

#define STBI_MALLOC(size)           MEMORY_ALLOCATE_SIZE(NULL, size)
#define STBI_REALLOC(pointer, size) MEMORY_REALLOCATE_SIZE(NULL, pointer, size)
#define STBI_FREE(pointer)          MEMORY_FREE(NULL, pointer)

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#if defined(__clang__)
	#pragma clang diagnostic pop
#endif

//
#include "image.h"

void image_init(struct Image * image, char const * path) {
	struct Array_Byte file;
	platform_file_read_entire(path, &file);

	// @todo: check if it's opengl-specific?
	stbi_set_flip_vertically_on_load(1);

	int size_x, size_y, channels;
	uint8_t * image_bytes = (uint8_t *)stbi_load_from_memory(file.data, (int)file.count, &size_x, &size_y, &channels, 0);

	array_byte_free(&file);

	*image = (struct Image){
		.capacity = (uint32_t)(size_x * size_y),
		.size_x = (uint32_t)size_x,
		.size_y = (uint32_t)size_y,
		.data = image_bytes,
		.parameters = {
			.texture_type = TEXTURE_TYPE_COLOR,
			.data_type = DATA_TYPE_U8,
			.channels = (uint32_t)channels,
		},
		.settings = {
			.wrap_x = WRAP_MODE_CLAMP,
			.wrap_y = WRAP_MODE_CLAMP,
		},
	};
}

void image_free(struct Image * image) {
	if (image->capacity > 0) {
		MEMORY_FREE(image, image->data);
	}
	memset(image, 0, sizeof(*image));
}
