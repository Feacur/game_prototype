#include "framework/common.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//
#include "world.h"

struct Entity {
	uint8_t dummy;
	struct Ref components[FLEXIBLE_ARRAY];
};

void world_init(struct World * world) {
	ref_table_init(&world->entities, sizeof(struct Entity));
}

void world_free(struct World * world) {
	ref_table_free(&world->entities);

	memset(world, 0, sizeof(*world));
}
