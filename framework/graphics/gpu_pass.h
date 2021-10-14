#if !defined(GAME_GRAPHICS_GPU_PASS)
#define GAME_GRAPHICS_GPU_PASS

// interface from `gpu_*/graphics.c` to anywhere
// - gpu draw call

#include "framework/vector_types.h"
#include "framework/containers/ref.h"

#include "types.h"

struct Gfx_Material;

enum Render_Pass_Type {
	RENDER_PASS_TYPE_CULL,
	RENDER_PASS_TYPE_TARGET,
	RENDER_PASS_TYPE_CLEAR,
	RENDER_PASS_TYPE_DRAW,
};

struct Render_Pass_Cull {
	enum Cull_Mode mode;
	enum Winding_Order order;
};

struct Render_Pass_Target {
	uint32_t screen_size_x, screen_size_y;
	struct Ref gpu_ref;
};

struct Render_Pass_Clear {
	enum Texture_Type mask;
	uint32_t rgba;
};

struct Render_Pass_Draw {
	struct Gfx_Material const * material;
	struct Ref gpu_mesh_ref;
	uint32_t offset, length;
};

struct Render_Pass {
	enum Render_Pass_Type type;
	union {
		struct Render_Pass_Cull   cull;
		struct Render_Pass_Target target;
		struct Render_Pass_Clear  clear;
		struct Render_Pass_Draw   draw;
	} as;
};

void graphics_process(struct Render_Pass const * pass);

#endif
