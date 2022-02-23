#if !defined(GAME_FRAMEWORK_ASSET_REF)
#define GAME_FRAMEWORK_ASSET_REF

#include "framework/containers/ref.h"

struct Asset_Ref {
	struct Ref instance_ref;
	uint32_t type_id, resource_id;
};

#endif
