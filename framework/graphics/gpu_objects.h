#if !defined(GAME_FRAMEWORK_GPU_OBJECTS)
#define GAME_FRAMEWORK_GPU_OBJECTS

#include "framework/common.h"
#include "framework/graphics/types.h"

struct Array_Byte;
struct Asset_Mesh;
struct Asset_Image;

// -- GPU program part
struct Gpu_Program;
struct Gpu_Program_Field;
struct Gpu_Program * gpu_program_init(struct Array_Byte * asset);
void gpu_program_free(struct Gpu_Program * gpu_program);
void gpu_program_get_uniforms(struct Gpu_Program * gpu_program, uint32_t * count, struct Gpu_Program_Field const ** values);

// -- GPU texture part
struct Ref gpu_texture_init(struct Asset_Image * asset);
void gpu_texture_free(struct Ref gpu_texture_ref);

void gpu_texture_get_size(struct Ref gpu_texture_ref, uint32_t * x, uint32_t * y);

void gpu_texture_update(struct Ref gpu_texture_ref, struct Asset_Image * asset);

// -- GPU target part
struct Ref gpu_target_init(
	uint32_t size_x, uint32_t size_y,
	struct Texture_Parameters const * parameters,
	uint32_t count
);
void gpu_target_free(struct Ref gpu_target_ref);

void gpu_target_get_size(struct Ref gpu_target_ref, uint32_t * x, uint32_t * y);
struct Ref gpu_target_get_texture_ref(struct Ref gpu_target_ref, enum Texture_Type type, uint32_t index);

// -- GPU mesh part
struct Ref gpu_mesh_init(struct Asset_Mesh * asset);
void gpu_mesh_free(struct Ref gpu_mesh_ref);

void gpu_mesh_update(struct Ref gpu_mesh_ref, struct Asset_Mesh * asset);

#endif
