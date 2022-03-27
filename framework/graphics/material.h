#if !defined(GAME_GRAPHICS_MATERIAL)
#define GAME_GRAPHICS_MATERIAL

#include "framework/containers/buffer.h"
#include "framework/containers/array_any.h"
#include "framework/containers/array_u32.h"
#include "framework/containers/array_s32.h"
#include "framework/containers/array_flt.h"
#include "framework/containers/ref.h"

#include "types.h"

// ----- ----- ----- ----- -----
//     uniforms
// ----- ----- ----- ----- -----

struct Gfx_Uniforms_Entry {
	uint32_t id;
	uint32_t size, offset;
};

struct Gfx_Uniforms {
	struct Array_Any headers;
	struct Buffer payload;
};

struct Gfx_Uniform_In {
	uint32_t size;
	void const * data;
};

struct Gfx_Uniform_Out {
	uint32_t size;
	void * data;
};

struct Gfx_Uniforms gfx_uniforms_init(void);
void gfx_uniforms_free(struct Gfx_Uniforms * uniforms);

void gfx_uniforms_clear(struct Gfx_Uniforms * uniforms);

struct Gfx_Uniform_Out gfx_uniforms_get(struct Gfx_Uniforms const * uniforms, uint32_t uniform_id);
void gfx_uniforms_set(struct Gfx_Uniforms * uniforms, uint32_t uniform_id, struct Gfx_Uniform_In value);
void gfx_uniforms_push(struct Gfx_Uniforms * uniforms, uint32_t uniform_id, struct Gfx_Uniform_In value);

// ----- ----- ----- ----- -----
//     material
// ----- ----- ----- ----- -----

struct Gfx_Material {
	struct Ref gpu_program_ref;
	struct Blend_Mode blend_mode;
	struct Depth_Mode depth_mode;
	struct Gfx_Uniforms uniforms;
};

struct Gfx_Material gfx_material_init(
	struct Ref gpu_program_ref,
	struct Blend_Mode const * blend_mode,
	struct Depth_Mode const * depth_mode
);
void gfx_material_free(struct Gfx_Material * material);

#endif
