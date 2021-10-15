#if !defined(GAME_FRAMEWORK_ASSET_TYPES)
#define GAME_FRAMEWORK_ASSET_TYPES

#include "framework/containers/ref.h"
#include "framework/graphics/font_image.h"
#include "framework/assets/json.h"

struct Font;
struct Font_Image;

// ----- ----- ----- ----- -----
//     Asset shader part
// ----- ----- ----- ----- -----

struct Asset_Shader {
	struct Ref gpu_ref;
};

void asset_shader_init(void * instance, struct CString name);
void asset_shader_free(void * instance);

// ----- ----- ----- ----- -----
//     Asset model part
// ----- ----- ----- ----- -----

struct Asset_Model {
	struct Ref gpu_ref;
};

void asset_model_init(void * instance, struct CString name);
void asset_model_free(void * instance);

// ----- ----- ----- ----- -----
//     Asset image part
// ----- ----- ----- ----- -----

struct Asset_Image {
	struct Ref gpu_ref;
};

void asset_image_init(void * instance, struct CString name);
void asset_image_free(void * instance);

// ----- ----- ----- ----- -----
//     Asset font part
// ----- ----- ----- ----- -----

// @todo: map font size to buffer and gpu_ref pair?
// @idea: put differently sized glyphs onto one atlas?

struct Asset_Font {
	struct Font * font;
	struct Font_Image * buffer;
	struct Ref gpu_ref;
};

void asset_font_init(void * instance, struct CString name);
void asset_font_free(void * instance);

// ----- ----- ----- ----- -----
//     Asset bytes part
// ----- ----- ----- ----- -----

struct Asset_Bytes {
	uint8_t * data;
	uint32_t length;
};

void asset_bytes_init(void * instance, struct CString name);
void asset_bytes_free(void * instance);

// ----- ----- ----- ----- -----
// -- Asset json part
// ----- ----- ----- ----- -----

struct Asset_JSON {
	struct JSON value;
};

void asset_json_type_init(void);
void asset_json_type_free(void);

void asset_json_init(void * instance, struct CString name);
void asset_json_free(void * instance);

#endif
