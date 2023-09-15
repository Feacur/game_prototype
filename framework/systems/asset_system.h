#if !defined(FRAMEWORK_SYSTEMS_ASSET_SYSTEM)
#define FRAMEWORK_SYSTEMS_ASSET_SYSTEM

#include "framework/containers/hash_table_u32.h"
#include "framework/containers/handle_table.h"

struct Asset_Callbacks {
	void (* type_init)(void);
	void (* type_free)(void);
	void (* init)(void * instance, struct CString name);
	void (* free)(void * instance);
};

void asset_system_init(void);
void asset_system_free(void);

void asset_system_map_extension(struct CString type_name, struct CString extension);
bool asset_system_match_type(struct Handle handle, struct CString type_name);

void asset_system_set_type(struct CString type_name, struct Asset_Callbacks callbacks, uint32_t value_size);
void asset_system_del_type(struct CString type_name);

struct Handle asset_system_aquire(struct CString name);
void asset_system_discard(struct Handle handle);

void * asset_system_take(struct Handle handle);
void * asset_system_aquire_instance(struct CString name);

struct CString asset_system_get_name(struct Handle handle);

#endif
