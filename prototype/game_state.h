#if !defined(GAME_PROTOTYPE_GAME_STATE)
#define GAME_PROTOTYPE_GAME_STATE

#include "framework/containers/array_any.h"
#include "framework/systems/asset_system.h"

struct Batcher_2D;
struct JSON;

extern struct Game_State {
	struct Batcher_2D * batcher;
	struct Buffer buffer;
	struct Array_Any gpu_commands;

	struct Asset_System assets;

	struct Array_Any cameras;
	struct Array_Any entities;
} gs_game;

//

void game_init(void);
void game_free(void);

void game_read_json(struct JSON const * json);

#endif
