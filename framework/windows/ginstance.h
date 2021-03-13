#if !defined(GAME_PLATFORM_GLIBRARY)
#define GAME_PLATFORM_GLIBRARY

#include "framework/common.h"

struct Window;
struct GInstance;

struct GInstance * ginstance_init(struct Window * window);
void ginstance_free(struct GInstance * ginstance);

int32_t ginstance_get_vsync(struct GInstance * ginstance);
void ginstance_set_vsync(struct GInstance * ginstance, int32_t value);
void ginstance_display(struct GInstance * ginstance);

// void ginstance_size(struct GInstance * ginstance, uint32_t size_x, uint32_t size_y);

#endif
