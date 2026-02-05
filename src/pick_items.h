#pragma once

#include "characters.h"

typedef struct {
    string name;
    string description; 
    Texture2D texture;
} Item_Data;

#define ITEM_LIST(X) \
    X(ITEM_NONE,              NULL,                NULL)                \
    X(ITEM_VACUUM,            "vacuum",            "A limited edition vacutron 380")     \
    X(ITEM_BOOKS,             "books",             "A bunch of textbooks")         \
    X(ITEM_KNIVES,            "knives",            "A high quality knife set")             \
    X(ITEM_LOLLIPOP,          "lollipop",          "Big swirly pop")       \
    X(ITEM_COMPUTER,          "computer",          "Turing complete computer")         \
    X(ITEM_VASE,              "vase",              "Holds flowers")     \
    X(ITEM_COMIC_BOOK,        "comic book",        "The first suberman comic")         \
    X(ITEM_FOOTBALL,          "football",          "Throw it long")     \
    X(ITEM_AK47,              "ak47",              "Fully automatic")   \
    X(ITEM_GIRLSCOUT_COOKIES, "girlscout cookies", "Thin mints and stuff!")       \
    X(ITEM_STEAK,             "steak",             "Served with a side of potatos and asparagus")       \
    X(ITEM_GAME_CONSOLE,      "game console",      "Press start")

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
