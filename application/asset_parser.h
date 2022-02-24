#if !defined(GAME_FRAMEWORK_ASSET_JSON_PARSER)
#define GAME_FRAMEWORK_ASSET_JSON_PARSER

#include "framework/graphics/types.h"

struct Ref;
struct JSON;
struct Array_Any;
struct Asset_System;
struct Gfx_Material;
struct Blend_Mode;
struct Depth_Mode;

enum Texture_Type state_read_json_texture_type(struct JSON const * json);

void state_read_json_float_n(struct JSON const * json, uint32_t length, float * result);
void state_read_json_u32_n(struct JSON const * json, uint32_t length, uint32_t * result);
void state_read_json_s32_n(struct JSON const * json, uint32_t length, int32_t * result);

void state_read_json_blend_mode(struct JSON const * json, struct Blend_Mode * result);
void state_read_json_depth_mode(struct JSON const * json, struct Depth_Mode * result);

void state_read_json_target(struct JSON const * json, struct Ref * result);
void state_read_json_material(struct Asset_System * system, struct JSON const * json, struct Gfx_Material * result);

#endif
