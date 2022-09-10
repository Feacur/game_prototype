#if !defined(APPLICATION_JSON_LOAD)
#define APPLICATION_JSON_LOAD

struct JSON;
struct Asset_System;
struct Gfx_Material;

void json_load_gfx_material(struct Asset_System * system, struct JSON const * json, struct Gfx_Material * result);

#endif
