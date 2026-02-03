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
    DrawTexture(shelf_tex, 400, 200, WHITE);
    // Draw the set of items in the briefcase
    DrawTexture(briefcase_tex, 1100, 430, WHITE);
    // Draw extra info
    CLAY(CLAY_ID("PickItemsTopBox"), {
        .floating = { .offset = {0, 0}, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_TOP, CLAY_ATTACH_POINT_CENTER_TOP } },
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_PERCENT(1.0),
                .height = CLAY_SIZING_PERCENT(0.125)
            },
        },
        .custom = { .customData = make_cool_background() },
        .backgroundColor = {47, 47, 47, 255},
    }) {
        CLAY(CLAY_ID("PickItemsLeft"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(),
                    .height = CLAY_SIZING_PERCENT(1.0)
                },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = {0, 200, 0, 0},
        }) { CLAY_TEXT((CLAY_STRING("Day 3/4")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} })); }
        CLAY(CLAY_ID("PickItemsCenter"), {
            .layout = {
                .childGap = 16,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(0.5),
                    .height = CLAY_SIZING_PERCENT(1.0)
                },
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {
            CLAY(CLAY_ID("PickItemsSold"), {
                .layout = {
                    .childGap = 16,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                    .sizing = {
                        .width = CLAY_SIZING_GROW(),
                        .height = CLAY_SIZING_PERCENT(1.0)
                    },
                },
                .backgroundColor = {200, 0, 200, 0},
            }) {
                CLAY_TEXT((CLAY_STRING("Items Sold")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} }));
                CLAY(CLAY_ID("PickItemsSoldCount"), {
                    .layout = {
                        .padding = {10, 10, 0, 0},
                    },
                    .backgroundColor = {77, 107, 226, 255},
                    .cornerRadius = CLAY_CORNER_RADIUS(6),
                }) { CLAY_TEXT((CLAY_STRING("8")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} })); };
            }
            CLAY(CLAY_ID("PickItemsQuota"), {
                .layout = {
                    .childGap = 16,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                    .sizing = {
                        .width = CLAY_SIZING_GROW(),
                        .height = CLAY_SIZING_PERCENT(1.0)
                    },
                },
                .backgroundColor = {200, 0, 200, 0},
            }) {
                CLAY_TEXT((CLAY_STRING("4 Day Item Quota")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} }));
                CLAY(CLAY_ID("PickItemsQuotaCount"), {
                    .layout = {
                        .padding = {10, 10, 0, 0},
                    },
                    .backgroundColor = {77, 107, 226, 255},
                    .cornerRadius = CLAY_CORNER_RADIUS(6),
                }) { CLAY_TEXT((CLAY_STRING("12")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} })); };
            }
        }
        CLAY(CLAY_ID("PickItemsRight"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(),
                    .height = CLAY_SIZING_PERCENT(1.0)
                },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = {0, 0, 200, 0},
        }) {
            CLAY(CLAY_ID("PickItemsStartDay"), {
                .layout = {
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                    .padding = { .left = 20, .right = 20, .top = 10, .bottom = 10 },
                },
                .backgroundColor = {200, 51, 0, 255},
                .custom = { .customData = make_cool_background(.color1 = { 244.0f, 51.0f, 0.0f, 255.0f }, .color2 = { 252.0f, 51.0f, 0.0f }) },
                .cornerRadius = CLAY_CORNER_RADIUS(40),
                .border = { .width = { 3, 3, 3, 3, 0 }, .color = {148, 31, 0, 255} },
            }) { CLAY_TEXT((CLAY_STRING("Start Day")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = 2, .textColor = {255, 255, 255, 255} })); }
        }
    }
}