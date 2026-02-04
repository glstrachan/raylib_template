#pragma once


#define CHARACTERS_LIST(X)                 \
    X(CHARACTERS_NONE,   NULL)             \
    X(CHARACTERS_SALESMAN,   "Salesman")   \
    X(CHARACTERS_OLDLADY,    "Old Lady")   \
    X(CHARACTERS_NERD,   "Nerd")           \
    X(CHARACTERS_SHOTGUNNER, "Shotgunner") \
    X(CHARACTERS_FATMAN, "Fat Man")        \
    X(CHARACTERS_COUNT, "Count")

typedef struct {
    string name;
} Character_Data;

extern Character_Data character_data[];

typedef enum {
    #define X(id, name) id,
    CHARACTERS_LIST(X)
    #undef X
} Character_Type;

Enum(Character_Position, uint32_t,
    CHARACTERS_LEFT,
    CHARACTERS_RIGHT
);

void characters_draw(Character_Type type, Character_Position pos);
void characters_init();
void characters_cleanup();