#include "raylib.h"
#include "rlgl.h"
#include "game.h"

#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.h"

#include "pick_items.h"

static Texture2D shop_bg_tex;
static Texture2D shelf_tex;
static Texture2D briefcase_tex;

typedef struct {
    // Persistent item pool
    Item_Type remaining_inventory[24];
    // Set of items that can be picked
    Item_Type pickable[6];
    // Set of items that are in the briefcase
    Item_Type picked[4];
} Pick_Items_Data;
static Pick_Items_Data data;

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

void pick_items_init() {
    shop_bg_tex = LoadTexture("resources/background_shop.png");
    shelf_tex = LoadTexture("resources/shelf.png");
    briefcase_tex = LoadTexture("resources/briefcase.png");

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
    UnloadTexture(shelf_tex);
    UnloadTexture(briefcase_tex);
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
    DrawTexture(shelf_tex, 200, (1080 - shelf_tex.height) / 2, WHITE);
    // Draw the set of items in the briefcase
    DrawTexture(briefcase_tex, 900, (1080 - briefcase_tex.height) / 2, WHITE);
    // Draw extra info
    CLAY(CLAY_ID("PickItemsTopBox"), {
        .floating = { .offset = {0, 0}, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_TOP, CLAY_ATTACH_POINT_CENTER_TOP } },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_PERCENT(1.0),
                .height = CLAY_SIZING_PERCENT(0.18)
            },
            .padding = {16, 16, 20, 10},
            .childGap = 16
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .cornerRadius = CLAY_CORNER_RADIUS(16)
    }) {}
}