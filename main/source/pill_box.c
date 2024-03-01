// Aqui preciso mexer com mutex, para alterar a data de remédios

#include "carroucel.h"
#include "pill.h"

typedef struct PillBox {
    Carroucel carroucel;
    Pill* pills;
} PillBox;

