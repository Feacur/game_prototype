#if !defined(GAME_FRAMEWORK_GPU_OBJECTS)
#define GAME_FRAMEWORK_GPU_OBJECTS

#include "framework/common.h"

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

// -- GPU mesh part
struct Gpu_Mesh;
struct Gpu_Mesh * gpu_mesh_init(struct Asset_Mesh * asset);
void gpu_mesh_free(struct Gpu_Mesh * gpu_mesh);

#endif