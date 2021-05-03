#if !defined(GAME_PROTOTYPE)
#define GAME_PROTOTYPE

#include "framework/containers/ref.h"

struct Asset_Program {
	struct Ref gpu_ref;
};

void asset_program_init(void * instance, char const * name);
void asset_program_free(void * instance);

#endif
