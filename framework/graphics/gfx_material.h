#if !defined(FRAMEWORK_GRAPHICS_MATERIAL)
#define FRAMEWORK_GRAPHICS_MATERIAL

#include "framework/containers/buffer.h"
#include "framework/containers/array.h"

#include "gfx_types.h"

// ----- ----- ----- ----- -----
//     uniforms
// ----- ----- ----- ----- -----

struct Gfx_Uniforms_Iterator {
	uint32_t current, next;
	struct Handle id;
	uint32_t size;
	void * value;
};

struct Gfx_Uniforms_Entry {
	struct Handle id;
	uint32_t size, offset;
};

struct Gfx_Uniforms {
	struct Array headers; // `struct Gfx_Uniforms_Entry`
	struct Buffer payload;
};

struct Gfx_Uniforms gfx_uniforms_init(void);
void gfx_uniforms_free(struct Gfx_Uniforms * uniforms);

void gfx_uniforms_clear(struct Gfx_Uniforms * uniforms);

struct CBuffer_Mut gfx_uniforms_get(struct Gfx_Uniforms const * uniforms, struct CString name, uint32_t offset);
void gfx_uniforms_push(struct Gfx_Uniforms * uniforms, struct CString name, struct CBuffer value);

struct CBuffer_Mut gfx_uniforms_id_get(struct Gfx_Uniforms const * uniforms, struct Handle sh_name, uint32_t offset);
void gfx_uniforms_id_push(struct Gfx_Uniforms * uniforms, struct Handle sh_name, struct CBuffer value);

bool gfx_uniforms_iterate(struct Gfx_Uniforms const * uniforms, struct Gfx_Uniforms_Iterator * iterator);

#define FOR_GFX_UNIFORMS(data, it) for ( \
	struct Gfx_Uniforms_Iterator it = {0}; \
	gfx_uniforms_iterate(data, &it); \
) \

// ----- ----- ----- ----- -----
//     material
// ----- ----- ----- ----- -----

struct Gfx_Material {
	enum Blend_Mode blend_mode;
	enum Depth_Mode depth_mode;
	struct Gfx_Uniforms uniforms;
	//
	struct Handle gh_program;
};

struct Gfx_Material gfx_material_init(void);
void gfx_material_free(struct Gfx_Material * material);

void gfx_material_set_shader(struct Gfx_Material * material, struct Handle gpu_handle);

#endif
