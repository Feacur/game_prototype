#if !defined(APPLICATION_UTILITIES)
#define APPLICATION_UTILITIES

#include "framework/common.h"

struct JSON;

void process_json(struct CString path, void * data, void (* action)(struct JSON const * json, void * output));

#endif
