#if !defined(FRAMEWORK_COMPILATION)
#define FRAMEWORK_COMPILATION

// ----- ----- ----- ----- -----
//     windows
// ----- ----- ----- ----- -----

#include "__warnings_push.h"
	// @note: explicitly disregard -W API and rely on UTF-8 via -A API and `activeCodePage` manifest;
	//        also CP_UTF8 for console is being set. it's trivial to restore the compatability code
	//        via the `MultiByteToWideChar` and following defines; but for now I can stop caring.
	#undef UNICODE
	#undef _UNICODE
	#undef DBGHELP_TRANSLATE_TCHAR
#include "__warnings_pop.h"

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
#endif

#endif
