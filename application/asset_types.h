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
	struct Ref gpu_ref;
};

struct Asset_Image {
	struct Ref gpu_ref;
};

struct Asset_Font {
	// @todo: map font size to buffer and gpu_ref pair?
	// @idea: put differently sized glyphs onto one atlas?
	struct Font * font;
	struct Font_Atlas * font_atlas;
	struct Ref gpu_ref;
};

struct Asset_Target {
	struct Ref gpu_ref;
};

struct Asset_Model {
	struct Ref gpu_ref;
};

struct Asset_Material {
	struct Gfx_Material value;
};

#endif
