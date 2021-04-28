#if !defined(GAME_GRAPHICS_MATERIAL)
#define GAME_GRAPHICS_MATERIAL

#include "framework/containers/array_u64.h"
#include "framework/containers/array_u32.h"
#include "framework/containers/array_s32.h"
#include "framework/containers/array_float.h"

struct Gpu_Program;
struct Gpu_Texture;

struct Gfx_Material {
	struct Gpu_Program * program; // weak reference

	struct Array_U64 textures;
	struct Array_U32 values_u32;
	struct Array_S32 values_s32;
	struct Array_Float values_float;
};

void gfx_material_init(struct Gfx_Material * material, struct Gpu_Program * gpu_program);
void gfx_material_free(struct Gfx_Material * material);

void gfx_material_set_texture(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, struct Gpu_Texture ** value);
void gfx_material_set_u32(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, uint32_t const * value);
void gfx_material_set_s32(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, int32_t const * value);
void gfx_material_set_float(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, float const * value);

#endif
