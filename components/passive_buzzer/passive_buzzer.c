#include "passive_buzzer.h"
#include "driver/ledc.h"
#include "hal/ledc_types.h"

void passive_buzzer_set_duty(PassiveBuzzerHandler handle, int duty);

void passive_buzzer_set_duty(PassiveBuzzerHandler handle, int duty){
	ledc_set_duty(handle.mode,handle.channel, duty);
	ledc_update_duty(handle.mode, handle.channel);
}

PassiveBuzzerHandler* passive_buzzer_init(int gpio_num){
	PassiveBuzzerHandler* handle = (PassiveBuzzerHandler*) malloc(sizeof(PassiveBuzzerHandler));

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = 100,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
	printf("\n Config timer");
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = gpio_num,
        .duty           = 0, // Set duty to 0%
    };
	printf("\n Config timer");
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

	handle->mode = LEDC_HIGH_SPEED_MODE;
	handle->channel = LEDC_CHANNEL_0;
	handle->timer = LEDC_TIMER_0;

	return handle;
}

void passive_buzzer_play(PassiveBuzzerHandler handle){
	passive_buzzer_set_duty(handle, 126);
}

void passive_buzzer_pause(PassiveBuzzerHandler handle){
	passive_buzzer_set_duty(handle, 0);
}

void passive_buzzer_set_freq(PassiveBuzzerHandler handle, uint32_t freq_hz){
	ledc_set_freq(handle.mode, handle.timer, freq_hz);
}

void passive_buzzer_destroy(PassiveBuzzerHandler *passive_buzzer){
	ledc_stop(passive_buzzer->mode, passive_buzzer->channel, 0);
	free(passive_buzzer);
}

