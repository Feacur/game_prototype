#if !defined(FRAMEWORK_SYSTEMS_ASSET_SYSTEM)
#define FRAMEWORK_SYSTEMS_ASSET_SYSTEM

#include "framework/common.h"

struct Asset_Info {
	uint32_t size;
	void (* load)(struct Handle handle);
	void (* drop)(struct Handle handle);
};

void asset_system_init(void);
void asset_system_free(void);

void asset_system_map_extension(struct CString type_name, struct CString extension);
void asset_system_set_type(struct CString type_name, struct Asset_Info info);
void asset_system_del_type(struct CString type_name);

struct Handle asset_system_load(struct CString name);
void asset_system_drop(struct Handle handle);

void * asset_system_get(struct Handle handle);
struct Handle asset_system_find(struct CString name);
struct CString asset_system_get_type(struct Handle handle);
struct CString asset_system_get_name(struct Handle handle);

#endif
