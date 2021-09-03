#if !defined(GAME_PLATFORM_SYSTEM_TO_INTERNAL)
#define GAME_PLATFORM_SYSTEM_TO_INTERNAL

// interface from `system.c` to anywhere
// - general purpose

#define APPLICATION_CLASS_NAME "game_prototype"
void * system_to_internal_get_module(void);

#endif
