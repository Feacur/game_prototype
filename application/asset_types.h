#if !defined(APPLICATION_ASSET_TYPES)
#define APPLICATION_ASSET_TYPES

#include "framework/containers/handle.h"
#include "framework/assets/json.h"
#include "framework/graphics/material.h"

struct Typeface;
struct Font;

struct Asset_Bytes {
	uint8_t * data;
	uint32_t length;
};

struct Asset_JSON {
	struct JSON value;
};

struct Asset_Shader {
	struct Handle gpu_handle;
};

struct Asset_Image {
	struct Handle gpu_handle;
};

struct Asset_Typeface {
	struct Typeface * typeface;
};

struct Asset_Glyph_Atlas {
	struct Font * font;
	struct Handle gpu_handle;
};

struct Asset_Target {
	struct Handle gpu_handle;
};

struct Asset_Model {
	struct Handle gpu_handle;
};

struct Asset_Material {
	struct Gfx_Material value;
};

#endif
