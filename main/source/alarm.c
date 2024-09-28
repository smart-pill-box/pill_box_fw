#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "alarm.h"
#include "freertos/portmacro.h"

bool is_alarm_on = false;
PassiveBuzzerHandler* passive_buzzer;

void alarm_task();

void alarm_init(int gpio_num){
	passive_buzzer = passive_buzzer_init(gpio_num);

	xTaskCreate(alarm_task, "alarm_task", 1024, NULL, 2, NULL);
}

void alarm_ring();

void alarm_pause();

void play_beep(){
	passive_buzzer_set_freq(*passive_buzzer, 2200);
	passive_buzzer_play(*passive_buzzer);

	vTaskDelay(100 / portTICK_PERIOD_MS);

	passive_buzzer_pause(*passive_buzzer);

	vTaskDelay(100 / portTICK_PERIOD_MS);

	passive_buzzer_play(*passive_buzzer);

	vTaskDelay(100 / portTICK_PERIOD_MS);

	passive_buzzer_pause(*passive_buzzer);

	vTaskDelay(100 / portTICK_PERIOD_MS);

	passive_buzzer_play(*passive_buzzer);

	vTaskDelay(100 / portTICK_PERIOD_MS);

	passive_buzzer_pause(*passive_buzzer);

	vTaskDelay(100 / portTICK_PERIOD_MS);

	passive_buzzer_play(*passive_buzzer);

	vTaskDelay(100 / portTICK_PERIOD_MS);

	passive_buzzer_pause(*passive_buzzer);

	vTaskDelay(500 / portTICK_PERIOD_MS);
}

void alarm_task(){
	while(true){
		if(is_alarm_on){
			play_beep();
		} else {
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
	}
}

void alarm_ring(){
	is_alarm_on = true;
}

void alarm_pause(){
	is_alarm_on = false;
}
