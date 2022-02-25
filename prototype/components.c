//
#include "components.h"

// @note: MSVC issues `C2099: initializer is not a constant`
//        when a designated initializer type is specified

struct Transform_3D const c_transform_3d_default = {
	.rotation = {0, 0, 0, 1},
	.scale = {1, 1, 1},
};

struct Transform_2D const c_transform_2d_default = {
	.rotation = {1, 0},
	.scale = {1, 1},
};
struct Transform_Rect const c_transform_rect_default = {
	.max_relative = {1, 1},
};
