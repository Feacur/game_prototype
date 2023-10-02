#if !defined(FRAMEWORK_MATHS_TYPES)
#define FRAMEWORK_MATHS_TYPES

#include "common.h"

/*
// @note: matrices are neither column-major, nor row-major

they are axes-major, if you will
both OpenGL and DirectX are axes-major too

the difference is:
- OpenGL  is column-major, and thus `struct matN` fields go into columns, which demands `matrix x vector`
- DirectX is row-major,    and thus `struct matN` fields go into rows,    which demands `vector x matrix`

although if matrices are transposed first, DirectX will happily do `matrix x vector` no issue

in `math.h` we have `mat4_mul_vec`, which implies by math convention that vector is a column of numbers:
we dot product each row of a matrix with a vector and get a new transformed column vector

but nothing prevents us from having a `vec4_mul_mat`, which implies the said vector is a row of numbers:
we dot product a vector with each column of a matrix and get a new transformed row vector

keep that in mind. also read notes in the `maths.c`
*/

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

struct urect { struct uvec2 min, max; };
struct srect { struct svec2 min, max; };
struct rect { struct vec2 min, max; };

//

extern struct vec2 const c_comp_identity;
extern struct vec4 const c_quat_identity;

extern struct mat2 const c_mat2_identity;
extern struct mat3 const c_mat3_identity;
extern struct mat4 const c_mat4_identity;

#endif
