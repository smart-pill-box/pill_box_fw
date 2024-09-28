#include "pills_handler.h"
#include "loaded_pills_stack.h"
#include "carroucel.h"
#include "api_client.h"

Pill pill_to_load;
int wanted_position;
bool pushed_to_end;

bool is_prepared_to_move(){
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
    if(!is_prepared_to_move()){
        return false;
    }
    if(pushed_to_end){
		wanted_position = forward(get_end());
        carroucel_to_pos_ccw(wanted_position);
        push_end(pill_to_load);
		post_device_pill(get_end(), &pill_to_load);
    } else {
		wanted_position = backward(backward(get_start()));
        carroucel_to_pos_acw(wanted_position);
        push_start(pill_to_load);
		post_device_pill(get_start(), &pill_to_load);
    }
	free(pill_to_load.pill_key);

    return true;
}

bool take_next_pill(){
	if(!is_prepared_to_move()){
		printf("\n CARROUCEL NOT PREPARED TO MOVE \n");
		return false;
	}

	int wanted_position = backward(get_end());
	pop_end();
	carroucel_to_pos_acw(wanted_position);

	return true;
}

Pill get_next_pill(){
	return get_end_pill();
}

bool have_pill_available() {
	return !is_empty();
}
