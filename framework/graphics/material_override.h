#if !defined(GAME_GRAPHICS_MATERIAL_OVERRIDE)
#define GAME_GRAPHICS_MATERIAL_OVERRIDE

#include "framework/containers/buffer.h"

struct Gfx_Material_Override_Entry {
	struct {
		uint32_t id;
		uint32_t size;
	} header;
	uint8_t payload[FLEXIBLE_ARRAY];
};

struct Gfx_Material_Override {
	struct Buffer const * buffer;
	uint32_t offset, count;
};

void const * gfx_material_override_find(struct Gfx_Material_Override const * override, uint32_t id, uint32_t size);

#endif
