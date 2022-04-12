#if !defined(GAME_ASSETS_IMAGE)
#define GAME_ASSETS_IMAGE

#include "framework/common.h"
#include "framework/graphics/types.h"

struct Buffer;

// @note: image data layout
// +----------+
// |image  1,1|
// |          |
// |0,0       |
// +----------+
struct Image {
	uint32_t capacity, size_x, size_y;
	void * data;
	struct Texture_Parameters parameters;
	struct Texture_Settings settings;
};

struct Image image_init(struct Texture_Settings settings, struct Buffer const * buffer);
void image_free(struct Image * image);

void image_ensure(struct Image * image, uint32_t size_x, uint32_t size_y);

#endif
