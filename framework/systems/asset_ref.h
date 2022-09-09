#if !defined(FRAMEWORK_SYSTEMS_ASSET_REF)
#define FRAMEWORK_SYSTEMS_ASSET_REF

#include "framework/containers/ref.h"

// @purpose: decouple `asset_system.h` with any other one

struct Asset_Ref {
	struct Ref instance_ref;
	uint32_t type_id, name_id;
};

inline static bool asset_ref_equals(struct Asset_Ref v1, struct Asset_Ref v2) {
	return ref_equals(v1.instance_ref, v2.instance_ref)
	    && v1.type_id == v2.type_id
	    && v1.name_id == v2.name_id;
}

#endif
