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

// ----- ----- ----- ----- -----
//     GPU program part
// ----- ----- ----- ----- -----

struct Handle gpu_program_init(struct Buffer const * asset);
void gpu_program_free(struct Handle handle);

void gpu_program_update(struct Handle handle, struct Buffer const * asset);

struct GPU_Program const * gpu_program_get(struct Handle handle);

// ----- ----- ----- ----- -----
//     GPU texture part
// ----- ----- ----- ----- -----

struct Handle gpu_texture_init(struct Image const * asset);
void gpu_texture_free(struct Handle handle);

void gpu_texture_update(struct Handle handle, struct Image const * asset);

struct GPU_Texture const * gpu_texture_get(struct Handle handle);

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

struct GPU_Target const * gpu_target_get(struct Handle handle);
struct Handle gpu_target_get_texture(struct Handle handle, enum Texture_Type type, uint32_t index);

// ----- ----- ----- ----- -----
//     GPU storage part
// ----- ----- ----- ----- -----

struct Handle gpu_buffer_init(struct Buffer const * asset);
void gpu_buffer_free(struct Handle handle);

void gpu_buffer_update(struct Handle handle, struct Buffer const * asset);

struct GPU_Buffer const * gpu_buffer_get(struct Handle handle);

// ----- ----- ----- ----- -----
//     GPU mesh part
// ----- ----- ----- ----- -----

struct Handle gpu_mesh_init(struct Mesh const * asset);
void gpu_mesh_free(struct Handle handle);

void gpu_mesh_update(struct Handle handle, struct Mesh const * asset);

struct GPU_Mesh const * gpu_mesh_get(struct Handle handle);

#endif
