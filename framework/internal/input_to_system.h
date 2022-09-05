#if !defined(FRAMEWORK_INTERNAL_INPUT_TO_SYSTEM)
#define FRAMEWORK_INTERNAL_INPUT_TO_SYSTEM

// @purpose: interface from `input.c` to `system.c`
// - framework initialization and manipulation

#include "framework/common.h"

bool input_to_system_init(void);
void input_to_system_free(void);

void input_to_platform_before_update(void);
void input_to_platform_after_update(void);

#endif
