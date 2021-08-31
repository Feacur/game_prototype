#if !defined(GAME_PLATFORM_TIMER_TO_SYSTEM)
#define GAME_PLATFORM_TIMER_TO_SYSTEM

// interface from `timer.c` to `system.c`
// - framework initialization

void timer_to_system_init(void);
void timer_to_system_free(void);

#endif