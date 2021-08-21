#if !defined(GAME_PLATFORM_GLIBRARY_TO_SYSTEM)
#define GAME_PLATFORM_GLIBRARY_TO_SYSTEM

// interface from `glibrary_*.c` to `system.c`
// - graphics library initialization

void glibrary_to_system_init(void);
void glibrary_to_system_free(void);

#endif
