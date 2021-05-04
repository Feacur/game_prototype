#include "framework/maths.c"
#include "framework/memory.c"
#include "framework/input.c"
#include "framework/unicode.c"
#include "framework/vector_types.c"

#include "framework/graphics/types.c"
#include "framework/graphics/material.c"
#include "framework/graphics/font_image.c"

#include "framework/containers/internal.c"
#include "framework/containers/array_any.c"
#include "framework/containers/array_byte.c"
#include "framework/containers/array_float.c"
#include "framework/containers/array_u32.c"
#include "framework/containers/array_s32.c"
#include "framework/containers/array_u64.c"
#include "framework/containers/hash_table_any.c"
#include "framework/containers/hash_table_u32.c"
#include "framework/containers/hash_table_u64.c"
#include "framework/containers/ref_table.c"
#include "framework/containers/strings.c"

#include "framework/assets/parsing.c"
#include "framework/assets/mesh_obj_scanner.c"
#include "framework/assets/mesh_obj.c"
#include "framework/assets/mesh.c"
#include "framework/assets/image.c"
#include "framework/assets/font.c"

#include "framework/systems/asset_system.c"

#define GAME_GRAPHICS_IS_OPENGL
#if defined(_WIN32) || defined(_WIN64)
	#include "framework/windows/timer.c"
	#include "framework/windows/file.c"
	#include "framework/windows/system.c"
	#include "framework/windows/window.c"

	#if defined(GAME_GRAPHICS_IS_OPENGL)
		#include "framework/windows/opengl.c"
		#include "framework/opengl/functions.c"
		#include "framework/opengl/graphics.c"
	#endif
#endif

#include "application/application.c"

#include "prototype/batcher_2d.c"
#include "prototype/asset_types.c"
#include "prototype/main_sandbox.c"
