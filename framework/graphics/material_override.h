#if !defined(GAME_GRAPHICS_MATERIAL_OVERRIDE)
#define GAME_GRAPHICS_MATERIAL_OVERRIDE

#include "framework/common.h"

struct Gfx_Uniforms;

struct Gfx_Material_Override {
	struct Gfx_Uniforms const * uniforms;
	uint32_t offset, count;
};

#endif
