// unity build
#include "framework/common.c"
#include "framework/maths.c"
#include "framework/input_keys.c"
#include "framework/input.c"
#include "framework/unicode.c"
#include "framework/maths_types.c"
#include "framework/formatter.c"
#include "framework/parsing.c"
#include "framework/json_read.c"




#include "framework/platform/allocator.c"
#define GAME_GRAPHICS_IS_OPENGL
#if defined(_WIN32) || defined(_WIN64)
	#include "framework/platform/windows/timer.c"
	#include "framework/platform/windows/file.c"
	#include "framework/platform/windows/system.c"
	#include "framework/platform/windows/window.c"
	#include "framework/platform/windows/debug.c"

	#if defined(GAME_GRAPHICS_IS_OPENGL)
		#include "framework/platform/windows/gpu_library_opengl.c"
		#include "framework/graphics/opengl/functions.c"
		#include "framework/graphics/opengl/gpu_types.c"
		#include "framework/graphics/opengl/graphics.c"
	#endif
#endif




#include "framework/containers/internal/helpers.c"
#include "framework/containers/array.c"
#include "framework/containers/buffer.c"
#include "framework/containers/hashmap.c"
#include "framework/containers/sparseset.c"

#include "framework/systems/memory_system.c"
#include "framework/systems/arena_system.c"
#include "framework/systems/string_system.c"
#include "framework/systems/defer_system.c"
#include "framework/systems/material_system.c"
#include "framework/systems/asset_system.c"

#include "framework/graphics/gfx_types.c"
#include "framework/graphics/gfx_objects.c"
#include "framework/graphics/gfx_material.c"

#include "framework/assets/internal/wfobj_lexer.c"
#include "framework/assets/internal/json_lexer.c"
#include "framework/assets/internal/wfobj.c"
#include "framework/assets/json.c"
#include "framework/assets/mesh.c"
#include "framework/assets/image.c"
#include "framework/assets/typeface.c"
#include "framework/assets/font.c"

#include "application/json_load.c"
#include "application/application.c"
#include "application/asset_types.c"
#include "application/app_components.c"
#include "application/batcher_2d.c"
#include "application/renderer.c"

#include "prototype/object_camera.c"
#include "prototype/object_entity.c"
#include "prototype/game_state.c"
#include "prototype/proto_components.c"
#include "prototype/ui.c"

#if !defined(GAME_ARCH_SHARED)
	#include "prototype/main.c"
#endif
