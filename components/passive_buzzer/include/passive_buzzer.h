#include "driver/ledc.h"

typedef struct PassiveBuzzerHandler {
	ledc_mode_t mode;
	ledc_channel_t channel;
	ledc_timer_t timer;

} PassiveBuzzerHandler;


PassiveBuzzerHandler* passive_buzzer_init(int gpio);

void passive_buzzer_destroy(PassiveBuzzerHandler* passive_buzzer);

void passive_buzzer_play(PassiveBuzzerHandler handle);

void passive_buzzer_pause(PassiveBuzzerHandler handle);

void passive_buzzer_set_freq(PassiveBuzzerHandler handle, uint32_t freq_hz);

