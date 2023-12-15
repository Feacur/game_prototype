#if !defined(FRAMEWORK_CONTAINERS_sparseset)
#define FRAMEWORK_CONTAINERS_sparseset

#include "array.h"

struct Sparseset_Iterator {
	uint32_t curr, next;
	void * value;
	struct Handle handle;
};

struct Sparseset {
	struct Array packed; // of value_size
	struct Array sparse; // `struct Handle`
	struct Array ids;    // `uint32_t` into `sparse`
	uint32_t free_list;  // into `sparse`
};

struct Sparseset sparseset_init(uint32_t value_size);
void sparseset_free(struct Sparseset * sparseset);

void sparseset_clear(struct Sparseset * sparseset, bool deallocate);
void sparseset_ensure(struct Sparseset * sparseset, uint32_t capacity);

struct Handle sparseset_aquire(struct Sparseset * sparseset, void const * value);
void sparseset_discard(struct Sparseset * sparseset, struct Handle handle);

void * sparseset_get(struct Sparseset const * sparseset, struct Handle handle);
void sparseset_set(struct Sparseset * sparseset, struct Handle handle, void const * value);

uint32_t sparseset_get_count(struct Sparseset const * sparseset);
void * sparseset_get_at(struct Sparseset const * sparseset, uint32_t index);

void * sparseset_get_at_unsafe(struct Sparseset const * sparseset, uint32_t index);

bool sparseset_iterate(struct Sparseset const * sparseset, struct Sparseset_Iterator * iterator);

#define FOR_SPARSESET(data, it) for ( \
	struct Sparseset_Iterator it = {0}; \
	sparseset_iterate(data, &it); \
) \

#endif
