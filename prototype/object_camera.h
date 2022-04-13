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
	struct Camera_Params {
		enum Camera_Mode mode;
		float ncp, fcp, ortho;
	} params;
	struct Camera_Clear {
		enum Texture_Type mask;
		union {
			struct uvec4 rgba32_u;
			struct svec4 rgba32_s;
			struct  vec4 rgba32_f;
		} color;
	} clear;
	struct Ref gpu_target_ref;
};

//

struct mat4 camera_get_projection(
	struct Camera_Params const * params,
	uint32_t viewport_size_x, uint32_t viewport_size_y
);

#endif
