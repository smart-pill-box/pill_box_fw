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
#include "pill_box_task.h"

static QueueHandle_t pill_box_queue = NULL;
PillBoxState _state = IDLE;


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
    }
}

void reloading_state(){

}

void idle_state(){

}

void pill_time_state(){

}

PillBoxEvent get_event(){
    PillBoxEvent event;
    if(xQueueReceive(pill_box_queue, &event, 0)){
        return event;
    }

    event = NONE;

    return event;
}

void proccess_event(PillBoxEvent event){
    switch (_state)
    {
    case IDLE:
        idle_proccess_event(event);
        break;

    case PILL_TIME:
        pill_time_proccess_event(event);
        break;

    case RELOADING:
        reloading_proccess_event(event);
        break;
    
    default:
        break;
    }
}

void idle_proccess_event(PillBoxEvent event){
    switch (event)
    {
    case PILL_TIME:
        // Start buzzer
        _state = PILL_TIME;
        break;

    case TOOK_PILL:
        // Conferir se ele pode tomar agora o remédio
        break;
    
    case START_RELOAD:
        _state = RELOADING;
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
}

void pill_time_proccess_event(PillBoxEvent event){
    switch (event)
    {
    case PILL_TIME:
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
}

void reloading_proccess_event(PillBoxEvent event){
    switch (event)
    {
    case PILL_TIME:
        /* code */
        break;

    case TOOK_PILL:
        /* code */
        break;
    
    case START_RELOAD:
        /* code */
        break;

    case FINISH_RELOAD:
        /* code */
        break;

    case LOAD_PILL:
        /* code */
        break;
    
    default:
        break;
    }
}


void start_pill_box_task(){
    pill_box_queue = xQueueCreate(64, sizeof PillBoxEvent);

    xTaskCreate(pill_box_task, "pill_box_task", 2048, NULL, 1, NULL);
}