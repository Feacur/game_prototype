#if !defined(GAME_GRAPHICS_PASS)
#define GAME_GRAPHICS_PASS

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
	struct Gfx_Material * material;
	struct Ref gpu_mesh_ref;
	uint32_t offset, length;
};

#endif
