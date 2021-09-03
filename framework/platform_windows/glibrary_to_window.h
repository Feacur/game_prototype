#if !defined(GAME_PLATFORM_GLIBRARY_TO_WINDOW)
#define GAME_PLATFORM_GLIBRARY_TO_WINDOW

// interface from `glibrary_*.c` to `window.c`
// - rendering context initialization

#include "framework/common.h"

struct Window;
struct GInstance;

struct GInstance * ginstance_init(struct Window * window);
void ginstance_free(struct GInstance * ginstance);

int32_t ginstance_get_vsync(struct GInstance const * ginstance);
void ginstance_set_vsync(struct GInstance * ginstance, int32_t value);
void ginstance_display(struct GInstance const * ginstance);

#endif
