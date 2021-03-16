#if !defined(GAME_FRAMEWORK_GRAPHICS)
#define GAME_FRAMEWORK_GRAPHICS

#include "framework/common.h"

#include "types.h"

struct Render_Pass;

uint32_t graphics_add_uniform(char const * name);
uint32_t graphics_find_uniform(char const * name);

void graphics_viewport(uint32_t x, uint32_t y, uint32_t size_x, uint32_t size_y);
void graphics_clear(uint32_t framebuffer, enum Texture_Type mask, uint32_t rgba);
void graphics_draw(struct Render_Pass const * pass);

#endif
