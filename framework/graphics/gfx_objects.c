#include "framework/formatter.h"


//
#include "gfx_objects.h"

struct Handle gpu_target_get_texture(struct Handle handle, enum Texture_Flag flags, uint32_t index) {
	struct GPU_Target const * target = gpu_target_get(handle);
	if (target == NULL) { return (struct Handle){0}; }

	uint32_t attachments_count = 0;
	FOR_ARRAY(&target->textures, it) {
		struct Handle const * gh_texture = it.value;
		struct GPU_Texture const * texture = gpu_texture_get(*gh_texture);
		if (texture == NULL) { continue; }

		uint32_t const attachment = attachments_count;
		if (texture->format.flags & TEXTURE_FLAG_COLOR) { attachments_count++; }

		if (texture->format.flags == flags && attachment == index) {
			return *gh_texture;
		}
	}

	WRN("failure: target doesn't have requested texture");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct Handle){0};
}
