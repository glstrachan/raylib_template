#pragma once

typedef struct {
    string name;
    string description;
    Texture2D texture;
} Item_Data;

#define ITEM_LIST(X) \
    X(ITEM_VACUUM,   "Vacuum",   "Sucks up dirt") \
    X(ITEM_BOOKS,    "Books",    "Knowledge")     \
    X(ITEM_KNIVES,   "Knives",   "Sharp")         \
    X(ITEM_LOLLIPOP, "Lollipop", "Sweet treat")   \
    X(ITEM_COMPUTER, "Computer", "Beep boop")     \
    // X(ITEM_VACUUM,   "A", "")                  \
    // X(ITEM_BOOKS,    "B", "")                  \
    // X(ITEM_KNIVES,   "C", "")                  \
    // X(ITEM_LOLLIPOP, "D", "")                  \
    // X(ITEM_COMPUTER, "E", "")                  \
    // X(ITEM_VACUUM,   "F", "")                  \
    // X(ITEM_BOOKS,    "G",  "")     

#define X(id, name, desc) id,
Enum(Item_Type, uint32_t,
    ITEM_LIST(X)
    ITEM_COUNT
);
#undef X

void items_init();

void pick_items_init();

void pick_items_cleanup();

// Picks 6 items from the remaining inventory
// and puts them in the pickable_items
void choose_pickable();

void pick_items_update();