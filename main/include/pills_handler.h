#include <stdbool.h>
#include "pill.h"

/* Prepare to load the pill, will move the carroucel to the position needed
    if it can't load that pill it will return false. Call is_prepared_to_load to 
    pull the carroucel already got to the desired position */
bool prepare_to_load_pill(Pill pill);

/* Returns True if the carroucel is in position and you can load, false instead */
bool is_prepared_to_load();

/* First call prepare_to_load_pill and then call load_pill to load */
bool load_pill();

/* Return the next pill that is loaded */
Pill get_next_pill();

/* Return the last pill that is already loaded */
Pill get_last_pill();
