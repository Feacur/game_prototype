#include "framework/common.c"
#include "framework/maths.c"
#include "framework/memory.c"
#include "framework/input.c"
#include "framework/unicode.c"
#include "framework/vector_types.c"
#include "framework/logger.c"
#include "framework/parsing.c"

#include "framework/graphics/types.c"
#include "framework/graphics/material.c"

#include "framework/containers/internal.c"
#include "framework/containers/buffer.c"
#include "framework/containers/array_any.c"
#include "framework/containers/array_flt.c"
#include "framework/containers/array_u32.c"
#include "framework/containers/array_s32.c"
#include "framework/containers/array_u64.c"
#include "framework/containers/hash_table_any.c"
#include "framework/containers/hash_table_u32.c"
#include "framework/containers/hash_table_u64.c"
#include "framework/containers/hash_set_u32.c"
#include "framework/containers/hash_set_u64.c"
#include "framework/containers/ref_table.c"
#include "framework/containers/strings.c"

#include "framework/assets/internal/wfobj_scanner.c"
#include "framework/assets/internal/json_scanner.c"
#include "framework/assets/wfobj.c"
#include "framework/assets/json.c"
#include "framework/assets/mesh.c"
#include "framework/assets/image.c"
#include "framework/assets/font.c"
#include "framework/assets/font_atlas.c"

#include "framework/systems/asset_system.c"

#define GAME_GRAPHICS_IS_OPENGL
#if defined(_WIN32) || defined(_WIN64)
	#include "framework/platform_windows/timer.c"
	#include "framework/platform_windows/file.c"
	#include "framework/platform_windows/system.c"
	#include "framework/platform_windows/window.c"
	#include "framework/platform_windows/debug.c"

	#if defined(GAME_GRAPHICS_IS_OPENGL)
		#include "framework/platform_windows/gpu_library_opengl.c"
		#include "framework/gpu_opengl/functions.c"
		#include "framework/gpu_opengl/types.c"
		#include "framework/gpu_opengl/graphics.c"
	#endif
#endif

#include "application/json_read.c"
#include "application/json_read_types.c"
#include "application/json_read_asset.c"
#include "application/application.c"
#include "application/asset_registry.c"
#include "application/components.c"
#include "application/batcher_2d.c"
#include "application/renderer.c"

#include "prototype/object_camera.c"
#include "prototype/object_entity.c"
#include "prototype/game_state.c"
#include "prototype/components.c"
#include "prototype/ui.c"
#include "prototype/main.c"

/*
Language: C99 or C11

Includes:
- ".."
- "../third_party"

Defines:
- _CRT_SECURE_NO_WARNINGS
- STRICT                  (Windows OS)
- VC_EXTRALEAN            (Windows OS)
- WIN32_LEAN_AND_MEAN     (Windows OS)
- NOMINMAX                (Windows OS)
- UNICODE                 (Windows OS)
- _MT                     (MSVC, dynamic CRT)
- _DLL                    (MSVC, dynamic CRT)
- _DEBUG                  (MSVC, debug CRT)
- GAME_TARGET_OPTIMIZED   (this game, optional)
- GAME_TARGET_DEVELOPMENT (this game, optional)
- GAME_TARGET_DEBUG       (this game, optional)

Features:
- disable exceptions
- disable RTTI
- maximum warnings level
- warnings as errors

Libs:
- Windows OS: kernel32.lib, user32.lib, gdi32.lib
- MSVC CRT:
  - dynamic: ucrt.lib, vcruntime.lib, msvcrt.lib
  - static:  libucrt.lib, libvcruntime.lib, libcmt.lib
  - dynamic_debug: ucrtd.lib, vcruntimed.lib, msvcrtd.lib
  - static_debug:  libucrtd.lib, libvcruntimed.lib, libcmtd.lib

Disable Clang warnings:
- switch-enum                     (allow partial list of enum cases in switch statements)
- float-equal                     (allow exact floating point values comparison)
- reserved-id-macro               (source: third-party libs; `GL`, `KHR`; is easy to fix)
- nonportable-system-include-path (source: third-party libs; `GL`; is easy to fix)
- assign-enum                     (allow enum values have implicit flags combinations)
- bad-function-cast               (allow casting function results without temporary assignment)
- documentation-unknown-command   (source: clangd; `@note`; doesn't affect compilation)
- declaration-after-statement     (allow mixing code and variables declarations)

Disable MSVC warnings:
- 5105, macro expansion producing 'defined' has undefined behavior (source: OS; `winbase.h`; is unfixable)
*/
