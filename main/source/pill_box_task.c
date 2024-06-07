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
#include <freertos/queue.h>
#include <string.h>
#include "carroucel.h"
#include "pill_box_task.h"
#include "pills_handler.h"
#include "xtensa/corebits.h"

#define ALARM_DURATION_SEC    60*1  // 1 min
#define ALARM_RETRY_EVERY_SEC 60*10 // 10 mins

// typedef enum ReloadingState {
//     IDLE,
//     PREPARING_TO_LOAD,
//     LOADING,
// } ReloadingState;

static QueueHandle_t pill_box_queue = NULL;
PillBoxState _state = IDLE;
// ReloadingState _reloading_state = IDLE;

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

bool is_next_pill_time(){
    // Pill next_pill = get_next_pill();

    // TODO comparar com agora

    return true;
}

int time_late_since_next_pill(){
    // Pill next_pill = get_next_pill();

    // TODO comparar com agora

    return 10;
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

}

void idle_state(){
    if(is_next_pill_time()){
        // send_pill_box_event(PILL_TIME);
    }
}

void pill_time_state(){
    static bool is_playing = false;
    int time_late = time_late_since_next_pill(); 
    if(time_late < 0){
        // TODO stop_buzzer();
        is_playing = false;
        send_pill_box_event(CANCELL_PILL_TIME);
    }

    if(!is_playing && time_late % ALARM_RETRY_EVERY_SEC < ALARM_DURATION_SEC){
        is_playing = true;
        // buzzer_play();
    } else if (is_playing && time_late % ALARM_RETRY_EVERY_SEC > ALARM_DURATION_SEC){
        is_playing = false;
        // buzzer_stop();
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
        _state = PILL_TIME;
        break;

    case TOOK_PILL:
        // Conferir se ele pode tomar agora o remédio
        break;
    
    case START_RELOAD:
        _state = RELOADING;
        printf("Going to state RELOADING\n");
        // _reloading_state = IDLE;
        // carroucel_to_pos_ccw(backward(get_start()));
        // Carroucel to position 
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
        _state = IDLE;
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
        _state = IDLE;
        break;

    case WILL_LOAD_PILL:
        WillLoadPillMessage message = event.message.will_load_pill_message;
		printf("\nHERE: %s\n", message.pill_to_load.pill_key);
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
