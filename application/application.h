#if !defined(APPLICATION_APPLICATION)
#define APPLICATION_APPLICATION

#include "framework/maths_types.h"
#include "framework/window_callbacks.h"

// @note: adaptive vsync mode (-1) might not make sense and is not tested

// @note: actual target frame time depends both on `vsync` and `target_refresh_rate`

// @note: application accumulates frame time and calls `fixed_tick` whenever possible;
//        the leftover time rolls over to the subsequent frames

struct Application_Config {
	struct uvec2 size;
	bool resizable;
	int32_t vsync;                // 0: off; 1+: fraction of display refresh rate
	uint32_t target_refresh_rate; // 0: as display; 1+: N times per second
	uint32_t fixed_refresh_rate;  // 0: as target; 1+: N times per second
};

struct Application_Callbacks {
	void (* init)(void);
	void (* free)(void);
	void (* fixed_tick)(void);
	void (* frame_tick)(void);
	struct Window_Callbacks window_callbacks;
};

void application_update(void);
void application_run(struct Application_Config config, struct Application_Callbacks callbacks);
void application_exit(void);

struct uvec2 application_get_screen_size(void);
double application_get_delta_time(void);
bool application_is_inited(void);

#endif
