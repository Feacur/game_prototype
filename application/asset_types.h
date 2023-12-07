#if !defined(APPLICATION_ASSET_TYPES)
#define APPLICATION_ASSET_TYPES

#include "framework/assets/json.h"

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
	struct Handle gh_program;
};

struct Asset_Image {
	struct Handle gh_texture;
};

struct Asset_Typeface {
	struct Typeface * typeface;
};

struct Asset_Font {
	struct Font * font;
	struct Handle gh_texture;
};

struct Asset_Target {
	struct Handle gh_target;
};

struct Asset_Model {
	struct Handle gh_mesh;
};

struct Asset_Material {
	struct Handle mh_mat;
};

#endif
