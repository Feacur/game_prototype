#include "framework/logger.h"

//
#include "material_override.h"

void const * gfx_material_override_find(struct Gfx_Material_Override const * override, uint32_t id, uint32_t size) {
	for (uint32_t i = 0, offset = override->offset; i < override->count; i++) {
		struct Gfx_Material_Override_Entry const * entry = (void *)(override->buffer->data + offset);
		if (entry->header.id == id) {
			if (entry->header.size == size) {
				return (void const *)entry->payload;
			}
			logger_to_console("override size mismatch\n"); DEBUG_BREAK();
			return NULL;
		}
		offset += sizeof(entry->header) + entry->header.size;
		offset = align_u32(offset);
	}
	return NULL;
}
