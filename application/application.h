#if !defined(GAME_APPLICATION)
#define GAME_APPLICATION

#include "framework/common.h"

struct Application_Config {
	uint32_t size_x, size_y;
	bool flexible;
	int32_t vsync;
	uint32_t target_refresh_rate, fixed_refresh_rate;
	uint32_t slow_frames_limit;
};

struct Application_Callbacks {
	void (* init)(void);
	void (* free)(void);
	void (* fixed_update)(uint64_t elapsed, uint64_t per_second);
	void (* frame_update)(uint64_t elapsed, uint64_t per_second);
	void (* draw_update)(uint64_t elapsed, uint64_t per_second);
};

void application_run(struct Application_Config config, struct Application_Callbacks callbacks);

void application_get_screen_size(uint32_t * size_x, uint32_t * size_y);

#endif
