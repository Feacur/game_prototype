#if !defined(GAME_INTERNAL_JSON_TO_SYSTEM)
#define GAME_INTERNAL_JSON_TO_SYSTEM

// interface from `json.c` to `system.c`
// - framework initialization and manipulation

void json_to_system_init(void);
void json_to_system_free(void);

#endif
