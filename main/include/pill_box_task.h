#include "pill.h"

typedef enum PillBoxEventType {
    NONE,
    PILL_TIME_EVENT,
    CANCELL_PILL_TIME,
    TOOK_PILL,
    START_RELOAD,
    FINISH_RELOAD,
    LOAD_PILL,
    WILL_LOAD_PILL
} PillBoxEventType;

typedef struct WillLoadPillMessage {
    Pill pill_to_load;
} WillLoadPillMessage;

typedef union PillBoxMessage {
    WillLoadPillMessage will_load_pill_message;
} PillBoxMessage;

typedef struct PillBoxEvent {
    PillBoxEventType event_type;
    TaskHandle_t task_handle;
    PillBoxMessage message;
} PillBoxEvent;

typedef enum PillBoxState {
    IDLE,
    PILL_TIME,
    RELOADING
} PillBoxState;

typedef enum PillBoxTaskMessageResponse {
    TIMEOUT,
    SUCCESS,
    FAILED
} PillBoxTaskMessageResponse;

void start_pill_box_task();

PillBoxState get_pill_box_state();

void send_pill_box_event(PillBoxEventType event);

void send_pill_box_event_from_isr(PillBoxEventType event);

PillBoxTaskMessageResponse send_pill_box_event_sync(PillBoxEventType event_type, TaskHandle_t task_handle);

PillBoxTaskMessageResponse send_pill_box_message_sync(PillBoxEventType event_type, PillBoxMessage message, TaskHandle_t task_handle);