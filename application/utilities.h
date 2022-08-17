#if !defined(APPLICATION_UTILITIES)
#define APPLICATION_UTILITIES

#include "framework/common.h"

struct JSON;

void process_json(void (* action)(struct JSON const * json, void * output), void * data, struct CString path);

#endif
