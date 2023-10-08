#if !defined(FRAMEWORK_WARNINGS_PUSH)
#define FRAMEWORK_WARNINGS_PUSH

#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Weverything"
#elif defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#else
	#error "warnings were pushed already"
#endif
