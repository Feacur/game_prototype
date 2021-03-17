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
struct Gpu_Texture;
struct Gpu_Texture * gpu_texture_init(struct Asset_Image * asset);
void gpu_texture_free(struct Gpu_Texture * gpu_texture);

void gpu_texture_get_size(struct Gpu_Texture * gpu_texture, uint32_t * x, uint32_t * y);

// -- GPU target part
struct Gpu_Target;
struct Gpu_Target * gpu_target_init(
	uint32_t size_x, uint32_t size_y,
	struct Texture_Parameters const * parameters,
	bool const * readable,
	uint32_t count
);
void gpu_target_free(struct Gpu_Target * gpu_target);

void gpu_target_get_size(struct Gpu_Target * gpu_target, uint32_t * x, uint32_t * y);
struct Gpu_Texture * gpu_target_get_texture(struct Gpu_Target * gpu_target, enum Texture_Type type, uint32_t index);

// -- GPU mesh part
struct Gpu_Mesh;
struct Gpu_Mesh * gpu_mesh_init(struct Asset_Mesh * asset);
void gpu_mesh_free(struct Gpu_Mesh * gpu_mesh);

#endif
