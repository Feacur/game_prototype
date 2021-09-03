#if !defined(GAME_ASSETS_IMAGE)
#define GAME_ASSETS_IMAGE

#include "framework/common.h"
#include "framework/graphics/types.h"

// @note: image data layout
// +----------+
// |image  1,1|
// |          |
// |0,0       |
// +----------+
struct Image {
	uint32_t capacity, size_x, size_y;
	uint8_t * data;
	struct Texture_Parameters parameters;
	struct Texture_Settings settings;
};

void image_init(struct Image * image, char const * path);
void image_free(struct Image * image);

void image_resize(struct Image * image, uint32_t size_x, uint32_t size_y);

#endif
