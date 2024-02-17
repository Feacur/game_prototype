#include "framework/maths.h"
#include "framework/formatter.h"


//
#include "gfx_objects.h"

// ----- ----- ----- ----- -----
//     GPU program part
// ----- ----- ----- ----- -----

// ----- ----- ----- ----- -----
//     GPU sampler part
// ----- ----- ----- ----- -----

// ----- ----- ----- ----- -----
//     GPU texture part
// ----- ----- ----- ----- -----

uint32_t gpu_texture_get_levels(struct GPU_Texture const * gpu_texture) {
	return min_u32(
		max_u32(gpu_texture->settings.levels, 1),
		max_u32((uint32_t)r32_log2((float)max_u32(
			gpu_texture->size.x, gpu_texture->size.y
		)), 1)
	);
}

// ----- ----- ----- ----- -----
//     GPU unit part
// ----- ----- ----- ----- -----

// ----- ----- ----- ----- -----
//     GPU target part
// ----- ----- ----- ----- -----

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

// ----- ----- ----- ----- -----
//     GPU buffer part
// ----- ----- ----- ----- -----

// ----- ----- ----- ----- -----
//     GPU mesh part
// ----- ----- ----- ----- -----
