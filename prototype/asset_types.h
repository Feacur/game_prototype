#if !defined(GAME_PROTOTYPE)
#define GAME_PROTOTYPE

#include "framework/containers/ref.h"

struct Font;

// -- Asset program part
struct Asset_Gpu_Program {
	struct Ref gpu_ref;
};

void asset_gpu_program_init(void * instance, char const * name);
void asset_gpu_program_free(void * instance);

// -- Asset font part
struct Asset_Font {
	struct Font * font;
};

void asset_font_init(void * instance, char const * name);
void asset_font_free(void * instance);

// -- Asset mesh part
struct Asset_Mesh {
	struct Ref gpu_ref;
};

void asset_mesh_init(void * instance, char const * name);
void asset_mesh_free(void * instance);

#endif
