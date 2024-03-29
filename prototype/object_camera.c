#include "framework/maths.h"
#include "framework/formatter.h"
#include "framework/systems/assets.h"
#include "framework/graphics/misc.h"


//
#include "object_camera.h"

struct Camera camera_init(void) {
	return (struct Camera){0};
}

void camera_free(struct Camera * camera) {
	system_assets_drop(camera->ah_target);
}

struct mat4 camera_get_projection(
	struct Camera_Params const * params,
	struct uvec2 viewport_size
) {
	switch (params->mode) {
		case CAMERA_MODE_NONE:
			return c_mat4_identity;

		case CAMERA_MODE_SCREEN:
			return graphics_projection_mat4(
				(struct vec2){2 / (float)viewport_size.x, 2 / (float)viewport_size.y},
				(struct vec2){-1, -1},
				params->ncp, params->fcp, params->ortho
			);

		case CAMERA_MODE_ASPECT_X:
			return graphics_projection_mat4(
				(struct vec2){1, (float)viewport_size.x / (float)viewport_size.y},
				(struct vec2){0, 0},
				params->ncp, params->fcp, params->ortho
			);

		case CAMERA_MODE_ASPECT_Y:
			return graphics_projection_mat4(
				(struct vec2){(float)viewport_size.y / (float)viewport_size.x, 1},
				(struct vec2){0, 0},
				params->ncp, params->fcp, params->ortho
			);
	}

	ERR("unknown camera mode");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct mat4){0};
}
