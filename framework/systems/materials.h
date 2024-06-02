#if !defined(FRAMEWORK_SYSTEMS_MATERIALS)
#define FRAMEWORK_SYSTEMS_MATERIALS

#include "framework/common.h"

struct Gfx_Material;

void system_materials_init(void);
void system_materials_free(void);

struct Handle system_materials_aquire(void);
HANDLE_ACTION(system_materials_discard);

struct Gfx_Material * system_materials_get(struct Handle handle);

#endif
