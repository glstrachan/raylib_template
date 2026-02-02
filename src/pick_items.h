#pragma once

#include "game.h"

#define CSTR_TO_STRING(str) (_Generic((str), string : (str), default: lit(str)))

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
    // X(ITEM_VACUUM,   "A", "")                     \
    // X(ITEM_BOOKS,    "B", "")                     \
    // X(ITEM_KNIVES,   "C", "")                     \
    // X(ITEM_LOLLIPOP, "D", "")                     \
    // X(ITEM_COMPUTER, "E", "")                     \
    // X(ITEM_VACUUM,   "F", "")                     \
    // X(ITEM_BOOKS,    "G",  "")     

#define X(id, name, desc) id,
Enum(Item_Type, uint32_t,
    ITEM_LIST(X)
    ITEM_COUNT
);
#undef X

Item_Data item_data[] = {
    #define X(id, name, desc) [id] = { CSTR_TO_STRING(name), CSTR_TO_STRING(desc), (Texture2D) {} },
    ITEM_LIST(X)
    #undef X
};

void items_init() {
    for(Item_Type item = 0; item < ITEM_COUNT; item++) {
        Oc_String_Builder builder;
        oc_sb_init(&builder, &arena);
        wprint(&builder.writer, "resources/{}", item_data[item].name);
        string texture_path = oc_sb_to_string(&builder);

        item_data[item].texture = LoadTexture(texture_path.ptr);
    }
}

static Texture2D shop_bg_tex;

typedef struct {
    // Persistent item pool
    Item_Type remaining_inventory[24];
    // Set of items that can be picked
    Item_Type pickable[6];
    // Set of items that are in the briefcase
    Item_Type picked[4];
} Pick_Items_Data;
static Pick_Items_Data data;

void pick_items_init() {
    shop_bg_tex = LoadTexture("resources/background_shop.png");

    // Initialize item pool to have 2 of each item
    for(int32_t i = 0; i < 2; i++) {
        for(Item_Type item = 0; item < ITEM_COUNT; item++) {
            data.remaining_inventory[i * item] = item;
        }
    }

    // Shuffling algorithm
    for(int32_t i = ITEM_COUNT * 2 - 1; i >= 0; i--) {
        uint32_t j = GetRandomValue(0, i);
        Item_Type temp = data.remaining_inventory[j];
        data.remaining_inventory[j] = data.remaining_inventory[i];
        data.remaining_inventory[i] = temp;
    }
}

void pick_items_cleanup() {
    UnloadTexture(shop_bg_tex);
}

// Picks 6 items from the remaining inventory
// and puts them in the pickable_items
void choose_pickable() {
    for(uint32_t i = 0; i < 6; i++) {
        data.pickable[i] = data.remaining_inventory[game.current_day * 6 + i];
    }
}

void pick_items_update() {
    DrawTexture(shop_bg_tex, 0, 0, WHITE);
    // Draw the set of items on the shelf
    // Draw the set of items in the briefcase
    // Draw extra info
    
}