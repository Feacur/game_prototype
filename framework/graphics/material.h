#if !defined(GAME_GRAPHICS_MATERIAL)
#define GAME_GRAPHICS_MATERIAL

#include "framework/containers/array_any.h"
#include "framework/containers/array_u32.h"
#include "framework/containers/array_s32.h"
#include "framework/containers/array_float.h"
#include "framework/containers/ref.h"

#include "types.h"

struct Gfx_Material {
	struct Ref gpu_program_ref;

	struct Array_Any textures;
	struct Array_U32 values_u32;
	struct Array_S32 values_s32;
	struct Array_Float values_float;

	struct Blend_Mode blend_mode;
	struct Depth_Mode depth_mode;
};

void gfx_material_init(
	struct Gfx_Material * material,
	struct Ref gpu_program_ref,
	struct Blend_Mode const * blend_mode,
	struct Depth_Mode const * depth_mode
);
void gfx_material_free(struct Gfx_Material * material);

void gfx_material_set_texture(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, struct Ref const * value);
void gfx_material_set_u32(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, uint32_t const * value);
void gfx_material_set_s32(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, int32_t const * value);
void gfx_material_set_float(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, float const * value);

struct Ref * gfx_material_get_texture(struct Gfx_Material * material, uint32_t uniform_id);
uint32_t * gfx_material_get_u32(struct Gfx_Material * material, uint32_t uniform_id);
int32_t * gfx_material_get_s32(struct Gfx_Material * material, uint32_t uniform_id);
float * gfx_material_get_float(struct Gfx_Material * material, uint32_t uniform_id);

#endif
