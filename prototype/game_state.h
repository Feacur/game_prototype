#if !defined(PROTOTYPE_GAME_STATE)
#define PROTOTYPE_GAME_STATE

#include "framework/containers/array_any.h"

struct JSON;

extern struct Game_State {
	struct Array_Any cameras;  // `struct Camera`
	struct Array_Any entities; // `struct Entity`
} gs_game;

//

void game_init(void);
void game_free(void);

void game_fill_scene(struct JSON const * json, void * data);

#endif
