#if !defined(GAME_PROTOTYPE_GAME_STATE)
#define GAME_PROTOTYPE_GAME_STATE

#include "framework/containers/array_any.h"
#include "framework/systems/asset_system.h"
#include "framework/graphics/material.h"

struct Batcher_2D;
struct JSON;

extern struct Game_State {
	struct Batcher_2D * batcher;
	struct Gfx_Uniforms uniforms;
	struct Array_Any gpu_commands; // `struct GPU_Command`

	struct Asset_System assets;

	struct Array_Any cameras;  // `struct Camera`
	struct Array_Any entities; // `struct Entity`
} gs_game;

//

void game_init(void);
void game_free(void);

void game_fill_scene(struct JSON const * json, void * data);

#endif
