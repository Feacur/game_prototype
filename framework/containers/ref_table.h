#if !defined(GAME_CONTAINTERS_REF_TABLE)
#define GAME_CONTAINTERS_REF_TABLE

#include "ref.h"

struct Ref_Table_Entry {
	uint32_t next;
	struct Ref ref;
	void * value;
};

// @todo: provide "0 id is empty" guarantee?
struct Ref_Table {
	uint32_t free_sparse_index;
	uint32_t value_size;
	uint32_t capacity, count;
	struct Ref * sparse;
	uint32_t * dense;
	uint8_t * values;
};

void ref_table_init(struct Ref_Table * ref_table, uint32_t value_size);
void ref_table_free(struct Ref_Table * ref_table);

void ref_table_clear(struct Ref_Table * ref_table);
void ref_table_resize(struct Ref_Table * ref_table, uint32_t target_capacity);

struct Ref ref_table_aquire(struct Ref_Table * ref_table, void const * value);
void ref_table_discard(struct Ref_Table * ref_table, struct Ref ref);

void * ref_table_get(struct Ref_Table * ref_table, struct Ref ref);
void ref_table_set(struct Ref_Table * ref_table, struct Ref ref, void const * value);

bool ref_table_exists(struct Ref_Table * ref_table, struct Ref ref);

uint32_t ref_table_get_count(struct Ref_Table * ref_table);
struct Ref ref_table_ref_at(struct Ref_Table * ref_table, uint32_t index);
void * ref_table_value_at(struct Ref_Table * ref_table, uint32_t index);

bool ref_table_iterate(struct Ref_Table * ref_table, struct Ref_Table_Entry * entry);

#endif
