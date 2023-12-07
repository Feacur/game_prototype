#if !defined(FRAMEWORK_GRAPHICS_GPU_OBJECTS)
#define FRAMEWORK_GRAPHICS_GPU_OBJECTS

// interface from `graphics.c` to anywhere
// - gpu objects initialization and manipulation

#include "framework/maths_types.h"
#include "framework/graphics/gfx_types.h"

// @note: texture coordinates
// +------------+
// |texture  1,1|
// |            |
// |0,0         |
// +------------+

struct Buffer;
struct Mesh;
struct Image;
struct Hashmap;

// ----- ----- ----- ----- -----
//     GPU program part
// ----- ----- ----- ----- -----

struct Handle gpu_program_init(struct Buffer const * asset);
void gpu_program_free(struct Handle handle);

void gpu_program_update(struct Handle handle, struct Buffer const * asset);

// uniform string id : `struct Gpu_Uniform`
struct Hashmap const * gpu_program_get_uniforms(struct Handle handle);

// ----- ----- ----- ----- -----
//     GPU texture part
// ----- ----- ----- ----- -----

struct Handle gpu_texture_init(struct Image const * asset);
void gpu_texture_free(struct Handle handle);

void gpu_texture_update(struct Handle handle, struct Image const * asset);

struct uvec2 gpu_texture_get_size(struct Handle handle);

// ----- ----- ----- ----- -----
//     GPU target part
// ----- ----- ----- ----- -----

struct GPU_Target_Asset {
	struct uvec2 size;
	uint32_t count;
	struct Texture_Parameters const * parameters;
};

struct Handle gpu_target_init(struct GPU_Target_Asset asset);
void gpu_target_free(struct Handle handle);

void gpu_target_update(struct Handle handle, struct GPU_Target_Asset asset);

struct uvec2 gpu_target_get_size(struct Handle handle);
struct Handle gpu_target_get_texture_handle(struct Handle handle, enum Texture_Type type, uint32_t index);

// ----- ----- ----- ----- -----
//     GPU storage part
// ----- ----- ----- ----- -----

struct Handle gpu_buffer_init(struct Buffer const * asset);
void gpu_buffer_free(struct Handle handle);

void gpu_buffer_update(struct Handle handle, struct Buffer const * asset);

// ----- ----- ----- ----- -----
//     GPU mesh part
// ----- ----- ----- ----- -----

struct Handle gpu_mesh_init(struct Mesh const * asset);
void gpu_mesh_free(struct Handle handle);

void gpu_mesh_update(struct Handle handle, struct Mesh const * asset);

#endif
