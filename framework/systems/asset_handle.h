#if !defined(FRAMEWORK_SYSTEMS_ASSET_HANDLE)
#define FRAMEWORK_SYSTEMS_ASSET_HANDLE

#include "framework/containers/handle.h"

// @purpose: decouple `asset_system.h` with any other one

struct Asset_Handle {
	struct Handle instance_handle;
	uint32_t type_id, name_id;
};

inline static bool asset_handle_is_null(struct Asset_Handle v) {
	return handle_is_null(v.instance_handle)
	    || v.type_id == 0
	    || v.name_id == 0;
}

inline static bool asset_handle_equals(struct Asset_Handle v1, struct Asset_Handle v2) {
	return handle_equals(v1.instance_handle, v2.instance_handle)
	    && v1.type_id == v2.type_id
	    && v1.name_id == v2.name_id;
}

#endif
