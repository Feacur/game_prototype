#if !defined(GAME_GRAPHICS_IMPLEMENTATION)
#define GAME_GRAPHICS_IMPLEMENTATION

#include "code/common.h"

// -- GPU types part
struct Gpu_Program;
struct Gpu_Texture;
struct Gpu_Mesh;

// -- graphics library part
uint32_t glibrary_get_uniform_id(char const * name);
void glibrary_clear(void);
void glibrary_draw(struct Gpu_Program * gpu_program, struct Gpu_Mesh * gpu_mesh);

// -- GPU program part
struct Gpu_Program * gpu_program_init(char const * text, uint32_t text_size);
void gpu_program_free(struct Gpu_Program * gpu_program);

void gpu_program_select(struct Gpu_Program * gpu_program);
void gpu_program_set_texture(struct Gpu_Program * gpu_program, uint32_t uniform_id, struct Gpu_Texture * gpu_texture);

// -- GPU texture part
struct Gpu_Texture * gpu_texture_init(uint8_t const * data, uint32_t size_x, uint32_t size_y, uint32_t channels);
void gpu_texture_free(struct Gpu_Texture * gpu_texture);

// -- GPU mesh part
struct Gpu_Mesh * gpu_mesh_init(
	float const * vertices, uint32_t vertices_count,
	uint32_t const * attributes, uint32_t attributes_count,
	uint32_t const * indices, uint32_t indices_count
);
void gpu_mesh_free(struct Gpu_Mesh * gpu_mesh);

void gpu_mesh_select(struct Gpu_Mesh * gpu_mesh);

// -- GPU unit part
void gpu_unit_init(struct Gpu_Texture * gpu_texture);
void gpu_unit_free(struct Gpu_Texture * gpu_texture);

#endif
