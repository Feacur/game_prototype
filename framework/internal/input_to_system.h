#if !defined(GAME_INPUT_TO_SYSTEM)
#define GAME_INPUT_TO_SYSTEM

// interface from `input.c` to `system.c`
// - framework initialization and manipulation

void input_to_system_init(void);
void input_to_system_free(void);

void input_to_platform_before_update(void);
void input_to_platform_after_update(void);

#endif
