#if !defined(GAME_PROTOTYPE_COMPONENTS)
#define GAME_PROTOTYPE_COMPONENTS

#include "framework/vector_types.h"

struct Transform_3D {
	struct vec3 scale;
	struct vec4 rotation;
	struct vec3 position;
};

struct Transform_2D {
	struct vec2 scale;
	struct vec2 rotation;
	struct vec2 position;
};

#endif
