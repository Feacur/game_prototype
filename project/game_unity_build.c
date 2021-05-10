#include "framework/maths.c"
#include "framework/memory.c"
#include "framework/input.c"
#include "framework/unicode.c"
#include "framework/vector_types.c"
#include "framework/logger.c"

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

/*
Language: C99 or C11

Includes:
- ".."
- "../third_party"

Defines:
- _CRT_SECURE_NO_WARNINGS
- WIN32_LEAN_AND_MEAN     (Windows OS)
- NOMINMAX                (Windows OS)
- UNICODE                 (Windows OS)
- GAME_TARGET_OPTIMIZED   (this game, optional)
- GAME_TARGET_DEVELOPMENT (this game, optional)
- GAME_TARGET_DEBUG       (this game, optional)
- _MT                     (MSVC, dynamic CRT)
- _DLL                    (MSVC, dynamic CRT)
- _DEBUG                  (MSVC, debug CRT)

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
- assign-enum                     (allow enum values have implicit flags cominations)
- bad-function-cast               (allow casting function results without temporary assignment)
- documentation-unknown-command   (source: clangd; `@note`; doesn't affect compilation)

Disable MSVC warnings:
- 5105, macro expansion producing 'defined' has undefined behavior (source: OS; `winbase.h`; is unfixable)
*/
