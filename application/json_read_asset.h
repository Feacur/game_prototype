#if !defined(APPLICATION_JSON_READ_ASSET)
#define APPLICATION_JSON_READ_ASSET

#include "framework/containers/ref.h"

struct JSON;
struct Asset_System;
struct Gfx_Material;

struct Ref json_read_target(struct JSON const * json);

struct Ref json_read_texture(struct Asset_System * system, struct JSON const * json);
void json_read_many_texture(struct Asset_System * system, struct JSON const * json, uint32_t length, struct Ref * result);
void json_read_material(struct Asset_System * system, struct JSON const * json, struct Gfx_Material * result);

#endif
