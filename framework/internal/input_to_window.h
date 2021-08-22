#if !defined(GAME_INTERNAL_INPUT_TO_WINDOW)
#define GAME_INTERNAL_INPUT_TO_WINDOW

// interface from `input.c` to `system.c`
// - framework initialization and manipulation

#include "framework/common.h"
#include "framework/input_keys.h"

void input_to_platform_reset(void);

void input_to_platform_on_key(enum Key_Code key, bool is_down);
void input_to_platform_on_codepoint(uint32_t codepoint);

void input_to_platform_on_mouse_move(uint32_t x, uint32_t y);
void input_to_platform_on_mouse_move_window(uint32_t x, uint32_t y);
void input_to_platform_on_mouse_delta(int32_t x, int32_t y);
void input_to_platform_on_mouse_wheel(float x, float y);
void input_to_platform_on_mouse(enum Mouse_Code key, bool is_down);

#endif
