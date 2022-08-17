#if !defined(FRAMEWORK_INPUT)
#define FRAMEWORK_INPUT

#include "common.h"
#include "input_keys.h"

struct Array_U32;

void input_reset_delta(void);
void input_update(void);

bool input_key(enum Key_Code key);
bool input_key_transition(enum Key_Code key, bool state);
void input_track_codepoints(bool state);
struct Array_U32 const * input_get_codepoints(void);

void input_mouse_position_window(uint32_t * x, uint32_t * y);
void input_mouse_position_display(uint32_t * x, uint32_t * y);
void input_mouse_delta(int32_t * x, int32_t * y);
bool input_mouse(enum Mouse_Code key);
bool input_mouse_transition(enum Mouse_Code key, bool state);

#endif
