#if !defined(GAME_CONTAINERS_STRINGS)
#define GAME_CONTAINERS_STRINGS

#include "framework/containers/array_byte.h"
#include "framework/containers/array_u32.h"

// @note: strings interning structure
struct Strings { // ZII
	struct Array_U32 offsets;
	struct Array_U32 lengths;
	struct Array_Byte buffer;
};

void strings_init(struct Strings * strings);
void strings_free(struct Strings * strings);

uint32_t strings_find(struct Strings const * strings, struct CString value);
uint32_t strings_add(struct Strings * strings, struct CString value);
struct CString strings_get(struct Strings const * strings, uint32_t id);

#endif
