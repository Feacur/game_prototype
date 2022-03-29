#if !defined(GAME_FRAMEWORK_ASSET_JSON_PARSER)
#define GAME_FRAMEWORK_ASSET_JSON_PARSER

#include "framework/containers/ref.h"
#include "framework/graphics/material.h"
#include "framework/graphics/types.h"

struct JSON;
struct Array_Any;
struct Asset_System;

enum Texture_Type state_read_json_texture_type(struct JSON const * json);
enum Filter_Mode state_read_json_filter_mode(struct JSON const * json);
enum Wrap_Mode state_read_json_wrap_mode(struct JSON const * json);

struct Texture_Settings state_read_json_texture_settings(struct JSON const * json);

void state_read_json_unt_n(struct Asset_System * system, struct JSON const * json, uint32_t length, struct Ref * result);
void state_read_json_flt_n(struct JSON const * json, uint32_t length, float * result);
void state_read_json_u32_n(struct JSON const * json, uint32_t length, uint32_t * result);
void state_read_json_s32_n(struct JSON const * json, uint32_t length, int32_t * result);

struct Blend_Mode state_read_json_blend_mode(struct JSON const * json);
struct Depth_Mode state_read_json_depth_mode(struct JSON const * json);

struct Ref state_read_json_target(struct JSON const * json);
struct Gfx_Material state_read_json_material(struct Asset_System * system, struct JSON const * json);

#endif
