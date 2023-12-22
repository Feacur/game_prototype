#if !defined(FRAMEWORK_COMPILATION)
#define FRAMEWORK_COMPILATION

/*
Language: C99 or C11

Includes:
- ".."
- "../third_party"

Defines:
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
*/

// ----- ----- ----- ----- -----
//     saner C99/C11
// ----- ----- ----- ----- -----

#if defined(__clang__)
	// https://clang.llvm.org/docs/DiagnosticsReference.html
	#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments" // error: token pasting of ',' and __VA_ARGS__ is a GNU extension
	#pragma clang diagnostic ignored "-Wdeclaration-after-statement"       // mixing declarations and code ... C99
	#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"               // unsafe pointer / unsafe buffer
	#pragma clang diagnostic ignored "-Wbad-function-cast"                 // cast from function call of type A to non-matching type B
	#pragma clang diagnostic ignored "-Wswitch-enum"                       // enumeration value ... not explicitly handled in switch
	#pragma clang diagnostic ignored "-Wassign-enum"                       // integer constant not in range of enumerated type A
	#pragma clang diagnostic ignored "-Wfloat-equal"                       // comparing floating point with == or != is unsafe

	// zig cc
	#pragma clang diagnostic ignored "-Wpadded"
	#pragma clang diagnostic ignored "-Walloca"
	#pragma clang diagnostic ignored "-Wnonportable-system-include-path"
#elif defined(_MSC_VER)
	// https://learn.microsoft.com/cpp/error-messages/compiler-warnings/compiler-warnings-c4000-c5999
	#pragma warning(disable : 4200) // nonstandard extension used : zero-sized array in struct/union

	#if !defined(_CRT_SECURE_NO_WARNINGS)
		#define _CRT_SECURE_NO_WARNINGS
	#endif

	#include "framework/__warnings_push.h"
		#undef _UNICODE
	#include "framework/__warnings_pop.h"
#endif

// ----- ----- ----- ----- -----
//     windows
// ----- ----- ----- ----- -----

#if defined (_WIN32)
	/*
	Libs:
	- dynamic:       ucrt.lib,     vcruntime.lib,     msvcrt.lib
	- static:        libucrt.lib,  libvcruntime.lib,  libcmt.lib
	- dynamic_debug: ucrtd.lib,    vcruntimed.lib,    msvcrtd.lib
	- static_debug:  libucrtd.lib, libvcruntimed.lib, libcmtd.lib
	*/
#endif

#endif
