#if !defined(FRAMEWORK_CONTAINERS_sparseset)
#define FRAMEWORK_CONTAINERS_sparseset

#include "handle.h"
#include "array.h"

struct Sparseset_Iterator {
	uint32_t current, next;
	struct Handle handle;
	void * value;
};

struct Sparseset {
	struct Array packed; // of value_size
	struct Array sparse; // `struct Handle`
	uint32_t * ids;      // into `sparse`
	uint32_t free_list;  // into `sparse`
};

struct Sparseset sparseset_init(uint32_t value_size);
void sparseset_free(struct Sparseset * sparseset);

void sparseset_clear(struct Sparseset * sparseset);
void sparseset_resize(struct Sparseset * sparseset, uint32_t target_capacity);

struct Handle sparseset_aquire(struct Sparseset * sparseset, void const * value);
void sparseset_discard(struct Sparseset * sparseset, struct Handle handle);

void * sparseset_get(struct Sparseset * sparseset, struct Handle handle);
void sparseset_set(struct Sparseset * sparseset, struct Handle handle, void const * value);

bool sparseset_iterate(struct Sparseset const * sparseset, struct Sparseset_Iterator * iterator);

#define FOR_SPARSESET(data, it) for ( \
	struct Sparseset_Iterator it = {0}; \
	sparseset_iterate(data, &it); \
) \

#endif
