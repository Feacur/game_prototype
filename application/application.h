#if !defined(GAME_APPLICATION)
#define GAME_APPLICATION

#include "framework/common.h"

struct Application_Config {
	struct {
		void (* init)(void);
		void (* free)(void);
		void (* fixed_update)(uint64_t elapsed, uint64_t per_second);
		void (* update)(uint64_t elapsed, uint64_t per_second);
		void (* render)(uint32_t size_x, uint32_t size_y);
	} callbacks;

	uint32_t size_x, size_y;
	int32_t vsync;
	uint32_t target_refresh_rate, fixed_refresh_rate;
	uint32_t slow_frames_limit;
};

void application_run(struct Application_Config * config);

#endif
