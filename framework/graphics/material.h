#if !defined(FRAMEWORK_GRAPHICS_MATERIAL)
#define FRAMEWORK_GRAPHICS_MATERIAL

#include "framework/containers/buffer.h"
#include "framework/containers/array_any.h"
#include "framework/containers/handle.h"

#include "types.h"

// ----- ----- ----- ----- -----
//     uniforms
// ----- ----- ----- ----- -----

struct Gfx_Uniforms_Entry {
	uint32_t id;
	uint32_t size, offset;
};

struct Gfx_Uniforms {
	struct Array_Any headers; // `struct Gfx_Uniforms_Entry`
	struct Buffer payload;
};

struct Gfx_Uniforms gfx_uniforms_init(void);
void gfx_uniforms_free(struct Gfx_Uniforms * uniforms);

void gfx_uniforms_clear(struct Gfx_Uniforms * uniforms);

struct CArray_Mut gfx_uniforms_get(struct Gfx_Uniforms const * uniforms, uint32_t uniform_id);
void gfx_uniforms_set(struct Gfx_Uniforms * uniforms, uint32_t uniform_id, struct CArray value);
void gfx_uniforms_push(struct Gfx_Uniforms * uniforms, uint32_t uniform_id, struct CArray value);

// ----- ----- ----- ----- -----
//     material
// ----- ----- ----- ----- -----

struct Gfx_Material {
	enum Blend_Mode blend_mode;
	enum Depth_Mode depth_mode;
	struct Gfx_Uniforms uniforms;
	//
	struct Handle gpu_program_handle;
};

struct Gfx_Material gfx_material_init(
	enum Blend_Mode blend_mode,
	enum Depth_Mode depth_mode
);
void gfx_material_free(struct Gfx_Material * material);

void gfx_material_set_shader(struct Gfx_Material * material, struct Handle gpu_handle);

#endif
