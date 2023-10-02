// unity build
#include "framework/common.c"
#include "framework/maths.c"
#include "framework/memory.c"
#include "framework/input.c"
#include "framework/unicode.c"
#include "framework/maths_types.c"
#include "framework/logger.c"
#include "framework/parsing.c"
#include "framework/json_read.c"

#include "framework/graphics/types.c"
#include "framework/graphics/material.c"

#include "framework/containers/internal/helpers.c"
#include "framework/containers/array.c"
#include "framework/containers/buffer.c"
#include "framework/containers/hashmap.c"
#include "framework/containers/hashset.c"
#include "framework/containers/sparseset.c"
#include "framework/containers/strings.c"

#include "framework/assets/internal/wfobj_lexer.c"
#include "framework/assets/internal/json_lexer.c"
#include "framework/assets/internal/wfobj.c"
#include "framework/assets/json.c"
#include "framework/assets/mesh.c"
#include "framework/assets/image.c"
#include "framework/assets/typeface.c"
#include "framework/assets/font.c"

#include "framework/systems/string_system.c"
#include "framework/systems/material_system.c"
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

#include "application/json_load.c"
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
- Windows OS
  - always:  STRICT, VC_EXTRALEAN, WIN32_LEAN_AND_MEAN, NOMINMAX
  - unicode: UNICODE, _UNICODE
- MSVC CRT
  - always:  _CRT_SECURE_NO_WARNINGS
  - dynamic: _MT, _DLL
  - debug:   _DEBUG
- this game %configuration%
  - GAME_TARGET_RELEASE
  - GAME_TARGET_DEVELOPMENT
  - GAME_TARGET_DEBUG
- this game %arch_mode%
  - DGAME_ARCH_SHARED
  - DGAME_ARCH_CONSOLE
  - DGAME_ARCH_WINDOWS

Features:
- disable exceptions
- disable RTTI
- maximum warnings level
- warnings as errors

Libs:
- Windows OS: kernel32.lib, user32.lib, gdi32.lib
  - release: -
  - else:    dbghelp.lib
- MSVC CRT:
  - dynamic:       ucrt.lib,     vcruntime.lib,     msvcrt.lib
  - static:        libucrt.lib,  libvcruntime.lib,  libcmt.lib
  - dynamic_debug: ucrtd.lib,    vcruntimed.lib,    msvcrtd.lib
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
- unsafe-buffer-usage             (allow unsafe pointer arithmetic)

Disable MSVC warnings:
- 5105, macro expansion producing 'defined' has undefined behavior (source: OS; `winbase.h`; is unfixable)
*/
