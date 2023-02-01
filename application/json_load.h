#if !defined(APPLICATION_JSON_LOAD)
#define APPLICATION_JSON_LOAD

struct JSON;
struct Asset_System;
struct Gfx_Material;
struct Font_Atlas;

void json_load_gfx_material(struct Asset_System * system, struct JSON const * json, struct Gfx_Material * result);
void json_load_fonts(struct Asset_System * system, struct JSON const * json, struct Font_Atlas * fonts);

#endif
