#if !defined(GAME_STRINGS)
#define GAME_STRINGS

#include "common.h"

struct Strings;

struct Strings * strings_init(void);
void strings_free(struct Strings * strings);

uint32_t strings_find(struct Strings * strings, uint32_t length, char const * value);
uint32_t strings_add(struct Strings * strings, uint32_t length, char const * value);
char const * strings_get(struct Strings * strings, uint32_t id);

#endif
