#if !defined(GAME_GRAPHICS_IMPLEMENTATION)
#define GAME_GRAPHICS_IMPLEMENTATION

#include "code/common.h"

struct Array_Byte;
struct Asset_Mesh;
struct Asset_Image;

// -- GPU types part
struct Gpu_Program;
struct Gpu_Texture;
struct Gpu_Mesh;

// -- graphics library part
uint32_t glibrary_find_uniform(char const * name);
void glibrary_clear(void);
void glibrary_viewport(uint32_t x, uint32_t y, uint32_t size_x, uint32_t size_y);
void glibrary_draw(struct Gpu_Program * gpu_program, struct Gpu_Mesh * gpu_mesh);

// -- GPU program part
struct Gpu_Program * gpu_program_init(struct Array_Byte * asset);
void gpu_program_free(struct Gpu_Program * gpu_program);

void gpu_program_select(struct Gpu_Program * gpu_program);
void gpu_program_set_texture(struct Gpu_Program * gpu_program, uint32_t uniform_id, struct Gpu_Texture * gpu_texture);

// -- GPU texture part
struct Gpu_Texture * gpu_texture_init(struct Asset_Image * asset);
void gpu_texture_free(struct Gpu_Texture * gpu_texture);

// -- GPU mesh part
struct Gpu_Mesh * gpu_mesh_init(struct Asset_Mesh * asset);
void gpu_mesh_free(struct Gpu_Mesh * gpu_mesh);

void gpu_mesh_select(struct Gpu_Mesh * gpu_mesh);

// -- GPU unit part
void gpu_unit_init(struct Gpu_Texture * gpu_texture);
void gpu_unit_free(struct Gpu_Texture * gpu_texture);

#endif
