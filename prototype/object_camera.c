#include "framework/maths.h"
#include "framework/logger.h"
#include "framework/graphics/gpu_misc.h"

//
#include "object_camera.h"

struct mat4 camera_get_projection(
	struct Camera_Params const * params,
	uint32_t viewport_size_x, uint32_t viewport_size_y
) {
	switch (params->mode) {
		case CAMERA_MODE_NONE:
			return c_mat4_identity;

		case CAMERA_MODE_SCREEN:
			return graphics_set_projection_mat4(
				(struct vec2){2 / (float)viewport_size_x, 2 / (float)viewport_size_y},
				(struct vec2){-1, -1},
				params->ncp, params->fcp, params->ortho
			);

		case CAMERA_MODE_ASPECT_X:
			return graphics_set_projection_mat4(
				(struct vec2){1, (float)viewport_size_x / (float)viewport_size_y},
				(struct vec2){0, 0},
				params->ncp, params->fcp, params->ortho
			);

		case CAMERA_MODE_ASPECT_Y:
			return graphics_set_projection_mat4(
				(struct vec2){(float)viewport_size_y / (float)viewport_size_x, 1},
				(struct vec2){0, 0},
				params->ncp, params->fcp, params->ortho
			);
	}

	logger_to_console("unknown camera mode\n"); DEBUG_BREAK();
	return (struct mat4){0};
}
