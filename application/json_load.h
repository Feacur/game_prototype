#if !defined(APPLICATION_JSON_LOAD)
#define APPLICATION_JSON_LOAD

#include "framework/containers/handle.h"

struct JSON;
struct Font;

struct Handle json_load_gfx_material(struct JSON const * json);
void json_load_font_range(struct JSON const * json, struct Font * font);

#endif
