#if !defined(GAME_PROTOTYPE_OBJECT_CAMERA)
#define GAME_PROTOTYPE_OBJECT_CAMERA

#include "framework/containers/ref.h"
#include "framework/graphics/types.h"

#include "components.h"

enum Camera_Mode {
	CAMERA_MODE_NONE,
	CAMERA_MODE_SCREEN,
	CAMERA_MODE_ASPECT_X,
	CAMERA_MODE_ASPECT_Y,
};

struct Camera {
	struct Transform_3D transform;
	//
	enum Camera_Mode mode;
	float ncp, fcp, ortho;
	//
	struct Ref gpu_target_ref;
	enum Texture_Type clear_mask;
	uint32_t clear_rgba;
};

//

struct mat4 camera_get_projection(
	struct Camera const * camera,
	uint32_t camera_size_x, uint32_t camera_size_y
);

#endif
