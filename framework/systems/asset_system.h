#if !defined(GAME_FRAMEWORK_ASSET_SYSTEM)
#define GAME_FRAMEWORK_ASSET_SYSTEM

#include "framework/containers/strings.h"
#include "framework/containers/hash_table_u32.h"
#include "framework/containers/ref_table.h"

struct Asset_Ref {
	struct Ref instance_ref;
	uint32_t type_id, resource_id;
};

struct Asset_Callbacks {
	void (* init)(void * instance, char const * name);
	void (* free)(void * instance);
};

struct Asset_System {
	struct Strings strings;
	struct Hash_Table_U32 types;
	struct Hash_Table_U32 refs;
	struct Hash_Table_U32 map;
};

void asset_system_init(struct Asset_System * system);
void asset_system_free(struct Asset_System * system);

void asset_system_map_extension(struct Asset_System * system, char const * type_name, char const * extension);

void asset_system_set_type(struct Asset_System * system, char const * type_name, struct Asset_Callbacks callbacks, uint32_t value_size);
void asset_system_del_type(struct Asset_System * system, char const * type_name);

struct Asset_Ref asset_system_aquire(struct Asset_System * system, char const * name);
void asset_system_discard(struct Asset_System * system, struct Asset_Ref asset_ref);

void * asset_system_get_instance(struct Asset_System * system, struct Asset_Ref asset_ref);
void * asset_system_find_instance(struct Asset_System * system, char const * name);

//

extern struct Asset_Ref const asset_ref_empty;

#endif
