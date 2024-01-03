#if !defined(FRAMEWORK_PARSING)
#define FRAMEWORK_PARSING

#include "common.h"

uint32_t parse_u32(struct CString value);
uint32_t parse_h32(struct CString value);

int32_t parse_s32(struct CString value);

float parse_r32(struct CString value);
double parse_r64(struct CString value);

#endif
