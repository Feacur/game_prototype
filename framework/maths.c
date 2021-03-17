#include <math.h>

//
#include "maths.h"

float convert_bits_u32_r32(uint32_t value) {
	union {
		uint32_t value_u32;
		float value_r32;
	} data;
	data.value_u32 = value;
	return data.value_r32;
}

uint32_t convert_bits_r32_u32(float value) {
	union {
		uint32_t value_u32;
		float value_r32;
	} data;
	data.value_r32 = value;
	return data.value_u32;
}

uint32_t hash_bytes_fnv1(uint8_t const * value, uint32_t length) {
	uint32_t hash = 0x811c9dc5;
	for (uint32_t i = 0; i < length; i++) {
		hash ^= value[i];
		hash *= 0x01000193;
	}
	return hash;
}

uint32_t hash_u32_xorshift(uint32_t value) {
	value ^= value << 13;
	value ^= value >> 17;
	value ^= value << 15;
	return value;
}

uint32_t mul_div_u32(uint32_t value, uint32_t numerator, uint32_t denominator) {
	uint32_t a = value / denominator;
	uint32_t b = value % denominator;
	return a * numerator + b * numerator / denominator;
}

uint64_t mul_div_u64(uint64_t value, uint64_t numerator, uint64_t denominator) {
	uint64_t a = value / denominator;
	uint64_t b = value % denominator;
	return a * numerator + b * numerator / denominator;
}

float lerp(float v1, float v2, float t) { return v1*(1 - t) + v2*t; }

float min_r32(float v1, float v2) { return (v1 < v2) ? v1 : v2; }
float max_r32(float v1, float v2) { return (v1 > v2) ? v1 : v2; }
float clamp_r32(float v, float low, float high) { return min_r32(max_r32(v, low), high); }

/* -- vectors

i = jk = -kj
j = ki = -ik
k = ij = -ji

> vectors cross product
ii = jj = kk = ijk =  0 == sin(0)

> vectors dot product
ii = jj = kk = ijk =  1 == cos(0)

> quaternions multiplication
ii = jj = kk = ijk = -1 == cos(90 + 90)

*/

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

struct vec3 vec3_cross(struct vec3 v1, struct vec3 v2) {
	return (struct vec3){
		v1.y*v2.z - v1.z*v2.y,
		v1.z*v2.x - v1.x*v2.z,
		v1.x*v2.y - v1.y*v2.x,
	};
}

/* -- quaternions

--- representations
    vector          + scalar
    x*i + y*j + z*k + w
    sin(angle)*axis + cos(angle)
    e^(axis * angle)

(!) make notice, it's `angle`, not `angle/2`

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
              = (sin(angle)*axis + cos(angle)) * V_perp + V_para
              = e^(axis * angle) * V_perp + V_para
              = e^(axis * a/2) * e^(-axis * a/2) * V_para + e^(axis * a/2) * e^(axis * a/2) * V_perp
              = e^(axis * a/2) * V_para * e^(-axis * a/2) + e^(axis * a/2) * V_perp * e^(axis * a/2)
              = e^(axis * a/2) * (V_para + V_perp) * e^(-axis * a/2)
              = e^(axis * a/2) * V * e^(-axis * a/2)

(!) make notice, it's `angle/2` now
    `quat_transform` relies on this format: no need to figure out `V_perp` and `V_para`
    which allows us rotating arbitrary vectors around arbitrary axes by arbitrary angles
*/

struct vec4 quat_set_axis(struct vec3 axis, float radians) {
	float const r = radians * 0.5f;
	float const s = sinf(r);
	float const c = cosf(r);
	return (struct vec4){axis.x * s, axis.y * s, axis.z * s, c};
}

struct vec4 quat_set_radians(struct vec3 radians) {
	struct vec3 const r = vec3_mul(radians, (struct vec3){0.5f, 0.5f, 0.5f});
	struct vec3 const s = (struct vec3){sinf(r.x), sinf(r.y), sinf(r.z)};
	struct vec3 const c = (struct vec3){cosf(r.x), cosf(r.y), cosf(r.z)};
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
*/
}

struct vec4 quat_conjugate(struct vec4 q) { return (struct vec4){-q.x, -q.y, -q.z, q.w}; }
struct vec4 quat_reciprocal(struct vec4 q) {
	float const ms = vec4_dot(q, q);
	return (struct vec4){-q.x/ms, -q.y/ms, -q.z/ms, q.w/ms};
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

/* -- matrices
*/

struct mat4 mat4_set_transformation(struct vec3 position, struct vec3 scale, struct vec4 rotation) {
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
}

struct mat4 mat4_set_inverse_transformation(struct vec3 position, struct vec3 scale, struct vec4 rotation) {
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

struct mat4 mat4_set_projection(struct vec2 scale_xy, float ncp, float fcp, float ortho) {
	float const NS_NCP = 0, NS_FCP = 1;
	float const reverse_depth = 1 / (fcp - ncp);

	float const persp_scale_z  = isinf(fcp) ? 1                         : (reverse_depth * (NS_FCP * fcp - NS_NCP * ncp));
	float const persp_offset_z = isinf(fcp) ? ((NS_NCP - NS_FCP) * ncp) : (reverse_depth * (NS_NCP - NS_FCP) * ncp * fcp);

	float const ortho_scale_z  = isinf(fcp) ? 0      : (reverse_depth * (NS_FCP - NS_NCP));
	float const ortho_offset_z = isinf(fcp) ? NS_NCP : (reverse_depth * (NS_NCP * fcp - NS_FCP * ncp));

	float const scale_z  = lerp(persp_scale_z, ortho_scale_z, ortho);
	float const offset_z = lerp(persp_offset_z, ortho_offset_z, ortho);
	float const zw = 1 - ortho;
	float const ww = ortho;

	return (struct mat4){
		{scale_xy.x, 0,          0,        0},
		{0,          scale_xy.y, 0,        0},
		{0,          0,          scale_z,  zw},
		{0,          0,          offset_z, ww},
	};

/*
> inputs
* ncp: world-space near clipping plane
* fcp: world-space far clipping plane
* ortho: [0 .. 1], where 0 is perspective mode, 1 is orthographic mode

> settings
* NS_NCP: normalized-space near clipping plane
* NS_FCP: normalized-space far clipping plane

> logic
* XYZ: world space vector
* XYZ': normalized space vector
--- orthograhic
    XYZ' = (offset + scale * XYZ) / 1
    XY scale: from [-scale_xy .. scale_xy] to [-1 .. 1]
    Z  scale: from [ncp .. fcp]            to [NS_NCP .. NS_FCP]
--- perspective
    XYZ' = (offset + scale * XYZ) / Z
    XY scale; from [-scale_xy .. scale_xy] to [-1 .. 1]
    Z  scale; from [ncp .. fcp]            to [NS_NCP .. NS_FCP]
*/
}

struct mat4 mat4_inverse_transformation(struct mat4 m) {
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
}

struct mat4 mat4_mul_mat(struct mat4 m1, struct mat4 m2) {
	return (struct mat4){
		mat4_mul_vec(m1, m2.x),
		mat4_mul_vec(m1, m2.y),
		mat4_mul_vec(m1, m2.z),
		mat4_mul_vec(m1, m2.w),
	};
}
