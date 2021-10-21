#if !defined(GAME_GRAPHICS_MATERIAL_OVERRIDE)
#define GAME_GRAPHICS_MATERIAL_OVERRIDE

#include "framework/vector_types.h"

struct Gfx_Material_Override_Entry {
	struct {
		uint32_t id;
		uint32_t size;
	} header;
	uint8_t payload[FLEXIBLE_ARRAY];
};

struct Gfx_Material_Override {
	struct Array_Byte const * buffer;
	uint32_t offset, count;
};

#endif
