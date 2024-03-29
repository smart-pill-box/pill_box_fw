#include "time.h"
#ifndef PILL_H
#define PILL_H

typedef struct Pill {
    time_t pill_datetime;
    char* pill_key;
} Pill;

#endif