#if !defined(FRAMEWORK_JSON_READ)
#define FRAMEWORK_JSON_READ

#include "framework/graphics/gfx_types.h"

void process_json(struct CString path, void * data, JSON_Processor * process);

// ----- ----- ----- ----- -----
//     common
// ----- ----- ----- ----- -----

uint32_t json_read_hex(struct JSON const * json);

void json_read_many_flt(struct JSON const * json, uint32_t length, float * result);
void json_read_many_u32(struct JSON const * json, uint32_t length, uint32_t * result);
void json_read_many_s32(struct JSON const * json, uint32_t length, int32_t * result);

// ----- ----- ----- ----- -----
//     graphics types
// ----- ----- ----- ----- -----

enum Gfx_Type json_read_gfx_type(struct JSON const * json);

enum Blend_Mode json_read_blend_mode(struct JSON const * json);
enum Depth_Mode json_read_depth_mode(struct JSON const * json);

enum Texture_Flag json_read_texture_flags(struct JSON const * json);
enum Lookup_Mode json_read_lookup_mode(struct JSON const * json);
enum Addr_Flag json_read_addr_flags(struct JSON const * json);
struct Gfx_Sampler json_read_sampler(struct JSON const * json);

struct Texture_Settings json_read_texture_settings(struct JSON const * json);

struct Target_Format json_read_target_format(struct JSON const * json);

// ----- ----- ----- ----- -----
//     graphics objects
// ----- ----- ----- ----- -----

struct Handle json_read_target(struct JSON const * json);

#endif
