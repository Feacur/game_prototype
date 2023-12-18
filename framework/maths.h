#if !defined(FRAMEWORK_MATHS)
#define FRAMEWORK_MATHS

#include "maths_types.h"

/*
// @note: left-handed

(0, 1, 0)
Y     
|     
|  Z  (0, 0, 1)
| /   
|/    
+----X (1, 0, 0)

`cross(X, Y) == Z`
`cross(Y, Z) == X`
`cross(Z, X) == Y`

why precisely so? because
- I like it
- I'm used to it
- I don't really care about people walking on a plane
- I want screen-space and world-space have X-right and Y-up

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

#define R32_NAN convert_bits_u32_r32(UINT32_C(0x7fc00000)) // [0x7f800001 .. 0x7fffffff] and [0xff800001 .. 0xffffffff]
#define R32_POS_INFINITY convert_bits_u32_r32(UINT32_C(0x7f800000))
#define R32_NEG_INFINITY convert_bits_u32_r32(UINT32_C(0xff800000))
#define R32_MAX convert_bits_u32_r32(UINT32_C(0x7f7fffff)) //  3.4028237e+38
#define R32_MIN convert_bits_u32_r32(UINT32_C(0xff7fffff)) // -3.4028237e+38
#define R32_SEQUENTIAL_INT_MIN convert_bits_u32_r32(UINT32_C(0xcb800000)) // -16_777_216 == -2^24
#define R32_SEQUENTIAL_INT_MAX convert_bits_u32_r32(UINT32_C(0x4b800000)) //  16_777_216 ==  2^24

#define R64_NAN convert_bits_u64_r64(UINT64_C(0x7ff8000000000000)) // [0x7ff0000000000001 .. 0x7fffffffffffffff] and [0xfff0000000000001 .. 0xffffffffffffffff]
#define R64_POS_INFINITY convert_bits_u64_r64(UINT64_C(0x7ff0000000000000))
#define R64_NEG_INFINITY convert_bits_u64_r64(UINT64_C(0xfff0000000000000))
#define R64_MAX convert_bits_u64_r64(UINT64_C(0x7fefffffffffffff)) //  1.79769313486231570815e+308
#define R64_MIN convert_bits_u64_r64(UINT64_C(0xffefffffffffffff)) // -1.79769313486231570815e+308
#define R64_SEQUENTIAL_INT_MIN convert_bits_u64_r64(UINT64_C(0xc340000000000000)) // -9_007_199_254_740_992 == -2^53
#define R64_SEQUENTIAL_INT_MAX convert_bits_u64_r64(UINT64_C(0x4340000000000000)) //  9_007_199_254_740_992 ==  2^53

float convert_bits_u32_r32(uint32_t value);
uint32_t convert_bits_r32_u32(float value);

double convert_bits_u64_r64(uint64_t value);
uint64_t convert_bits_r64_u64(double value);

#define U32_TO_R32_12(value) convert_bits_u32_r32(UINT32_C(0x3f800000) | (value >> 9))
#define U32_TO_R32_24(value) convert_bits_u32_r32(UINT32_C(0x40000000) | (value >> 9))

#define U64_TO_R64_12(value) convert_bits_u64_r64(UINT64_C(0x3ff0000000000000) | (value >> 9))
#define U64_TO_R64_24(value) convert_bits_u64_r64(UINT64_C(0x4000000000000000) | (value >> 9))

uint32_t hash_u32_bytes_fnv1(uint8_t const * value, uint64_t length);
uint32_t hash_u32_xorshift(uint32_t value);

uint64_t hash_u64_bytes_fnv1(uint8_t const * value, uint64_t length);
uint64_t hash_u64_xorshift(uint64_t value);

uint32_t po2_next_u32(uint32_t value);
uint64_t po2_next_u64(uint64_t value);

uint32_t mul_div_u32(uint32_t value, uint32_t mul, uint32_t div);
uint64_t mul_div_u64(uint64_t value, uint64_t mul, uint64_t div);
size_t mul_div_size(size_t value, size_t mul, size_t div);

uint32_t midpoint_u32(uint32_t v1, uint32_t v2);
uint64_t midpoint_u64(uint64_t v1, uint64_t v2);
size_t midpoint_size(size_t v1, size_t v2);

uint32_t min_u32(uint32_t v1, uint32_t v2);
uint32_t max_u32(uint32_t v1, uint32_t v2);
uint32_t clamp_u32(uint32_t v, uint32_t low, uint32_t high);

int32_t min_s32(int32_t v1, int32_t v2);
int32_t max_s32(int32_t v1, int32_t v2);
int32_t clamp_s32(int32_t v, int32_t low, int32_t high);

float min_r32(float v1, float v2);
float max_r32(float v1, float v2);
float clamp_r32(float v, float low, float high);

size_t min_size(size_t v1, size_t v2);
size_t max_size(size_t v1, size_t v2);
size_t clamp_size(size_t v, size_t low, size_t high);

float lerp(float v1, float v2, float t);
float lerp_stable(float v1, float v2, float t);
float inverse_lerp(float v1, float v2, float value);

float eerp(float v1, float v2, float t);
float eerp_stable(float v1, float v2, float t);
float inverse_eerp(float v1, float v2, float value);

bool  r32_isinf(float value);
bool  r32_isnan(float value);
float r32_floor(float value);
float r32_ceil(float value);
float r32_sqrt(float value);
float r32_sin(float value);
float r32_cos(float value);
float r32_ldexp(float factor, int32_t power); // @note: `ldexp(a, b) == a * 2^b`
float r32_trunc(float value);
float r32_log2(float value);
float r32_loge(float value);
float r32_log10(float value);

bool   r64_isinf(double value);
bool   r64_isnan(double value);
double r64_floor(double value);
double r64_ceil(double value);
double r64_sqrt(double value);
double r64_sin(double value);
double r64_cos(double value);
double r64_ldexp(double factor, int32_t power); // @note: `ldexp(a, b) == a * 2^b`
double r64_trunc(double value);
double r64_log2(double value);
double r64_loge(double value);
double r64_log10(double value);

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

uint32_t uvec2_cross(struct uvec2 v1, struct uvec2 v2);
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

int32_t svec2_cross(struct svec2 v1, struct svec2 v2);
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

float vec2_cross(struct vec2 v1, struct vec2 v2);
struct vec3 vec3_cross(struct vec3 v1, struct vec3 v2);

struct vec2 vec2_norm(struct vec2 v);
struct vec3 vec3_norm(struct vec3 v);
struct vec4 vec4_norm(struct vec4 v);

// ----- ----- ----- ----- -----
//     quaternions
// ----- ----- ----- ----- -----

struct vec4 quat_axis(struct vec3 axis, float radians);
struct vec4 quat_radians(struct vec3 radians);
struct vec4 quat_mul(struct vec4 q1, struct vec4 q2);

struct vec4 quat_conjugate(struct vec4 q);
struct vec4 quat_reciprocal(struct vec4 q);
struct vec3 quat_transform(struct vec4 q, struct vec3 v);
void        quat_get_axes(struct vec4 q, struct vec3 * x, struct vec3 * y, struct vec3 * z);

// ----- ----- ----- ----- -----
//     complex
// ----- ----- ----- ----- -----

struct vec2 comp_radians(float radians);

// ----- ----- ----- ----- -----
//     matrices
// ----- ----- ----- ----- -----

struct mat4 mat4_transformation(struct vec3 position, struct vec3 scale, struct vec4 rotation);
struct mat4 mat4_inverse_transformation(struct vec3 position, struct vec3 scale, struct vec4 rotation);

struct mat4 mat4_projection(
	struct vec2 scale_xy, struct vec2 offset_xy,
	float view_near, float view_far, float ortho,
	float ndc_near, float ndc_far
);

struct mat4 mat4_invert_transformation(struct mat4 m);
struct vec4 mat4_mul_vec(struct mat4 m, struct vec4 v);
struct mat4 mat4_mul_mat(struct mat4 m1, struct mat4 m2);

#endif
