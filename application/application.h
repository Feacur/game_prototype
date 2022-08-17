#if !defined(APPLICATION_APPLICATION)
#define APPLICATION_APPLICATION

#include "framework/vector_types.h"

// @note: adaptive vsync mode (-1) might not make sense and is not tested

// @note: actual target frame time depends both on `vsync` and `frame_refresh_rate`

// @note: application accumulates frame time and calls `fixed_tick` whenever possible;
//        the leftover time rolls over to the subsequent frames

// @note: resulting frame time is capped at `ideal_frame_time * slow_frames_limit`;
//        `slow_frames_limit` helps in case of using a debugger

struct Application_Config {
	struct uvec2 size;
	bool flexible;
	int32_t vsync;               // 0: off; 1+: fraction of display refresh rate
	uint32_t frame_refresh_rate; // 0: as display; 1+: manual
	uint32_t fixed_refresh_rate; // 0: target frame time; 1+: N times per second
	uint32_t slow_frames_limit;  // 0: disabled; 1+: N target frames time
};

struct Application_Callbacks {
	void (* init)(void);
	void (* free)(void);
	void (* fixed_tick)(void);
	void (* frame_tick)(void);
};

void application_run(struct Application_Config config, struct Application_Callbacks callbacks);

struct uvec2 application_get_screen_size(void);
float application_get_delta_time(void);

#endif
