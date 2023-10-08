#if !defined(FRAMEWORK_COMPILATION)
#define FRAMEWORK_COMPILATION

#if defined(__clang__)
	#pragma clang diagnostic ignored "-Wswitch-enum" // allow partial list of enum cases in switch statements
	#pragma clang diagnostic ignored "-Wfloat-equal" // allow exact floating point values comparison
	#pragma clang diagnostic ignored "-Wassign-enum" // allow enum values have implicit flags combinations
	#pragma clang diagnostic ignored "-Wbad-function-cast" // allow casting function results without temporary assignment
	#pragma clang diagnostic ignored "-Wdeclaration-after-statement" // allow mixing code and variables declarations
	#pragma clang diagnostic ignored "-Wunsafe-buffer-usage" // allow unsafe pointer arithmetic
#elif defined(_MSC_VER)
	#pragma warning(disable : 5105) // macro expansion producing 'defined' has undefined behavior
	#pragma warning(disable : 4200) // nonstandard extension used : zero-sized array in struct/union
#endif

#endif
