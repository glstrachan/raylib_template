#pragma once

#include "characters.h"

typedef struct {
    string name;
    string description; 
    Texture2D texture;
} Item_Data;

#define ITEM_LIST(X) \
    X(ITEM_NONE,              NULL,               NULL)                \
    X(ITEM_VACUUM,            "vacuum",           "Sucks up dirt")     \
    X(ITEM_BOOKS,             "books",            "Knowledge")         \
    X(ITEM_KNIVES,            "knives",           "Sharp")             \
    X(ITEM_LOLLIPOP,          "lollipop",         "Sweet treat")       \
    X(ITEM_COMPUTER,          "computer",         "Beep boop")         \
    X(ITEM_VASE,              "vase",             "Holds flowers")     \
    X(ITEM_COMIC_BOOK,        "comic book",       "Pow! Zap!")         \
    X(ITEM_FOOTBALL,          "football",         "Throw it long")     \
    X(ITEM_AK47,              "ak47",             "Fully automatic")   \
    X(ITEM_GIRLSCOUT_COOKIES, "girlscout cookies", "Thin mints!")      \
    X(ITEM_STEAK,             "steak",            "Medium rare")       \
    X(ITEM_GAME_CONSOLE,      "game console",     "Press start")

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
