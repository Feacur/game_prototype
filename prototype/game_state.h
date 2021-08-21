#if !defined(GAME_PROTOTYPE_STATE)
#define GAME_PROTOTYPE_STATE

#include "framework/containers/array_any.h"
#include "framework/containers/ref.h"
#include "framework/systems/asset_system.h"

struct Batcher_2D;

struct Game_State {
	struct Batcher_2D * batcher;

	struct Asset_System asset_system;

	struct Array_Any materials;
	struct Array_Any cameras;
	struct Array_Any entities;
};

extern struct Game_State state;

//

void state_init(void);
void state_free(void);

#endif
