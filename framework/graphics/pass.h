#if !defined(GAME_FRAMEWORK_GRAPHICS_PASS)
#define GAME_FRAMEWORK_GRAPHICS_PASS

#include "framework/vector_types.h"

#include "types.h"

struct Gfx_Material;
struct Gpu_Target;
struct Gpu_Mesh;

struct Render_Pass {
	uint32_t size_x, size_y;
	struct Gpu_Target * target;
	struct Blend_Mode blend_mode;
	bool depth_enabled; bool depth_mask;
	//
	enum Texture_Type clear_mask;
	uint32_t clear_rgba;
	//
	struct Gfx_Material * material;
	struct Gpu_Mesh * mesh;
	//
	uint32_t camera_id, transform_id;
	struct mat4 camera, transform;
};

#endif
