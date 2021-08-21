#if !defined(GAME_MEMORY_TO_SYSTEM)
#define GAME_MEMORY_TO_SYSTEM

// interface from `memory.c` to `system.c`
// - framework initialization

void memory_to_system_init(void);
void memory_to_system_free(void);

#endif
