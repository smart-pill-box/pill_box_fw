#include "pills_handler.h"
#include "loaded_pills_stack.h"
#include "carroucel.h"

Pill pill_to_load;
int wanted_position;
bool pushed_to_end;

bool is_prepared_to_load(){
    return get_carroucel_pos() == wanted_position && carroucel_is_on_position();
}

bool prepare_to_load_pill(Pill pill){
    if(get_capacity_left() == 0){
        return false;
    }
    Pill next_pill = get_end_pill();
    Pill last_pill = get_start_pill();

    if(pill.pill_datetime <= next_pill.pill_datetime || is_empty()) {
        wanted_position = get_end();

        carroucel_to_pos_acw(wanted_position);
        pill_to_load = pill;
        pushed_to_end = true;
    }
    else if(pill.pill_datetime >= last_pill.pill_datetime){
        wanted_position = backward(get_start());

        carroucel_to_pos_ccw(wanted_position);
        pill_to_load = pill;
        pushed_to_end = false;

    } else {
        return false;
    }

    return true;
}

bool load_pill(){
    if(!is_prepared_to_load()){
        return false;
    }
    if(pushed_to_end){
        carroucel_to_pos_ccw(forward(get_end()));
        push_end(pill_to_load);
    } else {
        carroucel_to_pos_acw(backward(backward(get_start())));
        push_start(pill_to_load);
    }

    return true;
}