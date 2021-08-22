//
#include "components.h"

// @note: MSVC issues `C2099: initializer is not a constant`
//        when a designated initializer type is specified

struct Transform_3D const transform_3d_default = {
	.scale = {1, 1, 1},
	.rotation = {0, 0, 0, 1},
};

struct Transform_2D const transform_2d_default = {
	.scale = {1, 1},
	.rotation = {1, 0},
};
struct Transform_Rect const transform_rect_default = {
	.max_relative = {1, 1},
};
