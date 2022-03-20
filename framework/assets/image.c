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

void image_init(struct Image * image, struct Texture_Settings settings, struct CString path) {
	struct Buffer file;
	bool const read_success = platform_file_read_entire(path, &file);
	if (!read_success || file.count == 0) { DEBUG_BREAK(); return; }

	// @note: ensure image data layout
	stbi_set_flip_vertically_on_load(1);

	int size_x, size_y, channels;
	uint8_t * image_bytes = (uint8_t *)stbi_load_from_memory(file.data, (int)file.count, &size_x, &size_y, &channels, 0);

	buffer_free(&file);

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
		.settings = settings,
	};
}

void image_free(struct Image * image) {
	MEMORY_FREE(image, image->data);
	common_memset(image, 0, sizeof(*image));
}

void image_resize(struct Image * image, uint32_t size_x, uint32_t size_y) {
	// @note: obviously, it's lossy
	image->data = MEMORY_REALLOCATE_ARRAY(image, image->data, size_x * size_y * image->parameters.channels);
	image->capacity = size_x * size_y;
}
