// Fila de eventos:
// - PillTime: sinaliza a hora de um remédio
// - StartReload: sinaliza o inicio de um carregamento
// - FinishReload: sinaliza o fim do carregamento
// - LoadPill: sinaliza que um novo remédio foi carregado
// - TookPill: sinaliza que um remédio foi tomado

// Três  estados
// Idle :
    // Confere se já é hora de um remédio
    // Se for, passa para o estado PillTime
    // Se receber requisição e estiver nesse estado vai para recarregando

// PillTime:
    // Chegou a hora do remédio, ativa o buzzer por um tempo
    // Se o botão for pressionado e estiver nesse estado, se move, para o buzzer, avisa o servidor e volta para idle
    // Se estiver sem conexão com a internet, aqui precisamos guardar oq aconteceu para avisar depois (uma task de sincronização)

// Recarregando
    // Se estiver nesse estado e pedir para terminar, volta para Idle
    // Se estiver nesse estado e pedir para carregar, move o carrocel um espaço

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <freertos/queue.h>
#include <string.h>
#include <time.h>
#include "alarm.h"
#include "carroucel.h"
#include "pill_box_task.h"
#include "pills_handler.h"
#include "xtensa/corebits.h"

#define ALARM_DURATION_SEC    60  // 1 min
#define ALARM_RETRY_EVERY_SEC 600 // 60 * 10 = 10 mins
#define RELOADING_STATE_TIMEOUT_SEC 20
#define TAG "pill_box_task"

// typedef enum ReloadingState {
//     IDLE,
//     PREPARING_TO_LOAD,
//     LOADING,
// } ReloadingState;

static QueueHandle_t pill_box_queue = NULL;
PillBoxState _state = IDLE;
time_t last_reloading_state_event;

void pill_box_task();
void reloading_state();
void idle_state();
void pill_time_state();
PillBoxEvent get_event();
void proccess_event(PillBoxEvent event);
PillBoxTaskMessageResponse idle_proccess_event(PillBoxEvent event);
PillBoxTaskMessageResponse pill_time_proccess_event(PillBoxEvent event);
PillBoxTaskMessageResponse reloading_proccess_event(PillBoxEvent event);
bool is_next_pill_time();
int time_late_since_next_pill();
void goto_state(PillBoxState state);
 
void goto_state(PillBoxState state){
	ESP_LOGI(TAG, "Going from state %d, to state %d", _state, state);
	if(state == RELOADING){
		time(&last_reloading_state_event);
		last_reloading_state_event = time(NULL);
	}

	_state = state;
}

bool is_next_pill_time(){
	if(!have_pill_available()){
		return false;
	}
    Pill next_pill = get_next_pill();

	return time(NULL) >= next_pill.pill_datetime;
}

int time_late_since_next_pill(){
	if(!have_pill_available()){
		return -1;
	}
	Pill next_pill = get_next_pill();

	time_t now = time(NULL);

    return now - next_pill.pill_datetime;
}

void pill_box_task(){
    while(1){
        PillBoxEvent event = get_event();
        proccess_event(event);
        switch (_state)
        {
        case IDLE:
            idle_state();
            break;

        case PILL_TIME:
            pill_time_state();
            break;

        case RELOADING:
            reloading_state();
            break;
        
        default:
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void reloading_state(){
	time_t now;
	time(&now);
	printf("\n\n now: %lld", now);
	printf("\n last_reloading_state_event: %lld", last_reloading_state_event);
	printf("\n %lld", now - last_reloading_state_event);
	if(now - last_reloading_state_event >= RELOADING_STATE_TIMEOUT_SEC){
		ESP_LOGI(TAG, "RELOADING TIMEOUT, returning to IDLE");
		goto_state(IDLE);	
	}
}

void idle_state(){
    if(is_next_pill_time()){
		ESP_LOGI(TAG, "Is next pill time, sending pill_time event");
        send_pill_box_event(PILL_TIME_EVENT);
    }
}

void pill_time_state(){
    static bool is_playing = false;
    int time_late = time_late_since_next_pill(); 
    if(time_late < 0){
        alarm_pause();
        is_playing = false;
		goto_state(IDLE);
		return;
    }

    if(!is_playing && ((time_late % ALARM_RETRY_EVERY_SEC) < ALARM_DURATION_SEC)){
        is_playing = true;
        alarm_ring();
    } else if (is_playing && ((time_late % ALARM_RETRY_EVERY_SEC) > ALARM_DURATION_SEC)){
        is_playing = false;
        alarm_pause();
    }
}

PillBoxState get_pill_box_state(){
    return _state;
}

void send_pill_box_event(PillBoxEventType event_type){
    PillBoxEvent event = {
        .event_type = event_type,
        .task_handle = NULL,
    };
    xQueueSend(pill_box_queue, &event, 0);
}

PillBoxTaskMessageResponse send_pill_box_event_sync(PillBoxEventType event_type, TaskHandle_t task_handle){
    PillBoxEvent event = {
        .event_type = event_type,
        .task_handle = task_handle,
    };
    xQueueSend(pill_box_queue, &event, 0);

    uint32_t notification;
    if (xTaskNotifyWait(0, 0, &notification, NULL)){
        return (PillBoxTaskMessageResponse) notification;
    }

    return TIMEOUT;
}

PillBoxTaskMessageResponse send_pill_box_message_sync(PillBoxEventType event_type, PillBoxMessage message, TaskHandle_t task_handle){

	if(event_type == WILL_LOAD_PILL) {
		char * pill_key = malloc(sizeof(char) * 37);
		memcpy(pill_key, message.will_load_pill_message.pill_to_load.pill_key, sizeof(char) * 37);
		message.will_load_pill_message.pill_to_load.pill_key = pill_key;
	}

    PillBoxEvent event = {
        .event_type = event_type,
        .message = message,
        .task_handle = task_handle,
    };
    xQueueSend(pill_box_queue, &event, 0);

    uint32_t notification;
    if (xTaskNotifyWait(0, 0, &notification, NULL)){
        return (PillBoxTaskMessageResponse) notification;
    }

    return TIMEOUT;
}

void send_pill_box_event_from_isr(PillBoxEventType event_type){
    PillBoxEvent event = {
        .event_type = event_type,
        .task_handle = NULL
    };
    xQueueSendFromISR(pill_box_queue, &event, NULL);
}

PillBoxEvent get_event(){
    PillBoxEvent event;
    if(xQueueReceive(pill_box_queue, &event, 0)){
        return event;
    }

    event.event_type = NONE;
    event.task_handle = NULL;

    return event;
}

void proccess_event(PillBoxEvent event){
    PillBoxTaskMessageResponse response = FAILED;

    switch (_state)
    {
    case IDLE:
        response = idle_proccess_event(event);
        break;

    case PILL_TIME:
        response = pill_time_proccess_event(event);
        break;

    case RELOADING:
        response = reloading_proccess_event(event);
        break;
    
    default:
        break;
    }

    if(event.task_handle != NULL){
        xTaskNotify(event.task_handle, response, eSetValueWithOverwrite);
    }
}

PillBoxTaskMessageResponse idle_proccess_event(PillBoxEvent event){
    switch (event.event_type)
    {
    case PILL_TIME_EVENT:
		goto_state(PILL_TIME);
        break;

    case TOOK_PILL:
        break;
    
    case START_RELOAD:
		goto_state(RELOADING);
        printf("Going to state RELOADING\n");
        break;

    case FINISH_RELOAD:
        // Erro
        break;

    case LOAD_PILL:
        break;
    
    default:
        break;
    }

    return SUCCESS;
}

PillBoxTaskMessageResponse pill_time_proccess_event(PillBoxEvent event){
    switch (event.event_type)
    {
    case PILL_TIME_EVENT:
        // Já chegou a hora do próximo, o que fazer ?
        break;

    case TOOK_PILL:
		printf("\n\n RECEIVED TOOK PILL EVENT\n\n");
		take_next_pill();
        break;
    
    case START_RELOAD:
        // Erro
        break;

    case FINISH_RELOAD:
        // Erro
        break;

    case LOAD_PILL:
        // Erro
        break;
    
    default:
        break;
    }

    return SUCCESS;
}

PillBoxTaskMessageResponse reloading_proccess_event(PillBoxEvent event){

	if(event.event_type != NONE){
		last_reloading_state_event = time(NULL);
	}

    switch (event.event_type)
    {
    case PILL_TIME_EVENT:
        /* code */
        break;

    case TOOK_PILL:
        /* code */
        break;
    
    case START_RELOAD:
        return FAILED;
        break;

    case FINISH_RELOAD:
		goto_state(IDLE);
        break;

    case WILL_LOAD_PILL:
        WillLoadPillMessage message = event.message.will_load_pill_message;
        if(!prepare_to_load_pill(message.pill_to_load)){
            return FAILED;
        }
        break;

    case LOAD_PILL:
        if(!load_pill()){
            return FAILED;
        }
        break;
    
    default:
        break;
    }

    return SUCCESS;
}

void start_pill_box_task(){
    pill_box_queue = xQueueCreate(64, sizeof(PillBoxEvent));

    xTaskCreate(pill_box_task, "pill_box_task", 2048, NULL, 1, NULL);
}
