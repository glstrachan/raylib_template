#pragma once

#include "characters.h"

typedef struct {
    string name;
    string description;
    Texture2D texture;
} Item_Data;

#define ITEM_LIST(X) \
    X(ITEM_NONE,   NULL,   NULL) \
    X(ITEM_VACUUM,   "vacuum",   "Sucks up dirt") \
    X(ITEM_BOOKS,    "books",    "Knowledge")     \
    X(ITEM_KNIVES,   "knives",   "Sharp")         \
    X(ITEM_LOLLIPOP, "lollipop", "Sweet treat")   \
    X(ITEM_COMPUTER, "computer", "Beep boop")     \
    X(ITEM_A, "computer", "")                            \
    X(ITEM_B, "computer", "")                            \
    X(ITEM_C, "computer", "")                            \
    X(ITEM_D, "computer", "")                            \
    X(ITEM_E, "computer", "")                            \
    X(ITEM_F, "computer", "")                            \
    X(ITEM_G, "computer", "")                             

#define X(id, name, desc) id,
Enum(Item_Type, uint32_t,
    ITEM_LIST(X)
    ITEM_COUNT
);
#undef X

void items_init();
void items_cleanup();

void pick_items_init();

void pick_items_cleanup();

// Picks 6 items from the remaining inventory
// and puts them in the pickable_items
void choose_pickable();

void pick_items_update();

extern Item_Data item_data[];
