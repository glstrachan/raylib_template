#pragma once

#include "game.h"

#define CSTR_TO_STRING(str) (_Generic((str), string : (str), default: lit(str)))
#define STRING_TO_CSTR(str) ({                                \
    char* cstr = malloc(sizeof(char) * ((str).len + 1));      \
    memcpy(cstr, (str).ptr, (str).len);                       \
    cstr[(str).len] = '\0';                                   \
    cstr;                                                     \
})

typedef struct {
    string name;
    string description;
    Texture2D texture;
} Item_Data;

#define ITEM_LIST \
    X(ITEM_VACUUM,   "Vacuum",   "Sucks up dirt") \
    X(ITEM_BOOKS,    "Books",    "Knowledge")     \
    X(ITEM_KNIVES,   "Knives",   "Sharp")         \
    X(ITEM_LOLLIPOP, "Lollipop", "Sweet treat")   \
    X(ITEM_COMPUTER, "Computer", "Beep boop")

typedef enum {
    #define X(id, name, desc) id,
    ITEM_LIST
    #undef X
    ITEM_COUNT
} Item_Type;

Item_Data item_data[] = {
    #define X(id, name, desc) [id] = { CSTR_TO_STRING(name), CSTR_TO_STRING(desc), (Texture2D) {} },
    ITEM_LIST
    #undef X
};

void items_init() {
    for(Item_Type item = 0; item < ITEM_COUNT; item++) {
        Oc_Arena_Save save = oc_arena_save(&arena);

        Oc_String_Builder builder;
        oc_sb_init(&builder, &arena);
        wprint(&builder.writer, "resources/");
        oc_sb_append_string(&builder, item_data[item].name);
        string texture_path = oc_sb_to_string(&builder);

        oc_arena_restore(&arena, save);

        item_data[item].texture = LoadTexture(STRING_TO_CSTR(texture_path));
    }
}

static Texture2D shop_bg_tex;

void pick_items_init() {
    shop_bg_tex = LoadTexture("resources/background_shop.png");
}

void pick_items_cleanup() {
    UnloadTexture(shop_bg_tex);
}

typedef struct {
    // Need data pertaining to
    // Set of items that can be picked
    // Set of items that are in the briefcase
} Pick_Items_Data;

void pick_items_update() {
    DrawTexture(shop_bg_tex, 0, 0, WHITE);
    // Draw the set of items on the shelf
    // Draw the set of items in the briefcase
    // Draw extra info
    
}