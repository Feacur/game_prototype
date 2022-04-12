#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/containers/buffer.h"

#include "framework/platform_file.h"

// @idea: compile third-parties as separate units
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Weverything"
#endif

#define STBI_ONLY_PNG

#define STBI_MALLOC(size)           MEMORY_ALLOCATE_SIZE(size)
#define STBI_REALLOC(pointer, size) memory_reallocate(pointer, size)
#define STBI_FREE(pointer)          MEMORY_FREE(pointer)

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#if defined(__clang__)
	#pragma clang diagnostic pop
#endif

//
#include "image.h"

struct Image image_init(struct Texture_Settings settings, struct Buffer const * buffer) {
	// @note: ensure image data layout
	stbi_set_flip_vertically_on_load(1);

	int size_x, size_y, channels;
	uint8_t * image_bytes = (uint8_t *)stbi_load_from_memory(buffer->data, (int)buffer->count, &size_x, &size_y, &channels, 0);

	return (struct Image){
		.capacity = (uint32_t)(size_x * size_y),
		.size_x = (uint32_t)size_x,
		.size_y = (uint32_t)size_y,
		.data = image_bytes,
		.parameters = {
			.texture_type = TEXTURE_TYPE_COLOR,
			.data_type = data_type_get_vector_type(DATA_TYPE_R8_UNORM, (uint32_t)channels),
		},
		.settings = settings,
	};
}

void image_free(struct Image * image) {
	MEMORY_FREE(image->data);
	common_memset(image, 0, sizeof(*image));
}

void image_ensure(struct Image * image, uint32_t size_x, uint32_t size_y) {
	uint32_t const channels = data_type_get_count(image->parameters.data_type);
	uint32_t const data_size = data_type_get_size(image->parameters.data_type);
	uint32_t const target_capacity = size_x * size_y * channels;
	if (image->capacity < target_capacity) {
		image->data = MEMORY_REALLOCATE_SIZE(image->data, size_x * size_y * data_size);
		image->capacity = target_capacity;
	}
	image->size_x = size_x;
	image->size_y = size_y;
}
