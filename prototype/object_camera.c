#include "framework/maths.h"
#include "framework/logger.h"

//
#include "object_camera.h"

struct mat4 camera_get_projection(
	struct Camera const * camera,
	uint32_t camera_size_x, uint32_t camera_size_y
) {
	switch (camera->mode) {
		case CAMERA_MODE_NONE: // @note: basically normalized device coordinates
			// @note: is equivalent of `CAMERA_MODE_ASPECT_X` or `CAMERA_MODE_ASPECT_Y`
			//        with `camera_size_x == camera_size_y`
			return mat4_identity;

		case CAMERA_MODE_SCREEN:
			return mat4_set_projection(
				(struct vec2){2 / (float)camera_size_x, 2 / (float)camera_size_y},
				(struct vec2){-1, -1},
				camera->ncp, camera->fcp, camera->ortho
			);

		case CAMERA_MODE_ASPECT_X:
			return mat4_set_projection(
				(struct vec2){1, (float)camera_size_x / (float)camera_size_y},
				(struct vec2){0, 0},
				camera->ncp, camera->fcp, camera->ortho
			);

		case CAMERA_MODE_ASPECT_Y:
			return mat4_set_projection(
				(struct vec2){(float)camera_size_y / (float)camera_size_x, 1},
				(struct vec2){0, 0},
				camera->ncp, camera->fcp, camera->ortho
			);
	}

	logger_to_console("unknown camera mode"); DEBUG_BREAK();
	return (struct mat4){0};
}
