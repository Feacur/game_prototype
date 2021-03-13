#if !defined(GAME_GRAPHICS_IMPLEMENTATION)
#define GAME_GRAPHICS_IMPLEMENTATION

#include "framework/common.h"

struct Gpu_Program;
struct Gpu_Texture;
struct Gpu_Mesh;

struct Material;

uint32_t graphics_add_uniform(char const * name);
uint32_t graphics_find_uniform(char const * name);

void graphics_clear(void);
void graphics_viewport(uint32_t x, uint32_t y, uint32_t size_x, uint32_t size_y);
void graphics_draw(struct Material const * material, struct Gpu_Mesh const * gpu_mesh);

#endif
