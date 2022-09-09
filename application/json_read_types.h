#if !defined(APPLICATION_JSON_READ_TYPES)
#define APPLICATION_JSON_READ_TYPES

#include "framework/graphics/types.h"

struct JSON;

enum Texture_Type json_read_texture_type(struct JSON const * json);
enum Filter_Mode json_read_filter_mode(struct JSON const * json);
enum Wrap_Mode json_read_wrap_mode(struct JSON const * json);
enum Blend_Mode json_read_blend_mode(struct JSON const * json);
enum Depth_Mode json_read_depth_mode(struct JSON const * json);

struct Texture_Parameters json_read_texture_parameters(struct JSON const * json);
struct Texture_Settings json_read_texture_settings(struct JSON const * json);

#endif
