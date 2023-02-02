#if !defined(APPLICATION_JSON_LOAD)
#define APPLICATION_JSON_LOAD

struct JSON;
struct Asset_System;
struct Gfx_Material;
struct Glyph_Atlas;

void json_load_gfx_material(struct Asset_System * system, struct JSON const * json, struct Gfx_Material * result);
void json_load_glyph_atlas_range(struct Asset_System * system, struct JSON const * json, struct Glyph_Atlas * glyph_atlas);

#endif
