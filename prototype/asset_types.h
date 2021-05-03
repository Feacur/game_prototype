#if !defined(GAME_PROTOTYPE)
#define GAME_PROTOTYPE

#include "framework/containers/ref.h"

struct Font;

// -- Asset shader part
struct Asset_Shader {
	struct Ref gpu_ref;
};

void asset_shader_init(void * instance, char const * name);
void asset_shader_free(void * instance);

// -- Asset model part
struct Asset_Model {
	struct Ref gpu_ref;
};

void asset_model_init(void * instance, char const * name);
void asset_model_free(void * instance);

// -- Asset image part
struct Asset_Image {
	struct Ref gpu_ref;
};

void asset_image_init(void * instance, char const * name);
void asset_image_free(void * instance);

// -- Asset font part
struct Asset_Font {
	struct Font * font;
};

void asset_font_init(void * instance, char const * name);
void asset_font_free(void * instance);

#endif
