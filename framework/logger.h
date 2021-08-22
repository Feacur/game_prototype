#if !defined(GAME_FRAMEWORK_LOGGER)
#define GAME_FRAMEWORK_LOGGER

#if defined(__clang__) // clang: argument 1 of 2 is a printf-like format literal
__attribute__((format(printf, 1, 2)))
#endif // __clang__
void logger_to_console(char const * format, ...);

#if defined(__clang__) // clang: argument 2 of 3 is a printf-like format literal
__attribute__((format(printf, 2, 3)))
#endif // __clang__
int logger_to_buffer(char * buffer, char const * format, ...);

#endif
