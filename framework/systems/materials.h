#if !defined(FRAMEWORK_SYSTEMS_MATERIALS)
#define FRAMEWORK_SYSTEMS_MATERIALS

#include "framework/common.h"

struct Gfx_Material;

void system_materials_clear(bool deallocate);

struct Handle system_materials_aquire(void);
HANDLE_ACTION(system_materials_discard);

struct Gfx_Material * system_materials_get(struct Handle handle);

#endif
