//
#include "maths_types.h"

// @note: MSVC issues `C2099: initializer is not a constant`
//        when a designated initializer type is specified

// ----- ----- ----- ----- -----
//     constants
// ----- ----- ----- ----- -----

struct vec2 const c_comp_identity = {1,0};
struct vec4 const c_quat_identity = {0,0,0,1};

struct mat2 const c_mat2_identity = {
	{1,0},
	{0,1},
};

struct mat3 const c_mat3_identity = {
	{1,0,0},
	{0,1,0},
	{0,0,1},
};

struct mat4 const c_mat4_identity = {
	{1,0,0,0},
	{0,1,0,0},
	{0,0,1,0},
	{0,0,0,1},
};
