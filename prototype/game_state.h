#if !defined(GAME_PROTOTYPE_GAME_STATE)
#define GAME_PROTOTYPE_GAME_STATE

#include "framework/containers/array_any.h"
#include "framework/systems/asset_system.h"

struct JSON;

extern struct Game_State {
	struct Asset_System assets;
	struct Array_Any cameras;  // `struct Camera`
	struct Array_Any entities; // `struct Entity`
} gs_game;

//

void game_init(void);
void game_free(void);

void game_fill_scene(struct JSON const * json, void * data);

#endif
