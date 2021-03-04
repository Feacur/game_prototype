#if !defined(GAME_ASSET_IMAGE)
#define GAME_ASSET_IMAGE

#include "common.h"

struct Asset_Image {
	uint32_t size_x, size_y, channels;
	uint8_t * data;
};

void asset_image_init(struct Asset_Image * asset_image, char const * path);
void asset_image_free(struct Asset_Image * asset_image);

#endif
