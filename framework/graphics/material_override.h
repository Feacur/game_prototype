#if !defined(GAME_GRAPHICS_MATERIAL_OVERRIDE)
#define GAME_GRAPHICS_MATERIAL_OVERRIDE

#include "framework/vector_types.h"

// @todo: make it flexible and able to contain multiple uniforms
#define GFX_MATERIAL_OVERRIDES_LIMIT 2
struct Gfx_Material_Override {
	uint32_t id;
	uint32_t length;
	union {
		struct mat4 mat4;
	} as;
};

#endif
