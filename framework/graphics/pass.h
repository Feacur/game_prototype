#if !defined(GAME_GRAPHICS_PASS)
#define GAME_GRAPHICS_PASS

#include "framework/vector_types.h"
#include "framework/containers/ref.h"

#include "types.h"

struct Gfx_Material;
struct Gpu_Target;
struct Gpu_Mesh;

struct Render_Pass {
	uint32_t size_x, size_y;
	struct Gpu_Target * target;
	struct Blend_Mode blend_mode;
	struct Depth_Mode depth_mode;
	//
	enum Texture_Type clear_mask;
	uint32_t clear_rgba;
	//
	struct Gfx_Material * material;
	struct Ref mesh;
	uint32_t offset, length;
};

#endif
