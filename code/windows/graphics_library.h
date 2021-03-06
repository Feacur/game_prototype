#if !defined(GAME_PLATFORM_GRAPHICS_LIBRARY)
#define GAME_PLATFORM_GRAPHICS_LIBRARY

#include "code/common.h"

struct Window;
struct Graphics;

struct Graphics * graphics_init(struct Window * window);
void graphics_free(struct Graphics * context);

int32_t graphics_get_vsync(struct Graphics * context);
void graphics_set_vsync(struct Graphics * context, int32_t value);
void graphics_display(struct Graphics * context);

// void graphics_size(struct Graphics * context, uint32_t size_x, uint32_t size_y);

#endif
