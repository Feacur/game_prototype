//
#include "vector_types.h"

// @note: MSVC issues `C2099: initializer is not a constant`
//        when a designated initializer type is specified

struct vec4 const quat_identity = {0,0,0,1};

struct mat2 const mat2_identity = {
	{1,0},
	{0,1},
};

struct mat3 const mat3_identity = {
	{1,0,0},
	{0,1,0},
	{0,0,1},
};

struct mat4 const mat4_identity = {
	{1,0,0,0},
	{0,1,0,0},
	{0,0,1,0},
	{0,0,0,1},
};
