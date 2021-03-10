#if !defined(GAME_APPLICATION)
#define GAME_APPLICATION

#include "framework/common.h"

struct Window;

struct Application_Config {
	struct {
		void (* init)(void);
		void (* free)(void);
		void (* fixed_update)(struct Window * window, uint64_t elapsed, uint64_t per_second);
		void (* update)(struct Window * window, uint64_t elapsed, uint64_t per_second);
		void (* render)(struct Window * window);
	} callbacks;

	int32_t vsync;
	uint32_t target_refresh_rate, fixed_refresh_rate;
	uint32_t slow_frames_factor;
};

void application_run(struct Application_Config * config);

#endif
