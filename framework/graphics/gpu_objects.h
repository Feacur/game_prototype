#if !defined(GAME_GRAPHICS_GPU_OBJECTS)
#define GAME_GRAPHICS_GPU_OBJECTS

// interface from `gpu_*/graphics.c` to anywhere
// - gpu objects initialization and manipulation

#include "framework/graphics/types.h"

// @note: texture coordinates
// +------------+
// |texture  1,1|
// |            |
// |0,0         |
// +------------+

struct Array_Byte;
struct Mesh;
struct Image;

// ----- ----- ----- ----- -----
//     GPU program part
// ----- ----- ----- ----- -----
struct Gpu_Program_Field;
struct Ref gpu_program_init(struct Array_Byte const * asset);
void gpu_program_free(struct Ref gpu_program_ref);
void gpu_program_get_uniforms(struct Ref gpu_program_ref, uint32_t * count, struct Gpu_Program_Field const ** values);

// ----- ----- ----- ----- -----
//     GPU texture part
// ----- ----- ----- ----- -----
struct Ref gpu_texture_init(struct Image const * asset);
void gpu_texture_free(struct Ref gpu_texture_ref);

void gpu_texture_get_size(struct Ref gpu_texture_ref, uint32_t * x, uint32_t * y);

void gpu_texture_update(struct Ref gpu_texture_ref, struct Image const * asset);

// ----- ----- ----- ----- -----
//     GPU target part
// ----- ----- ----- ----- -----
struct Ref gpu_target_init(
	uint32_t size_x, uint32_t size_y,
	struct Texture_Parameters const * parameters,
	uint32_t count
);
void gpu_target_free(struct Ref gpu_target_ref);

void gpu_target_get_size(struct Ref gpu_target_ref, uint32_t * x, uint32_t * y);
struct Ref gpu_target_get_texture_ref(struct Ref gpu_target_ref, enum Texture_Type type, uint32_t index);

// ----- ----- ----- ----- -----
//     GPU mesh part
// ----- ----- ----- ----- -----
struct Ref gpu_mesh_init(struct Mesh const * asset);
void gpu_mesh_free(struct Ref gpu_mesh_ref);

void gpu_mesh_update(struct Ref gpu_mesh_ref, struct Mesh const * asset);

#endif
