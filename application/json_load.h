#if !defined(APPLICATION_JSON_LOAD)
#define APPLICATION_JSON_LOAD

#include "framework/common.h"

struct JSON;
struct Font;

struct Handle json_load_gfx_material(struct JSON const * json);
struct Handle json_load_font_range(struct JSON const * json, uint32_t * from, uint32_t * to);

#endif
