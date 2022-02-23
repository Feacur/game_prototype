#if !defined(GAME_FRAMEWORK_ASSET_SYSTEM)
#define GAME_FRAMEWORK_ASSET_SYSTEM

#include "framework/containers/strings.h"
#include "framework/containers/hash_table_u32.h"
#include "framework/containers/ref_table.h"

#include "asset_ref.h"

struct Asset_Callbacks {
	void (* type_init)(void);
	void (* type_free)(void);
	void (* init)(void * instance, struct CString name);
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

void asset_system_map_extension(struct Asset_System * system, struct CString type_name, struct CString extension);

void asset_system_set_type(struct Asset_System * system, struct CString type, struct Asset_Callbacks callbacks, uint32_t value_size);
void asset_system_del_type(struct Asset_System * system, struct CString type);

struct Asset_Ref asset_system_aquire(struct Asset_System * system, struct CString name);
void asset_system_discard(struct Asset_System * system, struct Asset_Ref asset_ref);

void * asset_system_find_instance(struct Asset_System * system, struct Asset_Ref asset_ref);
void * asset_system_aquire_instance(struct Asset_System * system, struct CString name);

struct CString asset_system_get_name(struct Asset_System * system, struct Asset_Ref asset_ref);

//

extern struct Asset_Ref const с_asset_ref_empty;

#endif
