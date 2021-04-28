#if !defined(GAME_CONTAINTERS_STRINGS)
#define GAME_CONTAINTERS_STRINGS

#include "framework/containers/array_byte.h"
#include "framework/containers/array_u32.h"

struct Strings {
	struct Array_U32 offsets;
	struct Array_U32 lengths;
	struct Array_Byte buffer;
};

void strings_init(struct Strings * strings);
void strings_free(struct Strings * strings);

uint32_t strings_find(struct Strings * strings, uint32_t length, char const * value);
uint32_t strings_add(struct Strings * strings, uint32_t length, char const * value);
char const * strings_get(struct Strings * strings, uint32_t id);

#endif
