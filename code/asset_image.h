#if !defined(GAME_ASSET_IMAGE)
#define GAME_ASSET_IMAGE

#include "common.h"

uint8_t * asset_image_read(char const * path, uint32_t * out_size_x, uint32_t * out_size_y, uint32_t * out_channels);

#endif
