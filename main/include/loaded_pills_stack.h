#include "pill.h"

/* This works like a buffer that can grow in both directions of the circle
    The start of the circle is where the last pill will be
    The end of the circle is the location of the "idle" space (one space after the next pill to be taken)

    The buffer starts with the start and end at the same place, after the first pill is loaded, the end moves one place

    DEFINITIONS:
    - The position numbers increases in the ACC direction
    - The end INCREASES in the ACW direction and DECREASES in the CCW direction
    - The start INCREASES in the CCW direction and DECREASES in the ACW direction
    - The minimum distance between the end and the start (measured in ACW) is 1 (we need the idle position)

    In usual, pills will be taken starting from the end (like a stack), but if the client
    wants to schedule more pills for the future, it can append to the start of the stack
 */

typedef struct LoadedPillsCircleStack {
    Pill* pills;
    int stack_size;
    int start;
    int end;
} LoadedPillsCircleStack;

void push_start(Pill pill);
void push_end(Pill pill);
void pop_start();
void pop_end();
Pill get_start_pill();
Pill get_end_pill();
int get_start();
int get_end();
int get_capacity_left();
void update_pill_by_key(char* pill_key, time_t new_datetime);