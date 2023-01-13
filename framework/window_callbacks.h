#if !defined(FRAMEWORK_WINDOW_CALLBACKS)
#define FRAMEWORK_WINDOW_CALLBACKS

#include "common.h"

struct Window_Callbacks {
	bool (* close)(void);
	void (* resize)(void);
};

#endif
