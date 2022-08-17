#if !defined(APPLICATION_ASSET_REGISTRY)
#define APPLICATION_ASSET_REGISTRY

struct Asset_System;

void asset_types_init(struct Asset_System * system);
void asset_types_free(struct Asset_System * system);

#endif
