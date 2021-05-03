#if !defined(GAME_ASSET_IMAGE)
#define GAME_ASSET_IMAGE

#include "framework/common.h"
#include "framework/graphics/types.h"

struct Image {
	uint32_t capacity, size_x, size_y;
	uint8_t * data;
	struct Texture_Parameters parameters;
	struct Texture_Settings settings;
};

void image_init(struct Image * image, char const * path);
void image_free(struct Image * image);

#endif
