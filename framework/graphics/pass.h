#if !defined(GAME_FRAMEWORK_GRAPHICS_PASS)
#define GAME_FRAMEWORK_GRAPHICS_PASS

#include "framework/vectors.h"

#include "types.h"

struct Gfx_Material;
struct Gpu_Target;
struct Gpu_Mesh;

struct Render_Pass {
	uint32_t size_x, size_y;
	struct Gpu_Target * target;
	//
	enum Texture_Type clear_mask;
	uint32_t clear_rgba;
	//
	struct Blend_Mode blend_mode;
	struct Gfx_Material * material;
	struct Gpu_Mesh * mesh;
	//
	uint32_t camera_id, transform_id;
	struct mat4 camera, transform;
};

#endif
