#if !defined(GAME_FRAMEWORK_GRAPHICS_PASS)
#define GAME_FRAMEWORK_GRAPHICS_PASS

#include "framework/vectors.h"

#include "types.h"

struct Gfx_Material;
struct Gpu_Target;
struct Gpu_Mesh;

struct Render_Pass {
	struct Gfx_Material * material;
	struct Gpu_Target * target;
	struct Gpu_Mesh * mesh;
	struct Blend_Mode blend_mode;
	uint32_t camera_id, transform_id;
	struct mat4 camera, transform;
};

#endif
