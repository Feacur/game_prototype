#include "framework/formatter.h"

#include <math.h>


//
#include "maths.h"

float bits_u32_r32(uint32_t value) {
	union {
		uint32_t value_u32;
		float value_r32;
	} data;
	data.value_u32 = value;
	return data.value_r32;
}

uint32_t bits_r32_u32(float value) {
	union {
		uint32_t value_u32;
		float value_r32;
	} data;
	data.value_r32 = value;
	return data.value_u32;
}

double bits_u64_r64(uint64_t value) {
	union {
		uint64_t value_u64;
		double value_r64;
	} data;
	data.value_u64 = value;
	return data.value_r64;
}

uint64_t bits_r64_u64(double value) {
	union {
		uint64_t value_u64;
		double value_r64;
	} data;
	data.value_r64 = value;
	return data.value_u64;
}

uint32_t hash_u32_fnv1(uint8_t const * value, size_t size) {
	uint32_t hash = UINT32_C(2166136261);
	for (size_t i = 0; i < size; i++) {
		hash *= UINT32_C(16777619);
		hash ^= value[i];
	}
	return hash;
}

uint32_t hash_u32_xorshift(uint32_t value) {
	value ^= value << 13;
	value ^= value >> 17;
	value ^= value <<  5;
	return value;
}

uint64_t hash_u64_fnv1(uint8_t const * value, size_t size) {
	uint64_t hash = UINT64_C(14695981039346656037);
	for (size_t i = 0; i < size; i++) {
		hash *= UINT64_C(1099511628211);
		hash ^= value[i];
	}
	return hash;
}

uint64_t hash_u64_xorshift(uint64_t value) {
	value ^= value << 13;
	value ^= value >>  7;
	value ^= value << 17;
	return value;
}

uint32_t po2_next_u32(uint32_t value) {
#if !defined(GAME_TARGET_RELEASE)
	if (value > (UINT32_C(1) << 31)) {
		ERR("PO2 overflow: %u", value);
		REPORT_CALLSTACK(); DEBUG_BREAK(); return 0;
	}
#endif

	value -= 1;
	value |= value >>  1;
	value |= value >>  2;
	value |= value >>  4;
	value |= value >>  8;
	value |= value >> 16;
	return value + 1;
}

uint64_t po2_next_u64(uint64_t value) {
#if !defined(GAME_TARGET_RELEASE)
	if (value > (UINT64_C(1) << 63)) {
		ERR("PO2 overflow: %llu", value);
		REPORT_CALLSTACK(); DEBUG_BREAK(); return 0;
	}
#endif

	value -= 1;
	value |= value >>  1;
	value |= value >>  2;
	value |= value >>  4;
	value |= value >>  8;
	value |= value >> 16;
	value |= value >> 32;
	return value + 1;
}

uint32_t mul_div_u32(uint32_t value, uint32_t mul, uint32_t div) {
	return (value / div) * mul
	     + (value % div) * mul / div;
}

uint64_t mul_div_u64(uint64_t value, uint64_t mul, uint64_t div) {
	return (value / div) * mul
	     + (value % div) * mul / div;
}

size_t mul_div_size(size_t value, size_t mul, size_t div) {
	return (value / div) * mul
	     + (value % div) * mul / div;
}

uint32_t midpoint_u32(uint32_t v1, uint32_t v2) {
	return (v1 < v2)
		? v1 + (v2 - v1) / 2
		: v2 + (v1 - v2) / 2;
}

uint64_t midpoint_u64(uint64_t v1, uint64_t v2) {
	return (v1 < v2)
		? v1 + (v2 - v1) / 2
		: v2 + (v1 - v2) / 2;
}

size_t midpoint_size(size_t v1, size_t v2) {
	return (v1 < v2)
		? v1 + (v2 - v1) / 2
		: v2 + (v1 - v2) / 2;
}

uint32_t min_u32(uint32_t v1, uint32_t v2) { return (v1 < v2) ? v1 : v2; }
uint32_t max_u32(uint32_t v1, uint32_t v2) { return (v1 > v2) ? v1 : v2; }
uint32_t clamp_u32(uint32_t v, uint32_t low, uint32_t high) { return min_u32(max_u32(v, low), high); }

int32_t min_s32(int32_t v1, int32_t v2) { return (v1 < v2) ? v1 : v2; }
int32_t max_s32(int32_t v1, int32_t v2) { return (v1 > v2) ? v1 : v2; }
int32_t clamp_s32(int32_t v, int32_t low, int32_t high) { return min_s32(max_s32(v, low), high); }

float min_r32(float v1, float v2) { return (v1 < v2) ? v1 : v2; }
float max_r32(float v1, float v2) { return (v1 > v2) ? v1 : v2; }
float clamp_r32(float v, float low, float high) { return min_r32(max_r32(v, low), high); }

size_t min_size(size_t v1, size_t v2) { return (v1 < v2) ? v1 : v2; }
size_t max_size(size_t v1, size_t v2) { return (v1 > v2) ? v1 : v2; }
size_t clamp_size(size_t v, size_t low, size_t high) { return min_size(max_size(v, low), high); }

float lerp(float v1, float v2, float t) { return v1 + (v2 - v1)*t; }
float lerp_stable(float v1, float v2, float t) { return v1*(1 - t) + v2*t; }
float inverse_lerp(float v1, float v2, float value) { return (value - v1) / (v2 - v1); }

float eerp(float v1, float v2, float t) { return v1 * powf(v2 / v1, t); }
float eerp_stable(float v1, float v2, float t) { return powf(v1, (1 - t)) * powf(v2, t); }
float inverse_eerp(float v1, float v2, float value) { return logf(value / v1) / logf(v2 / v1); }

bool r32_isinf(float value) {
	// @note: check all exponent bits and no mantissa bits are set
	// uint32_t const bits = convert_bits_r32_u32(value);
	// return (bits & UINT32_C(0x7fffffff)) == UINT32_C(0x7f800000);
	return isinf(value);
}
bool r32_isnan(float value) {
	// @note: check all exponent bits and some mantissa bits are set
	// uint32_t const bits = convert_bits_r32_u32(value);
	// return (bits & UINT32_C(0x7f800000)) == UINT32_C(0x7f800000)
	//     && (bits & UINT32_C(0x7fffffff)) != UINT32_C(0x7f800000);
	return isnan(value);
}
float r32_floor(float value) { return floorf(value); }
float r32_ceil(float value) { return ceilf(value); }
float r32_sqrt(float value) { return sqrtf(value); }
float r32_sin(float value) { return sinf(value); }
float r32_cos(float value) { return cosf(value); }
float r32_ldexp(float factor, int32_t power) { return ldexpf(factor, power); }
float r32_trunc(float value) { return truncf(value); }
float r32_log2(float value) { return log2f(value); }
float r32_loge(float value) { return logf(value); }
float r32_log10(float value) { return log10f(value); }

bool r64_isinf(double value) {
	// @note: check all exponent bits and no mantissa bits are set
	// uint64_t const bits = convert_bits_r64_u64(value);
	// return (bits & UINT64_C(0x7fffffffffffffff)) == UINT64_C(0x7ff0000000000000);
	return isinf(value);
}
bool r64_isnan(double value) {
	// @note: check all exponent bits and some mantissa bits are set
	// uint64_t const bits = convert_bits_r64_u64(value);
	// return (bits & UINT64_C(0x7ff0000000000000)) == UINT64_C(0x7ff0000000000000)
	//     && (bits & UINT64_C(0x7fffffffffffffff)) != UINT64_C(0x7ff0000000000000);
	return isnan(value);
}
double r64_floor(double value) { return floor(value); }
double r64_ceil(double value) { return ceil(value); }
double r64_sqrt(double value) { return sqrt(value); }
double r64_sin(double value) { return sin(value); }
double r64_cos(double value) { return cos(value); }
double r64_ldexp(double factor, int32_t power) { return ldexp(factor, power); }
double r64_trunc(double value) { return trunc(value); }
double r64_log2(double value) { return log2(value); }
double r64_loge(double value) { return log(value); }
double r64_log10(double value) { return log10(value); }

uint8_t map01_to_u8(float value) {
	if (value < 0) { return 0; }
	if (value > 1) { return UINT8_MAX; }
	return (uint8_t)(value * (float)UINT8_MAX);
}

uint16_t map01_to_u16(float value) {
	if (value < 0) { return 0; }
	if (value > 1) { return UINT16_MAX; }
	return (uint16_t)(value * (float)UINT16_MAX);
}

uint32_t map01_to_u32(float value) {
	if (value < 0) { return 0; }
	if (value > 1) { return UINT32_MAX; }
	return (uint32_t)(value * (float)UINT32_MAX);
}

int8_t map01_to_s8(float value) {
	if (value < -1) { return INT8_MIN; }
	if (value >  1) { return INT8_MAX; }
	return (value > 0)
		? (int8_t)(-value * (float)INT8_MIN)
		: (int8_t)( value * (float)INT8_MAX);
}

int16_t map01_to_s16(float value) {
	if (value < -1) { return INT16_MIN; }
	if (value >  1) { return INT16_MAX; }
	return (value > 0)
		? (int16_t)(-value * (float)INT16_MIN)
		: (int16_t)( value * (float)INT16_MAX);
}

int32_t map01_to_s32(float value) {
	if (value < -1) { return INT32_MIN; }
	if (value >  1) { return INT32_MAX; }
	return (value > 0)
		? (int32_t)(-value * (float)INT32_MIN)
		: (int32_t)( value * (float)INT32_MAX);
}

// ----- ----- ----- ----- -----
//     any vectors
// ----- ----- ----- ----- -----

/* -- vectors

> imaginary basis
  +------------+
  |    i  j  k |
  | i -1  k -j |
  | j -k -1  i |
  | k  j -i -1 |
  +------------+
  v = i*a1 + j*a2 + k*a3 + ...
  v1 * v2 = (i*a1 + j*a2 + k*a3 + ...) * (i*b1 + j*b2 + k*b3 + ...)
          = -dot(v1, v2) + cross(v1, v2)

> bivector basis
  +---------------+
  |     x   y   z |
  | x   1  xy  xz |
  | y -xy   1  yz |
  | z -xz -yz   1 |
  +---------------+
  v = x*S1 + y*S2 + z*S3 + ...
  v1 * v2 = (x*a1 + y*a2 + z*a3 + ...) * (x*b1 + y*b2 + z*b3 + ...)
          = dot(v1, v2) + wedge(v1, v2)

> correspondence in 3d
  i ~ yz
  j ~ zx
  k ~ xy

> vector inner (dot)
  v1 . v2 = |v1| * |v2| * cos(a)

> vector outer (wedge)
  v1 ^ v2 = |v1| * |v2| * sin(a) * I
  I is a unit bivector, parallel to v1 and v2

> vector cross (effectively wedge in 3d)
  v1 x v2 = |v1| * |v2| * sin(a) * n
  n is a unit vector, perpendicular to v1 and v2
*/

// ----- ----- ----- ----- -----
//     uint32_t vectors
// ----- ----- ----- ----- -----

struct uvec2 uvec2_add(struct uvec2 v1, struct uvec2 v2) { return (struct uvec2){v1.x + v2.x, v1.y + v2.y}; }
struct uvec2 uvec2_sub(struct uvec2 v1, struct uvec2 v2) { return (struct uvec2){v1.x - v2.x, v1.y - v2.y}; }
struct uvec2 uvec2_mul(struct uvec2 v1, struct uvec2 v2) { return (struct uvec2){v1.x * v2.x, v1.y * v2.y}; }
struct uvec2 uvec2_div(struct uvec2 v1, struct uvec2 v2) { return (struct uvec2){v1.x / v2.x, v1.y / v2.y}; }
uint32_t     uvec2_dot(struct uvec2 v1, struct uvec2 v2) { return v1.x * v2.x + v1.y * v2.y; }

struct uvec3 uvec3_add(struct uvec3 v1, struct uvec3 v2) { return (struct uvec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }
struct uvec3 uvec3_sub(struct uvec3 v1, struct uvec3 v2) { return (struct uvec3){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }
struct uvec3 uvec3_mul(struct uvec3 v1, struct uvec3 v2) { return (struct uvec3){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z}; }
struct uvec3 uvec3_div(struct uvec3 v1, struct uvec3 v2) { return (struct uvec3){v1.x / v2.x, v1.y / v2.y, v1.z / v2.z}; }
uint32_t     uvec3_dot(struct uvec3 v1, struct uvec3 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

struct uvec4 uvec4_add(struct uvec4 v1, struct uvec4 v2) { return (struct uvec4){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w}; }
struct uvec4 uvec4_sub(struct uvec4 v1, struct uvec4 v2) { return (struct uvec4){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w}; }
struct uvec4 uvec4_mul(struct uvec4 v1, struct uvec4 v2) { return (struct uvec4){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w}; }
struct uvec4 uvec4_div(struct uvec4 v1, struct uvec4 v2) { return (struct uvec4){v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w}; }
uint32_t     uvec4_dot(struct uvec4 v1, struct uvec4 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w; }

uint32_t uvec2_cross(struct uvec2 v1, struct uvec2 v2) {
	return v1.x*v2.y - v1.y*v2.x;
}

struct uvec3 uvec3_cross(struct uvec3 v1, struct uvec3 v2) {
	return (struct uvec3){
		v1.y*v2.z - v1.z*v2.y,
		v1.z*v2.x - v1.x*v2.z,
		v1.x*v2.y - v1.y*v2.x,
	};
}

// ----- ----- ----- ----- -----
//     int32_t vectors
// ----- ----- ----- ----- -----

struct svec2 svec2_add(struct svec2 v1, struct svec2 v2) { return (struct svec2){v1.x + v2.x, v1.y + v2.y}; }
struct svec2 svec2_sub(struct svec2 v1, struct svec2 v2) { return (struct svec2){v1.x - v2.x, v1.y - v2.y}; }
struct svec2 svec2_mul(struct svec2 v1, struct svec2 v2) { return (struct svec2){v1.x * v2.x, v1.y * v2.y}; }
struct svec2 svec2_div(struct svec2 v1, struct svec2 v2) { return (struct svec2){v1.x / v2.x, v1.y / v2.y}; }
int32_t      svec2_dot(struct svec2 v1, struct svec2 v2) { return v1.x * v2.x + v1.y * v2.y; }

struct svec3 svec3_add(struct svec3 v1, struct svec3 v2) { return (struct svec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }
struct svec3 svec3_sub(struct svec3 v1, struct svec3 v2) { return (struct svec3){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }
struct svec3 svec3_mul(struct svec3 v1, struct svec3 v2) { return (struct svec3){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z}; }
struct svec3 svec3_div(struct svec3 v1, struct svec3 v2) { return (struct svec3){v1.x / v2.x, v1.y / v2.y, v1.z / v2.z}; }
int32_t      svec3_dot(struct svec3 v1, struct svec3 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

struct svec4 svec4_add(struct svec4 v1, struct svec4 v2) { return (struct svec4){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w}; }
struct svec4 svec4_sub(struct svec4 v1, struct svec4 v2) { return (struct svec4){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w}; }
struct svec4 svec4_mul(struct svec4 v1, struct svec4 v2) { return (struct svec4){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w}; }
struct svec4 svec4_div(struct svec4 v1, struct svec4 v2) { return (struct svec4){v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w}; }
int32_t      svec4_dot(struct svec4 v1, struct svec4 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w; }

int32_t svec2_cross(struct svec2 v1, struct svec2 v2) {
	return v1.x*v2.y - v1.y*v2.x;
}

struct svec3 svec3_cross(struct svec3 v1, struct svec3 v2) {
	return (struct svec3){
		v1.y*v2.z - v1.z*v2.y,
		v1.z*v2.x - v1.x*v2.z,
		v1.x*v2.y - v1.y*v2.x,
	};
}

// ----- ----- ----- ----- -----
//     float vectors
// ----- ----- ----- ----- -----

struct vec2 vec2_add(struct vec2 v1, struct vec2 v2) { return (struct vec2){v1.x + v2.x, v1.y + v2.y}; }
struct vec2 vec2_sub(struct vec2 v1, struct vec2 v2) { return (struct vec2){v1.x - v2.x, v1.y - v2.y}; }
struct vec2 vec2_mul(struct vec2 v1, struct vec2 v2) { return (struct vec2){v1.x * v2.x, v1.y * v2.y}; }
struct vec2 vec2_div(struct vec2 v1, struct vec2 v2) { return (struct vec2){v1.x / v2.x, v1.y / v2.y}; }
float       vec2_dot(struct vec2 v1, struct vec2 v2) { return v1.x * v2.x + v1.y * v2.y; }

struct vec3 vec3_add(struct vec3 v1, struct vec3 v2) { return (struct vec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }
struct vec3 vec3_sub(struct vec3 v1, struct vec3 v2) { return (struct vec3){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }
struct vec3 vec3_mul(struct vec3 v1, struct vec3 v2) { return (struct vec3){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z}; }
struct vec3 vec3_div(struct vec3 v1, struct vec3 v2) { return (struct vec3){v1.x / v2.x, v1.y / v2.y, v1.z / v2.z}; }
float       vec3_dot(struct vec3 v1, struct vec3 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

struct vec4 vec4_add(struct vec4 v1, struct vec4 v2) { return (struct vec4){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w}; }
struct vec4 vec4_sub(struct vec4 v1, struct vec4 v2) { return (struct vec4){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w}; }
struct vec4 vec4_mul(struct vec4 v1, struct vec4 v2) { return (struct vec4){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w}; }
struct vec4 vec4_div(struct vec4 v1, struct vec4 v2) { return (struct vec4){v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w}; }
float       vec4_dot(struct vec4 v1, struct vec4 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w; }

float vec2_cross(struct vec2 v1, struct vec2 v2) {
	return v1.x*v2.y - v1.y*v2.x;
}

struct vec3 vec3_cross(struct vec3 v1, struct vec3 v2) {
	return (struct vec3){
		v1.y*v2.z - v1.z*v2.y,
		v1.z*v2.x - v1.x*v2.z,
		v1.x*v2.y - v1.y*v2.x,
	};
}

struct vec2 vec2_norm(struct vec2 v) {
	float rm = 1 / vec2_dot(v, v);
	return (struct vec2){v.x * rm, v.y * rm};
}

struct vec3 vec3_norm(struct vec3 v) {
	float rm = 1 / vec3_dot(v, v);
	return (struct vec3){v.x * rm, v.y * rm, v.z * rm};
}

struct vec4 vec4_norm(struct vec4 v) {
	float rm = 1 / vec4_dot(v, v);
	return (struct vec4){v.x * rm, v.y * rm, v.z * rm, v.w * rm};
}

// ----- ----- ----- ----- -----
//     quaternions
// ----- ----- ----- ----- -----

/*
--- representations
    vector          + scalar
    x*i + y*j + z*k + w
    sin(angle)*axis + cos(angle)
    e^(axis * angle)

(!) notice, it's `angle` yet, not `angle/2`

--- the general concept used to rotate a vector `V` around an `axis` of rotation by an `angle`
    * split the vector into two parts
      V              = V_perp + V_para
      V_para         = axis * dot(V, axis)
    * rotate parts separately
      V_perp_rotated = sin(angle) * (axis x V_perp) + cos(angle) * V_perp
      V_para_rotated = V_para
    * recombine parts
      V_rotated      = V_perp_rotated + V_para_rotated

--- Rodrigues' rotation formula
    V_rotated = sin(angle) * (axis x V_perp) + cos(angle) * V_perp + V_para
              = sin(angle) * (axis x V)      + cos(angle) * V      + axis * dot(V, axis) * (1 - cos(angle))

--- transform it into the quaternion form
    V_rotated = sin(angle) * (axis x V_perp) + cos(angle) * V_perp + V_para
              = (sin(angle)*axis + cos(angle))  * V_perp  +                                    V_para
              = e^(axis * angle)                * V_perp  +                                    V_para
              = e^(axis * a/2) * e^(axis * a/2) * V_perp  + e^(axis * a/2) * e^(-axis * a/2) * V_para
              = e^(axis * a/2) * V_perp * e^(-axis * a/2) + e^(axis * a/2) * V_para * e^(-axis * a/2)
              = e^(axis * a/2) * (V_para + V_perp) * e^(-axis * a/2)
              = e^(axis * a/2) * V * e^(-axis * a/2)

(!) notice, it's `angle/2` now
    `quat_transform` relies on this format: no need to figure out `V_perp` and `V_para`
    which allows us rotating arbitrary vectors around arbitrary axes by arbitrary angles
*/

struct vec4 quat_axis(struct vec3 axis, float radians) {
	float const r = radians * 0.5f;
	float const s = r32_sin(r);
	float const c = r32_cos(r);
	return (struct vec4){axis.x * s, axis.y * s, axis.z * s, c};
}

struct vec4 quat_radians(struct vec3 radians) {
	struct vec3 const r = vec3_mul(radians, (struct vec3){0.5f, 0.5f, 0.5f});
	struct vec3 const s = (struct vec3){r32_sin(r.x), r32_sin(r.y), r32_sin(r.z)};
	struct vec3 const c = (struct vec3){r32_cos(r.x), r32_cos(r.y), r32_cos(r.z)};
	float const sy_cx = s.y*c.x; float const cy_sx = c.y*s.x;
	float const cy_cx = c.y*c.x; float const sy_sx = s.y*s.x;
	return (struct vec4){
		sy_cx*s.z + cy_sx*c.z,
		sy_cx*c.z - cy_sx*s.z,
		cy_cx*s.z - sy_sx*c.z,
		cy_cx*c.z + sy_sx*s.z,
	};

/*
result = quat_set_axis({0,1,0}, radians.y)
       * quat_set_axis({1,0,0}, radians.x)
       * quat_set_axis({0,0,1}, radians.z)
*/
}

struct vec4 quat_mul(struct vec4 q1, struct vec4 q2) {
	return (struct vec4){
		 q1.x*q2.w + q1.y*q2.z - q1.z*q2.y + q1.w*q2.x,
		-q1.x*q2.z + q1.y*q2.w + q1.z*q2.x + q1.w*q2.y,
		 q1.x*q2.y - q1.y*q2.x + q1.z*q2.w + q1.w*q2.z,
		-q1.x*q2.x - q1.y*q2.y - q1.z*q2.z + q1.w*q2.w,
	};

/*
result = (q1.x*i + q1.y*j + q1.z*k + q1.w)
       * (q2.x*i + q2.y*j + q2.z*k + q2.w)

vector part: w1 * v2 + w2 * v1 + cross(v1, v2)
scalar part: w1      * w2      - dot(v1, v2)
*/
}

struct vec4 quat_conjugate(struct vec4 q) { return (struct vec4){-q.x, -q.y, -q.z, q.w}; }
struct vec4 quat_reciprocal(struct vec4 q) {
	float const rms = 1 / vec4_dot(q, q);
	return (struct vec4){-q.x * rms, -q.y * rms, -q.z * rms, q.w * rms};
}

struct vec3 quat_transform(struct vec4 q, struct vec3 v) {
	float const xx = q.x*q.x; float const yy = q.y*q.y; float const zz = q.z*q.z; float const ww = q.w*q.w;
	float const xy = q.x*q.y; float const yz = q.y*q.z; float const zw = q.z*q.w; float const wx = q.w*q.x;
	float const xz = q.x*q.z; float const yw = q.y*q.w;
	return (struct vec3){
		v.x * ( xx - yy - zz + ww) + (v.y * (xy - zw) + v.z * (yw + xz)) * 2,
		v.y * (-xx + yy - zz + ww) + (v.z * (yz - wx) + v.x * (zw + xy)) * 2,
		v.z * (-xx - yy + ww + zz) + (v.x * (xz - yw) + v.y * (yz + wx)) * 2,
	};

/*
result = q * {vector, 0} * q_reciprocal
*/
}

void quat_get_axes(struct vec4 q, struct vec3 * x, struct vec3 * y, struct vec3 * z) {
	float const xx = q.x*q.x; float const yy = q.y*q.y; float const zz = q.z*q.z; float const ww = q.w*q.w;
	float const xy = q.x*q.y; float const yz = q.y*q.z; float const zw = q.z*q.w; float const wx = q.w*q.x;
	float const xz = q.x*q.z; float const yw = q.y*q.w;
	*x = (struct vec3){(xx - yy - zz + ww), (zw + xy) * 2,        (xz - yw) * 2};
	*y = (struct vec3){(xy - zw) * 2,       (-xx + yy - zz + ww), (yz + wx) * 2};
	*z = (struct vec3){(yw + xz) * 2,       (yz - wx) * 2,        (-xx - yy + ww + zz)};

/*
x = quat_transform(q, {1,0,0})
y = quat_transform(q, {0,1,0})
z = quat_transform(q, {0,0,1})
*/
}

// ----- ----- ----- ----- -----
//     complex
// ----- ----- ----- ----- -----

struct vec2 comp_radians(float radians) {
	return (struct vec2){r32_cos(radians), r32_sin(radians)};
}

// ----- ----- ----- ----- -----
//     matrices
// ----- ----- ----- ----- -----

struct mat4 mat4_transformation(struct vec3 position, struct vec3 scale, struct vec4 rotation) {
	struct vec3 axis_x, axis_y, axis_z;
	quat_get_axes(rotation, &axis_x, &axis_y, &axis_z);
	axis_x = (struct vec3){axis_x.x * scale.x, axis_x.y * scale.x, axis_x.z * scale.x};
	axis_y = (struct vec3){axis_y.x * scale.y, axis_y.y * scale.y, axis_y.z * scale.y};
	axis_z = (struct vec3){axis_z.x * scale.z, axis_z.y * scale.z, axis_z.z * scale.z};
	return (struct mat4){
		{axis_x.x,   axis_x.y,   axis_x.z,   0},
		{axis_y.x,   axis_y.y,   axis_y.z,   0},
		{axis_z.x,   axis_z.y,   axis_z.z,   0},
		{position.x, position.y, position.z, 1},
	};

/*
notice, designated initializer's looks have nothing to do with ordering
- which is a programming concept, not mathematical (concerning multiplication)
- matrix structures have explicit vector components (treat them as you please)

row-major order means interpreting contiguous memory like this
+---------------+
|x_x x_y x_z x_w| -> axis_x
+---------------+
|y_x y_y y_z y_w| -> axis_y
+---------------+
|z_x z_y z_z z_w| -> axis_z
+---------------+
|w_x w_y w_z w_w| -> position
+---------------+

while column-major order means doing it that way around
 axis_x  axis_z
 |       |
+---------------+
|x_x|y_x|z_x|w_x|
|x_y|y_y|z_y|w_y|
|x_z|y_z|z_z|w_z|
|x_w|y_w|z_w|w_w|
+---------------+
     |       |
     axis_y  position
*/
}

struct mat4 mat4_inverse_transformation(struct vec3 position, struct vec3 scale, struct vec4 rotation) {
	struct vec3 axis_x, axis_y, axis_z;
	quat_get_axes(rotation, &axis_x, &axis_y, &axis_z);
	axis_x = (struct vec3){axis_x.x * scale.x, axis_x.y * scale.x, axis_x.z * scale.x};
	axis_y = (struct vec3){axis_y.x * scale.y, axis_y.y * scale.y, axis_y.z * scale.y};
	axis_z = (struct vec3){axis_z.x * scale.z, axis_z.y * scale.z, axis_z.z * scale.z};
	return (struct mat4){
		{axis_x.x, axis_y.x, axis_z.x, 0},
		{axis_x.y, axis_y.y, axis_z.y, 0},
		{axis_x.z, axis_y.z, axis_z.z, 0},
		{
			-vec3_dot(position, axis_x),
			-vec3_dot(position, axis_y),
			-vec3_dot(position, axis_z),
			1
		},
	};
}

struct mat4 mat4_projection(
	struct vec2 scale_xy, struct vec2 offset_xy,
	float view_near, float view_far, float ortho,
	float ndc_near, float ndc_far
) {
	float const reverse_depth = 1 / (view_far - view_near);

	float const persp_scale_z  = r32_isinf(view_far) ? ndc_far                            : (reverse_depth * (ndc_far * view_far - ndc_near * view_near));
	float const persp_offset_z = r32_isinf(view_far) ? ((ndc_near - ndc_far) * view_near) : (reverse_depth * (ndc_near - ndc_far) * view_near * view_far);

	float const ortho_scale_z  = r32_isinf(view_far) ? 0        : (reverse_depth * (ndc_far - ndc_near));
	float const ortho_offset_z = r32_isinf(view_far) ? ndc_near : (reverse_depth * (ndc_near * view_far - ndc_far * view_near));

	float const scale_z  = lerp(persp_scale_z,  ortho_scale_z,  ortho);
	float const offset_z = lerp(persp_offset_z, ortho_offset_z, ortho);
	float const zw = 1 - ortho;
	float const ww = ortho;

	return (struct mat4){
		{scale_xy.x,  0,           0,         0},
		{0,           scale_xy.y,  0,         0},
		{0,           0,           scale_z,  zw},
		{offset_xy.x, offset_xy.y, offset_z, ww},
	};

/*
> inputs
* ortho: [0 .. 1], where 0 is full perspective mode, 1 is full orthographic mode
* NDC: stands for normalized device space, which is the output of a projection

> logic
* XYZ: world-space vector
* XYZ': normalized-space vector
--- orthograhic
    XYZ' = (offset + scale * XYZ) / 1
    XY scale: from [-scale_xy .. scale_xy] to [-1 .. 1]
    Z  scale: from [view_near .. view_far] to [ndc_near .. ndc_far]
--- perspective
    XYZ' = (offset + scale * XYZ) / Z
    XY scale; from [-scale_xy .. scale_xy] to [-1 .. 1]
    Z  scale; from [view_near .. view_far] to [ndc_near .. ndc_far]
*/
}

struct mat4 mat4_invert_transformation(struct mat4 m) {
	struct vec3 const position = (struct vec3){m.w.x, m.w.y, m.w.z};
	return (struct mat4){
		{m.x.x, m.y.x, m.z.x, 0},
		{m.x.y, m.y.y, m.z.y, 0},
		{m.x.z, m.y.z, m.z.z, 0},
		{
			-vec3_dot(position, (struct vec3){m.x.x, m.x.y, m.x.z}),
			-vec3_dot(position, (struct vec3){m.y.x, m.y.y, m.y.z}),
			-vec3_dot(position, (struct vec3){m.z.x, m.z.y, m.z.z}),
			1
		},
	};
}

struct vec4 mat4_mul_vec(struct mat4 m, struct vec4 v) {
	return (struct vec4){
		vec4_dot((struct vec4){m.x.x, m.y.x, m.z.x, m.w.x}, v),
		vec4_dot((struct vec4){m.x.y, m.y.y, m.z.y, m.w.y}, v),
		vec4_dot((struct vec4){m.x.z, m.y.z, m.z.z, m.w.z}, v),
		vec4_dot((struct vec4){m.x.w, m.y.w, m.z.w, m.w.w}, v),
	};

/*
for what it's worth, matrix multiplication is basically doing dot product of left-hand side's rows with right-hand side's columns

notice, arguments order of the function mimics this notation
+---------------+    +---+    +---+
|x_x|y_x|z_x|w_x|    |v_x|    |r_x|
|x_y|y_y|z_y|w_y| \/ |v_y| == |r_y|
|x_z|y_z|z_z|w_z| /\ |v_z| == |r_z|
|x_w|y_w|z_w|w_w|    |v_w|    |r_w|
+---------------+    +---+    +---+

but effectively, `M x V == transpose(transpose(V) x transpose(M))`
                    +---------------+
                    |x_x x_y x_z x_w|
                    +---------------+
+---------------+   |y_x y_y y_z y_w|   +---------------+
|v_x v_y v_z v_w| x +---------------+ = |r_x r_y r_z r_w|
+---------------+   |z_x z_y z_z z_w|   +---------------+
                    +---------------+
                    |w_x w_y w_z w_w|
                    +---------------+
*/
}

struct mat4 mat4_mul_mat(struct mat4 m1, struct mat4 m2) {
	return (struct mat4){
		mat4_mul_vec(m1, m2.x),
		mat4_mul_vec(m1, m2.y),
		mat4_mul_vec(m1, m2.z),
		mat4_mul_vec(m1, m2.w),
	};
}
