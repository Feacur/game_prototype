#if !defined(GAME_FRAMEWORK_MATHS)
#define GAME_FRAMEWORK_MATHS

#include "vector_types.h"

/*
// @note: left-handed
Y     
|     
|  Z  
| /   
|/    
+----X
`cross(X, Y) == Z`
`cross(Y, Z) == X`
`cross(Z, X) == Y`

// @note: expects functional `IEEE 754` float and double
half   [sign:1][exponent: 5][mantissa:10] == sign * (1 + mantissa / 2^10) * 2^(exponent -   15)
float  [sign:1][exponent: 8][mantissa:23] == sign * (1 + mantissa / 2^23) * 2^(exponent -  127)
double [sign:1][exponent:11][mantissa:52] == sign * (1 + mantissa / 2^52) * 2^(exponent - 1023)
-----
`exponent` is a "window" between powers of two: [0.25 .. 0.5], [0.5 .. 1], [1 .. 2], [2 .. 4]
`mantissa` is an "offset" within the window:    [0.5 .. 1] with [mantissa_max / 2] == 0.75
thus, each window precision is `(window_max - window_min) / mantissa_max`
-----
*/

#define MATHS_PI  3.14159265359f // convert_bits_u32_r32(UINT32_C(0x40490fdb))
#define MATHS_TAU 6.28318530718f // convert_bits_u32_r32(UINT32_C(0x40c90fdb))

#define FLOAT_NAN convert_bits_u32_r32(UINT32_C(0x7fc00000)) // [0x7f800001 .. 0x7fffffff] and [0xff800001 .. 0xffffffff]
#define FLOAT_POS_INFINITY convert_bits_u32_r32(UINT32_C(0x7f800000))
#define FLOAT_NEG_INFINITY convert_bits_u32_r32(UINT32_C(0xff800000))
#define FLOAT_MAX convert_bits_u32_r32(UINT32_C(0x7f7fffff)) //  3.4028237e+38
#define FLOAT_MIN convert_bits_u32_r32(UINT32_C(0xff7fffff)) // -3.4028237e+38
#define FLOAT_INT_MIN convert_bits_u32_r32(UINT32_C(0xcb800000)) // -16_777_216 == -2^24
#define FLOAT_INT_MAX convert_bits_u32_r32(UINT32_C(0x4b800000)) //  16_777_216 ==  2^24

#define DOUBLE_NAN convert_bits_u64_r64(UINT64_C(0x7ff8000000000000)) // [0x7ff0000000000001 .. 0x7fffffffffffffff] and [0xfff0000000000001 .. 0xffffffffffffffff]
#define DOUBLE_POS_INFINITY convert_bits_u64_r64(UINT64_C(0x7ff0000000000000))
#define DOUBLE_NEG_INFINITY convert_bits_u64_r64(UINT64_C(0xfff0000000000000))
#define DOUBLE_MAX convert_bits_u64_r64(UINT64_C(0x7fefffffffffffff)) //  1.79769313486231570815e+308
#define DOUBLE_MIN convert_bits_u64_r64(UINT64_C(0xffefffffffffffff)) // -1.79769313486231570815e+308
#define DOUBLE_INT_MIN convert_bits_u64_r64(UINT64_C(0xc340000000000000)) // -9_007_199_254_740_992 == -2^53
#define DOUBLE_INT_MAX convert_bits_u64_r64(UINT64_C(0x4340000000000000)) //  9_007_199_254_740_992 ==  2^53

float convert_bits_u32_r32(uint32_t value);
uint32_t convert_bits_r32_u32(float value);

float convert_bits_u64_r64(uint64_t value);
uint64_t convert_bits_r64_u64(float value);

#define U32_TO_R32_12(value) convert_bits_u32_r32(UINT32_C(0x3f800000) | (value >> 9))
#define U32_TO_R32_24(value) convert_bits_u32_r32(UINT32_C(0x40000000) | (value >> 9))

#define U64_TO_R64_12(value) convert_bits_u64_r64(UINT64_C(0x3ff0000000000000) | (value >> 9))
#define U64_TO_R64_24(value) convert_bits_u64_r64(UINT64_C(0x4000000000000000) | (value >> 9))

uint32_t hash_u32_bytes_fnv1(uint8_t const * value, uint64_t length);
uint32_t hash_u32_xorshift(uint32_t value);

uint64_t hash_u64_bytes_fnv1(uint8_t const * value, uint64_t length);
uint64_t hash_u64_xorshift(uint64_t value);

uint32_t round_up_to_PO2_u32(uint32_t value);
uint64_t round_up_to_PO2_u64(uint64_t value);

uint32_t mul_div_u32(uint32_t value, uint32_t numerator, uint32_t denominator);
uint64_t mul_div_u64(uint64_t value, uint64_t numerator, uint64_t denominator);

uint32_t midpoint_u32(uint32_t v1, uint32_t v2);
uint64_t midpoint_u64(uint64_t v1, uint64_t v2);

uint32_t min_u32(uint32_t v1, uint32_t v2);
uint32_t max_u32(uint32_t v1, uint32_t v2);
uint32_t clamp_u32(uint32_t v, uint32_t low, uint32_t high);

int32_t min_s32(int32_t v1, int32_t v2);
int32_t max_s32(int32_t v1, int32_t v2);
int32_t clamp_s32(int32_t v, int32_t low, int32_t high);

float min_r32(float v1, float v2);
float max_r32(float v1, float v2);
float clamp_r32(float v, float low, float high);

float lerp(float v1, float v2, float t);
float lerp_stable(float v1, float v2, float t);
float inverse_lerp(float v1, float v2, float value);

float eerp(float v1, float v2, float t);
float eerp_stable(float v1, float v2, float t);
float inverse_eerp(float v1, float v2, float value);

bool  maths_isinf(float value);
bool  maths_isnan(float value);
float maths_floor(float value);
float maths_ceil(float value);
float maths_sqrt(float value);
float maths_sin(float value);
float maths_cos(float value);
float maths_ldexp(float factor, int32_t power); // @note: `ldexp(a, b) == a * 2^b`
double maths_ldexp_double(double factor, int32_t power); // @note: `ldexp(a, b) == a * 2^b`

uint8_t  map01_to_u8(float value);
uint16_t map01_to_u16(float value);
uint32_t map01_to_u32(float value);

int8_t  map01_to_s8(float value);
int16_t map01_to_s16(float value);
int32_t map01_to_s32(float value);

// ----- ----- ----- ----- -----
//     uint32_t vectors
// ----- ----- ----- ----- -----

struct uvec2 uvec2_add(struct uvec2 v1, struct uvec2 v2);
struct uvec2 uvec2_sub(struct uvec2 v1, struct uvec2 v2);
struct uvec2 uvec2_mul(struct uvec2 v1, struct uvec2 v2);
struct uvec2 uvec2_div(struct uvec2 v1, struct uvec2 v2);
uint32_t     uvec2_dot(struct uvec2 v1, struct uvec2 v2);

struct uvec3 uvec3_add(struct uvec3 v1, struct uvec3 v2);
struct uvec3 uvec3_sub(struct uvec3 v1, struct uvec3 v2);
struct uvec3 uvec3_mul(struct uvec3 v1, struct uvec3 v2);
struct uvec3 uvec3_div(struct uvec3 v1, struct uvec3 v2);
uint32_t     uvec3_dot(struct uvec3 v1, struct uvec3 v2);

struct uvec4 uvec4_add(struct uvec4 v1, struct uvec4 v2);
struct uvec4 uvec4_sub(struct uvec4 v1, struct uvec4 v2);
struct uvec4 uvec4_mul(struct uvec4 v1, struct uvec4 v2);
struct uvec4 uvec4_div(struct uvec4 v1, struct uvec4 v2);
uint32_t     uvec4_dot(struct uvec4 v1, struct uvec4 v2);

struct uvec3 uvec3_cross(struct uvec3 v1, struct uvec3 v2);

// ----- ----- ----- ----- -----
//     int32_t vectors
// ----- ----- ----- ----- -----

struct svec2 svec2_add(struct svec2 v1, struct svec2 v2);
struct svec2 svec2_sub(struct svec2 v1, struct svec2 v2);
struct svec2 svec2_mul(struct svec2 v1, struct svec2 v2);
struct svec2 svec2_div(struct svec2 v1, struct svec2 v2);
int32_t      svec2_dot(struct svec2 v1, struct svec2 v2);

struct svec3 svec3_add(struct svec3 v1, struct svec3 v2);
struct svec3 svec3_sub(struct svec3 v1, struct svec3 v2);
struct svec3 svec3_mul(struct svec3 v1, struct svec3 v2);
struct svec3 svec3_div(struct svec3 v1, struct svec3 v2);
int32_t      svec3_dot(struct svec3 v1, struct svec3 v2);

struct svec4 svec4_add(struct svec4 v1, struct svec4 v2);
struct svec4 svec4_sub(struct svec4 v1, struct svec4 v2);
struct svec4 svec4_mul(struct svec4 v1, struct svec4 v2);
struct svec4 svec4_div(struct svec4 v1, struct svec4 v2);
int32_t      svec4_dot(struct svec4 v1, struct svec4 v2);

struct svec3 svec3_cross(struct svec3 v1, struct svec3 v2);

// ----- ----- ----- ----- -----
//     float vectors
// ----- ----- ----- ----- -----

struct vec2 vec2_add(struct vec2 v1, struct vec2 v2);
struct vec2 vec2_sub(struct vec2 v1, struct vec2 v2);
struct vec2 vec2_mul(struct vec2 v1, struct vec2 v2);
struct vec2 vec2_div(struct vec2 v1, struct vec2 v2);
float       vec2_dot(struct vec2 v1, struct vec2 v2);

struct vec3 vec3_add(struct vec3 v1, struct vec3 v2);
struct vec3 vec3_sub(struct vec3 v1, struct vec3 v2);
struct vec3 vec3_mul(struct vec3 v1, struct vec3 v2);
struct vec3 vec3_div(struct vec3 v1, struct vec3 v2);
float       vec3_dot(struct vec3 v1, struct vec3 v2);

struct vec4 vec4_add(struct vec4 v1, struct vec4 v2);
struct vec4 vec4_sub(struct vec4 v1, struct vec4 v2);
struct vec4 vec4_mul(struct vec4 v1, struct vec4 v2);
struct vec4 vec4_div(struct vec4 v1, struct vec4 v2);
float       vec4_dot(struct vec4 v1, struct vec4 v2);

struct vec3 vec3_cross(struct vec3 v1, struct vec3 v2);

struct vec2 vec2_norm(struct vec2 v);
struct vec3 vec3_norm(struct vec3 v);
struct vec4 vec4_norm(struct vec4 v);

// ----- ----- ----- ----- -----
//     quaternions
// ----- ----- ----- ----- -----

struct vec4 quat_set_axis(struct vec3 axis, float radians);
struct vec4 quat_set_radians(struct vec3 radians);
struct vec4 quat_mul(struct vec4 q1, struct vec4 q2);

struct vec4 quat_conjugate(struct vec4 q);
struct vec4 quat_reciprocal(struct vec4 q);
struct vec3 quat_transform(struct vec4 q, struct vec3 v);
void        quat_get_axes(struct vec4 q, struct vec3 * x, struct vec3 * y, struct vec3 * z);

// ----- ----- ----- ----- -----
//     matrices
// ----- ----- ----- ----- -----

struct mat4 mat4_set_transformation(struct vec3 position, struct vec3 scale, struct vec4 rotation);
struct mat4 mat4_set_inverse_transformation(struct vec3 position, struct vec3 scale, struct vec4 rotation);
struct mat4 mat4_set_projection(struct vec2 scale_xy, struct vec2 offset_xy, float ncp, float fcp, float ortho);

struct mat4 mat4_inverse_transformation(struct mat4 m);
struct vec4 mat4_mul_vec(struct mat4 m, struct vec4 v);
struct mat4 mat4_mul_mat(struct mat4 m1, struct mat4 m2);

#endif
