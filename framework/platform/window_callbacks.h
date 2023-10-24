#if !defined(FRAMEWORK_WINDOW_CALLBACKS)
#define FRAMEWORK_WINDOW_CALLBACKS

#include "framework/common.h"

struct Window_Callbacks {
	bool (* close)(void);
};

#endif
