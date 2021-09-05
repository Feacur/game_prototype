#if !defined(GAME_FRAMEWORK_VECTOR_TYPES)
#define GAME_FRAMEWORK_VECTOR_TYPES

#include "common.h"

struct uvec2 { uint32_t x, y; };
struct uvec3 { uint32_t x, y, z; };
struct uvec4 { uint32_t x, y, z, w; };

struct svec2 { int32_t x, y; };
struct svec3 { int32_t x, y, z; };
struct svec4 { int32_t x, y, z, w; };

struct vec2 { float x, y; };
struct vec3 { float x, y, z; };
struct vec4 { float x, y, z, w; };

struct mat2 { struct vec2 x, y; };
struct mat3 { struct vec3 x, y, z; };
struct mat4 { struct vec4 x, y, z, w; };

//

extern struct vec2 const comp_identity;
extern struct vec4 const quat_identity;

extern struct mat2 const mat2_identity;
extern struct mat3 const mat3_identity;
extern struct mat4 const mat4_identity;

#endif
