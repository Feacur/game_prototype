#if !defined(GAME_FRAMEWORK_GRAPHICS_MATERIAL)
#define GAME_FRAMEWORK_GRAPHICS_MATERIAL

#include "framework/containers/array_pointer.h"
#include "framework/containers/array_u32.h"
#include "framework/containers/array_s32.h"
#include "framework/containers/array_float.h"

struct Gpu_Program;
struct Gpu_Texture;

struct Material {
	struct Gpu_Program * program;

	struct Array_Pointer textures;
	struct Array_U32 values_u32;
	struct Array_S32 values_s32;
	struct Array_Float values_float;
};

void material_init(struct Material * material, struct Gpu_Program * gpu_program);
void material_free(struct Material * material);

void material_set_texture(struct Material * material, uint32_t uniform_id, uint32_t count, struct Gpu_Texture ** value);
void material_set_u32(struct Material * material, uint32_t uniform_id, uint32_t count, uint32_t const * value);
void material_set_s32(struct Material * material, uint32_t uniform_id, uint32_t count, int32_t const * value);
void material_set_float(struct Material * material, uint32_t uniform_id, uint32_t count, float const * value);

#endif
