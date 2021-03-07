#if !defined(GAME_VECTORS)
#define GAME_VECTORS

#include "common.h"

struct vec2 { float x, y; };
struct vec3 { float x, y, z; };
struct vec4 { float x, y, z, w; };

struct mat2 { struct vec2 x, y; };
struct mat3 { struct vec3 x, y, z; };
struct mat4 { struct vec4 x, y, z, w; };

#endif
