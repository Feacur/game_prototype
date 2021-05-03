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

void asset_image_init(struct Image * asset_image, char const * path);
void asset_image_free(struct Image * asset_image);

#endif
