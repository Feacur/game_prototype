#if !defined(APPLICATION_ASSET_TYPES)
#define APPLICATION_ASSET_TYPES

#include "framework/containers/handle.h"
#include "framework/assets/json.h"
#include "framework/graphics/material.h"

struct Font;
struct Font_Atlas;

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

struct Asset_Font {
	// @todo: map font size to buffer and gpu_handle pair?
	// @idea: put differently sized glyphs onto one atlas?
	struct Font * font;
	struct Font_Atlas * font_atlas;
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
