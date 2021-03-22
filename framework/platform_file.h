#if !defined(GAME_PLATFORM_FILE)
#define GAME_PLATFORM_FILE

#include "common.h"

struct Array_Byte;

bool platform_file_read(struct Array_Byte * buffer, char const * path);

uint64_t platform_file_size(char const * path);
uint64_t platform_file_time(char const * path);

#endif
