#if !defined(GAME_PLATFORM_LOGGER)
#define GAME_PLATFORM_LOGGER

#if defined(__clang__) // clang: argument 1 of 2 is a printf-like format literal
__attribute__((format(printf, 1, 2)))
#endif // __clang__
void logger_to_console(char const * format, ...);

#endif
