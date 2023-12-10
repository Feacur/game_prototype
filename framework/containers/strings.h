#if !defined(FRAMEWORK_CONTAINERS_STRINGS)
#define FRAMEWORK_CONTAINERS_STRINGS

#include "framework/containers/buffer.h"
#include "framework/containers/array.h"

// @purpose: strings interning structure

// @note: `0` is a NULL id
struct Strings {
	struct Array offsets; // uint32_t
	struct Array lengths; // uint32_t
	struct Buffer buffer;
};

struct Strings strings_init(void);
void strings_free(struct Strings * strings);

void strings_clear(struct Strings * strings, bool deallocate);

uint32_t strings_find(struct Strings const * strings, struct CString value);
uint32_t strings_add(struct Strings * strings, struct CString value);
struct CString strings_get(struct Strings const * strings, uint32_t id);

#endif
