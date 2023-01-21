#if !defined(FRAMEWORK_SYSTEMS_ASSET_SYSTEM)
#define FRAMEWORK_SYSTEMS_ASSET_SYSTEM

#include "framework/containers/strings.h"
#include "framework/containers/hash_table_u32.h"
#include "framework/containers/handle_table.h"

#include "asset_handle.h"

struct Asset_System {
	struct Strings strings; // extensions, types, names
	struct Hash_Table_U32 handles; // name string id : instance handle
	struct Hash_Table_U32 types; // type string id : callbacks and instances
	struct Hash_Table_U32 map; // extension string id : type string id
};

struct Asset_Callbacks {
	void (* type_init)(void);
	void (* type_free)(void);
	void (* init)(struct Asset_System * system, void * instance, struct CString name);
	void (* free)(struct Asset_System * system, void * instance);
};

struct Asset_System asset_system_init(void);
void asset_system_free(struct Asset_System * system);

void asset_system_map_extension(struct Asset_System * system, struct CString type_name, struct CString extension);
bool asset_system_match_type(struct Asset_System * system, struct Asset_Handle handle, struct CString type_name);

void asset_system_set_type(struct Asset_System * system, struct CString type, struct Asset_Callbacks callbacks, uint32_t value_size);
void asset_system_del_type(struct Asset_System * system, struct CString type);

struct Asset_Handle asset_system_aquire(struct Asset_System * system, struct CString name);
void asset_system_discard(struct Asset_System * system, struct Asset_Handle handle);

void * asset_system_find_instance(struct Asset_System * system, struct Asset_Handle handle);
void * asset_system_aquire_instance(struct Asset_System * system, struct CString name);

struct CString asset_system_get_name(struct Asset_System * system, struct Asset_Handle handle);

#endif
