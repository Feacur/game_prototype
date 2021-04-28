#if !defined(GAME_GRAPHICS_IMPLEMENTATION)
#define GAME_GRAPHICS_IMPLEMENTATION

#include "pass.h"

struct Render_Pass;

uint32_t graphics_add_uniform(char const * name);
uint32_t graphics_find_uniform(char const * name);

void graphics_draw(struct Render_Pass const * pass);

#endif
