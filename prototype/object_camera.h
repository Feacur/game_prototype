#if !defined(PROTOTYPE_OBJECT_CAMERA)
#define PROTOTYPE_OBJECT_CAMERA

#include "framework/graphics/gfx_types.h"

#include "application/app_components.h"

#include "proto_components.h"

enum Camera_Mode {
	CAMERA_MODE_NONE,     // map to normalized device
	CAMERA_MODE_SCREEN,   // map one-to-one with viewport
	CAMERA_MODE_ASPECT_X, // lock horizontal FOV, scale vertical FOV with aspect
	CAMERA_MODE_ASPECT_Y, // lock vertical FOV, scale horizontal FOV with aspect
};

struct Camera {
	struct Transform_3D transform;
	//
	struct Camera_Params {
		enum Camera_Mode mode;
		float ncp, fcp, ortho;
	} params;
	struct Camera_Clear {
		enum Texture_Flag flags;
		struct vec4 color;
	} clear;
	struct Handle ah_target;
	//
	struct uvec2 cached_size;
};

struct Camera camera_init(void);
void camera_free(struct Camera * camera);

struct mat4 camera_get_projection(
	struct Camera_Params const * params,
	struct uvec2 viewport_size
);

#endif
