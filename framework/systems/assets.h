#if !defined(FRAMEWORK_SYSTEMS_ASSETS)
#define FRAMEWORK_SYSTEMS_ASSETS

#include "framework/common.h"

struct Asset_Info {
	uint32_t size;
	Handle_Action * load;
	Handle_Action * drop;
};

void system_assets_clear(bool deallocate);

void system_assets_type_map(struct CString type_name, struct CString extension);
void system_assets_type_set(struct CString type_name, struct Asset_Info info);
void system_assets_type_del(struct CString type_name);

struct Handle system_assets_load(struct CString name);
HANDLE_ACTION(system_assets_drop);

void * system_assets_get(struct Handle handle);
struct Handle system_assets_find(struct CString name);
struct CString system_assets_get_type(struct Handle handle);
struct CString system_assets_get_name(struct Handle handle);

#endif
