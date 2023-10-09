#if !defined(FRAMEWORK_COMPILATION)
#define FRAMEWORK_COMPILATION

// ----- ----- ----- ----- -----
//     saner C99/C11
// ----- ----- ----- ----- -----

#if defined(__clang__)
	// https://clang.llvm.org/docs/DiagnosticsReference.html
	#pragma clang diagnostic ignored "-Wdeclaration-after-statement" // mixing declarations and code ... C99
	#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"         // unsafe pointer / unsafe buffer
	#pragma clang diagnostic ignored "-Wbad-function-cast"           // cast from function call of type A to non-matching type B
	#pragma clang diagnostic ignored "-Wswitch-enum"                 // enumeration value ... not explicitly handled in switch
	#pragma clang diagnostic ignored "-Wassign-enum"                 // integer constant not in range of enumerated type A
	#pragma clang diagnostic ignored "-Wfloat-equal"                 // comparing floating point with == or != is unsafe
#elif defined(_MSC_VER)
	// https://learn.microsoft.com/cpp/error-messages/compiler-warnings/compiler-warnings-c4000-c5999
	#pragma warning(disable : 4200) // nonstandard extension used : zero-sized array in struct/union
#endif

#endif
