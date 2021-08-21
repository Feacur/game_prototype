#if !defined(GAME_GRAPHICS_PASS)
#define GAME_GRAPHICS_PASS

// interface from `gpu_*/graphics.c` to anywhere
// - gpu draw call

#include "framework/vector_types.h"
#include "framework/containers/ref.h"

#include "types.h"

struct Gfx_Material;

struct Render_Pass {
	uint32_t screen_size_x, screen_size_y;
	struct Ref gpu_target_ref;
	struct Blend_Mode blend_mode;
	struct Depth_Mode depth_mode;
	//
	enum Texture_Type clear_mask;
	uint32_t clear_rgba;
	//
	struct Gfx_Material const * material;
	struct Ref gpu_mesh_ref;
	uint32_t offset, length;
};

void graphics_draw(struct Render_Pass const * pass);

#endif
