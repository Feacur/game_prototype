#if !defined(APPLICATION_JSON_READ)
#define APPLICATION_JSON_READ

#include "framework/common.h"

struct JSON;

void process_json(struct CString path, void * data, void (* action)(struct JSON const * json, void * output));

uint32_t json_read_hex(struct JSON const * json);

void json_read_many_flt(struct JSON const * json, uint32_t length, float * result);
void json_read_many_u32(struct JSON const * json, uint32_t length, uint32_t * result);
void json_read_many_s32(struct JSON const * json, uint32_t length, int32_t * result);

#endif
