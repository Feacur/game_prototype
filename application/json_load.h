#if !defined(APPLICATION_JSON_LOAD)
#define APPLICATION_JSON_LOAD

struct JSON;
struct Asset_System;
struct Gfx_Material;
struct Font;

void json_load_gfx_material(struct Asset_System * system, struct JSON const * json, struct Gfx_Material * result);
void json_load_font_range(struct Asset_System * system, struct JSON const * json, struct Font * font);

#endif
