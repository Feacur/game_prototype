#if !defined(FRAMEWORK_CONTAINERS_HANDLE_TABLE)
#define FRAMEWORK_CONTAINERS_HANDLE_TABLE

#include "handle.h"
#include "array.h"

struct Handle_Table_Iterator {
	uint32_t current, next;
	struct Handle handle;
	void * value;
};

struct Handle_Table {
	struct Array packed; // of value_size
	struct Array sparse; // `struct Handle`
	uint32_t * ids;      // into `sparse`
	uint32_t free_list;  // into `sparse`
};

struct Handle_Table handle_table_init(uint32_t value_size);
void handle_table_free(struct Handle_Table * handle_table);

void handle_table_clear(struct Handle_Table * handle_table);
void handle_table_resize(struct Handle_Table * handle_table, uint32_t target_capacity);

struct Handle handle_table_aquire(struct Handle_Table * handle_table, void const * value);
void handle_table_discard(struct Handle_Table * handle_table, struct Handle handle);

void * handle_table_get(struct Handle_Table * handle_table, struct Handle handle);
void handle_table_set(struct Handle_Table * handle_table, struct Handle handle, void const * value);

bool handle_table_iterate(struct Handle_Table const * handle_table, struct Handle_Table_Iterator * iterator);

#define FOR_HANDLE_TABLE(data, it) for ( \
	struct Handle_Table_Iterator it = {0}; \
	handle_table_iterate(data, &it); \
) \

#endif
