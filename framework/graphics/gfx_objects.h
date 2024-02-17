#if !defined(FRAMEWORK_GRAPHICS_GPU_OBJECTS)
#define FRAMEWORK_GRAPHICS_GPU_OBJECTS

// interface from `graphics.c` to anywhere
// - gpu objects initialization and manipulation

// @note: texture coordinates
// +------------+
// |texture  1,1|
// |            |
// |0,0         |
// +------------+

#include "framework/containers/array.h"
#include "framework/containers/hashmap.h"
#include "framework/graphics/gfx_types.h"

struct Buffer;
struct Mesh;
struct Image;

struct GPU_Target_Asset {
	struct uvec2 size;
	struct Array formats; // `struct Target_Format`
};

// ----- ----- ----- ----- -----
//     GPU program part
// ----- ----- ----- ----- -----

struct GPU_Uniform {
	enum Gfx_Type type;
	uint32_t array_size;
};

struct GPU_Program {
	struct Hashmap uniforms; // name `struct Handle` : `struct GPU_Uniform` (at least)
	// @idea: add an optional asset source
};

struct Handle gpu_program_init(struct Buffer const * asset);
HANDLE_ACTION(gpu_program_free);

void gpu_program_update(struct Handle handle, struct Buffer const * asset);

struct GPU_Program const * gpu_program_get(struct Handle handle);

// ----- ----- ----- ----- -----
//     GPU sampler part
// ----- ----- ----- ----- -----

struct Handle gpu_sampler_init(struct Gfx_Sampler const * asset);
HANDLE_ACTION(gpu_sampler_free);

void gpu_sampler_update(struct Handle handle, struct Gfx_Sampler const * asset);

struct Gfx_Sampler const * gpu_sampler_get(struct Handle handle);

// ----- ----- ----- ----- -----
//     GPU texture part
// ----- ----- ----- ----- -----

struct GPU_Texture {
	struct uvec2 size;
	struct Texture_Format format;
	struct Texture_Settings settings;
	// @idea: add an optional asset source
};

struct Handle gpu_texture_init(struct Image const * asset);
HANDLE_ACTION(gpu_texture_free);

void gpu_texture_update(struct Handle handle, struct Image const * asset);

struct GPU_Texture const * gpu_texture_get(struct Handle handle);

uint32_t gpu_texture_get_levels(struct GPU_Texture const * gpu_texture);

// ----- ----- ----- ----- -----
//     GPU unit part
// ----- ----- ----- ----- -----

// ----- ----- ----- ----- -----
//     GPU target part
// ----- ----- ----- ----- -----

struct GPU_Target_Buffer {
	struct Texture_Format format;
};

struct GPU_Target {
	struct uvec2 size;
	struct Array textures; // texture `struct Handle`
	struct Array buffers;  // `struct GPU_Target_Buffer` (at least)
	// @idea: add an optional asset source
};

struct Handle gpu_target_init(struct GPU_Target_Asset const * asset);
HANDLE_ACTION(gpu_target_free);

void gpu_target_update(struct Handle handle, struct GPU_Target_Asset const * asset);

struct GPU_Target const * gpu_target_get(struct Handle handle);
struct Handle gpu_target_get_texture(struct Handle handle, enum Texture_Flag flags, uint32_t index);

// ----- ----- ----- ----- -----
//     GPU buffer part
// ----- ----- ----- ----- -----

struct GPU_Buffer {
	size_t capacity, size;
};

struct Handle gpu_buffer_init(struct Buffer const * asset);
HANDLE_ACTION(gpu_buffer_free);

void gpu_buffer_update(struct Handle handle, struct Buffer const * asset);

struct GPU_Buffer const * gpu_buffer_get(struct Handle handle);

// ----- ----- ----- ----- -----
//     GPU mesh part
// ----- ----- ----- ----- -----

struct GPU_Mesh_Buffer {
	struct Handle gh_buffer;
	struct Mesh_Format format;
	struct Mesh_Attributes attributes;
	bool is_index;
};

struct GPU_Mesh {
	struct Array buffers; // `struct GPU_Mesh_Buffer`
	// @idea: add an optional asset source
};

struct Handle gpu_mesh_init(struct Mesh const * asset);
HANDLE_ACTION(gpu_mesh_free);

void gpu_mesh_update(struct Handle handle, struct Mesh const * asset);

struct GPU_Mesh const * gpu_mesh_get(struct Handle handle);

#endif
