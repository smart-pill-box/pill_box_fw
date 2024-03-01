typedef enum PillBoxEvent {
    NONE,
    PILL_TIME,
    CANCELL_PILL_TIME,
    TOOK_PILL,
    START_RELOAD,
    FINISH_RELOAD,
    LOAD_PILL
} PillBoxEvent;

typedef enum PillBoxState {
    IDLE,
    PILL_TIME,
    RELOADING
} PillBoxState;