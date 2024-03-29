#include "loaded_pills_stack.h"

LoadedPillsCircleStack _pills_stack;

void create_pills_stack(int stack_size){
    _pills_stack.stack_size = stack_size;
    _pills_stack.pills = malloc(sizeof(Pill) * stack_size);
    _pills_stack.start = 0;
    _pills_stack.end = 0;
}

/* Measured in the ACW direction */
int get_distance(int from_pos, int to_pos){
    if(to_pos >= from_pos){
        return to_pos - from_pos;
    }

    return _pills_stack.stack_size - (from_pos - to_pos);
}

/* Get the next position in CCW direction */
int forward(int pos){
    if(pos == _pills_stack.stack_size - 1){
        return 0;
    }

    return pos + 1;
}

/* Get the next position in ACCW direction */
int backward(int pos){
    if(pos == 0){
        return _pills_stack.stack_size - 1;
    }

    return pos - 1;
}

/* Push to the start of the stack, this will be the last popped pill */
void push_start(Pill pill){
    if(get_distance(_pills_stack.start, _pills_stack.end) == 1){
        // error
    }

    _pills_stack.pills[backward(_pills_stack.start)] = pill;
    _pills_stack.start = backward(_pills_stack.start);
}

/* Push to the end of the stack, this will be the next popped pill */
void push_end(Pill pill){
    if(get_distance(_pills_stack.start, _pills_stack.end) == 1){
        // error
    }

    _pills_stack.pills[_pills_stack.end] = pill;
    _pills_stack.end = forward(_pills_stack.end);
}

void pop_start(){
    if(_pills_stack.start == _pills_stack.end){
        // Error
    }

    _pills_stack.start = forward(_pills_stack.start);
}

void pop_end(){
    if(_pills_stack.start == _pills_stack.end){
        // Error
    }

    _pills_stack.end = backward(_pills_stack.end);
}

int get_start(){
    return _pills_stack.start;
}

int get_end(){
    return _pills_stack.end;
}

int get_capacity_left(){
    return get_distance(_pills_stack.end, _pills_stack.start) - 1;
}

bool is_empty(){
    return _pills_stack.end == _pills_stack.start;
}

Pill get_start_pill(){
    return _pills_stack.pills[_pills_stack.start];
}

Pill get_end_pill(){
    return _pills_stack.pills[backward(_pills_stack.end)];
}

void update_pill_by_key(char* pill_key, time_t new_datetime){
    // TODO lock
    Pill* pill_found = NULL;
    int found_pos;
    for (int i = _pills_stack.start; get_distance(i, _pills_stack.end) > 1; forward(i)){
        if(_pills_stack.pills[i].pill_key == pill_key){
            pill_found = &_pills_stack.pills[i];
            found_pos = i;
            break;
        }
    }

    if(pill_found == NULL){
        // error
    }
    if(new_datetime < _pills_stack.pills[backward(found_pos)].pill_datetime){
        // error
    }

    pill_found->pill_datetime = new_datetime;
}
