#if !defined(GAME_GRAPHICS_IMPLEMENTATION)
#define GAME_GRAPHICS_IMPLEMENTATION

#include "framework/common.h"

struct Gpu_Program;
struct Gpu_Texture;
struct Gpu_Mesh;

uint32_t graphics_add_uniform(char const * name);
uint32_t graphics_find_uniform(char const * name);

void graphics_clear(void);
void graphics_viewport(uint32_t x, uint32_t y, uint32_t size_x, uint32_t size_y);
void graphics_draw(struct Gpu_Program * gpu_program, struct Gpu_Mesh * gpu_mesh);

void graphics_select_program(struct Gpu_Program * gpu_program);
void graphics_select_mesh(struct Gpu_Mesh * gpu_mesh);

void graphics_set_uniform(struct Gpu_Program * gpu_program, uint32_t uniform_id, void const * data);

#endif
