#if defined(FRAMEWORK_WARNINGS_PUSH)
#undef FRAMEWORK_WARNINGS_PUSH

#if defined(__clang__)
	#pragma clang diagnostic pop
#elif defined(_MSC_VER)
	#pragma warning(pop)
#endif

#else
	#error "warnings were not pushed"
#endif
