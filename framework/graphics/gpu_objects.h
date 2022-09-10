#if !defined(FRAMEWORK_GRAPHICS_GPU_OBJECTS)
#define FRAMEWORK_GRAPHICS_GPU_OBJECTS

// interface from `gpu_*/graphics.c` to anywhere
// - gpu objects initialization and manipulation

#include "framework/graphics/types.h"

// @note: texture coordinates
// +------------+
// |texture  1,1|
// |            |
// |0,0         |
// +------------+

struct Buffer;
struct Mesh;
struct Image;
struct Hash_Table_U32;

// ----- ----- ----- ----- -----
//     GPU program part
// ----- ----- ----- ----- -----

struct Ref gpu_program_init(struct Buffer const * asset);
void gpu_program_free(struct Ref gpu_program_ref);

// uniform string id : `struct Gpu_Uniform`
struct Hash_Table_U32 const * gpu_program_get_uniforms(struct Ref gpu_program_ref);

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
	uint32_t parameters_count,
	struct Texture_Parameters const * parameters
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
