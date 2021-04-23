#if !defined (GAME_WORLD)
#define GAME_WORLD

#include "framework/containers/array_any.h"
#include "framework/containers/ref_table.h"

struct World {
	struct Ref_Table entities;
};

void world_init(struct World * world);
void world_free(struct World * world);

#endif
