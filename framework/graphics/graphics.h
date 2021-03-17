#if !defined(GAME_FRAMEWORK_GRAPHICS)
#define GAME_FRAMEWORK_GRAPHICS

#include "framework/common.h"

#include "types.h"

struct Gpu_Target;
struct Render_Pass;

uint32_t graphics_add_uniform(char const * name);
uint32_t graphics_find_uniform(char const * name);

void graphics_draw(struct Render_Pass const * pass);

#endif
