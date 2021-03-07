#if !defined(GAME_MATHS)
#define GAME_MATHS

#include "vectors.h"

uint64_t mul_div_u64(uint64_t value, uint64_t numerator, uint64_t denominator);

float lerp(float v1, float v2, float t);

float min_r32(float v1, float v2);
float max_r32(float v1, float v2);
float clamp_r32(float v, float low, float high);

// -- vectors
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

// -- quaternions
struct vec4 quat_set_axis(struct vec3 axis, float radians);
struct vec4 quat_set_radians(struct vec3 radians);
struct vec4 quat_mul(struct vec4 q1, struct vec4 q2);

struct vec4 quat_conjugate(struct vec4 q);
struct vec4 quat_reciprocal(struct vec4 q);
struct vec3 quat_transform(struct vec4 q, struct vec3 v);
void        quat_get_axes(struct vec4 q, struct vec3 * x, struct vec3 * y, struct vec3 * z);

// -- matrices
struct mat4 mat4_set_transformation(struct vec3 position, struct vec3 scale, struct vec4 rotation);
struct mat4 mat4_set_projection(struct vec2 scale_xy, float ncp, float fcp, float ortho);

struct mat4 mat4_inverse_transformation(struct mat4 m);
struct vec4 mat4_mul_vec(struct mat4 m, struct vec4 v);
struct mat4 mat4_mul_mat(struct mat4 m1, struct mat4 m2);

#endif
