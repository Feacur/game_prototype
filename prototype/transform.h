#if !defined(GAME_TRANSFORM)
#define GAME_TRANSFORM

#include "framework/vector_types.h"

struct Transform {
	struct vec3 position;
	struct vec3 scale;
	struct vec4 rotation;
};

#endif
