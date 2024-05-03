#include "framework/formatter.h"

#include "framework/platform/file.h"
#include "framework/containers/buffer.h"
#include "framework/systems/memory.h"

#include "framework/__warnings_push.h"
	#define STBI_NO_STDIO
	#define STBI_ONLY_PNG

	#define STBI_MALLOC(size)           REALLOCATE(NULL,    size)
	#define STBI_REALLOC(pointer, size) REALLOCATE(pointer, size)
	#define STBI_FREE(pointer)          REALLOCATE(pointer, 0)

	#define STB_IMAGE_STATIC
	#define STB_IMAGE_IMPLEMENTATION
	#include <stb/stb_image.h>
#include "framework/__warnings_pop.h"


//
#include "image.h"

struct Image image_init(struct Buffer const * buffer) {
	// @note: ensure image layout
	stbi_set_flip_vertically_on_load(1);

	int size_x, size_y, channels;
	uint8_t * image_bytes = stbi_load_from_memory(buffer->data, (int)buffer->size, &size_x, &size_y, &channels, 0);

	return (struct Image){
		.capacity = (uint32_t)(size_x * size_y),
		.size = (struct uvec2){(uint32_t)size_x, (uint32_t)size_y},
		.data = image_bytes,
		.format = {
			.flags = TEXTURE_FLAG_COLOR,
			.type = gfx_type_get_vector_type(GFX_TYPE_R8_UNORM, (uint32_t)channels),
		},
	};
}

void image_free(struct Image * image) {
	FREE(image->data);
	cbuffer_clear(CBMP_(image));
}

void image_ensure(struct Image * image, struct uvec2 size) {
	uint32_t const channels = gfx_type_get_count(image->format.type);
	uint32_t const data_size = gfx_type_get_size(image->format.type);
	uint32_t const target_capacity = size.x * size.y * channels;
	if (image->capacity < target_capacity) {
		image->data = REALLOCATE(image->data, size.x * size.y * data_size);
		image->capacity = target_capacity;
	}
	image->size = size;
}
