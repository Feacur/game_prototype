#if !defined(FRAMEWORK_SYSTEMS_MATERIAL_SYSTEM)
#define FRAMEWORK_SYSTEMS_MATERIAL_SYSTEM

#include "framework/common.h"

struct Gfx_Material;

void material_system_clear(bool deallocate);

struct Handle material_system_aquire(void);
void material_system_discard(struct Handle handle);

struct Gfx_Material * material_system_take(struct Handle handle);

#endif
