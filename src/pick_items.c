#include "raylib.h"
#include "rlgl.h"
#include "game.h"

#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.h"

#include "pick_items.h"

static Texture2D shop_bg_tex;
static Texture2D shelf_tex;
static Texture2D briefcase_tex;
static Shader item_bg_shader;

typedef struct {
    // Persistent item pool
    Item_Type remaining_inventory[24];
    // Set of items that can be picked
    Item_Type pickable[6];
    // Set of items that are in the briefcase
    Item_Type picked[4];
} Pick_Items_Data;

static Pick_Items_Data data = {0};

Item_Data item_data[] = {
    #define X(id, name, desc) [id] = { CSTR_TO_STRING(name), CSTR_TO_STRING(desc), (Texture2D) {} },
    ITEM_LIST(X)
    #undef X
};

void items_init() {
    for(Item_Type item = 1; item < ITEM_COUNT; item++) {
        Oc_String_Builder builder;
        oc_sb_init(&builder, &arena);
        wprint(&builder.writer, "resources/{}.png", item_data[item].name);
        string texture_path = oc_sb_to_string(&builder);

        item_data[item].texture = LoadTexture(texture_path.ptr);
        SetTextureFilter(item_data[item].texture, TEXTURE_FILTER_BILINEAR);
    }
}

void items_cleanup() {
    for(Item_Type item = 0; item < ITEM_COUNT; item++) {
        UnloadTexture(item_data[item].texture);
    }
}

void pick_items_init() {
    shop_bg_tex = LoadTexture("resources/background_shop.png");
    shelf_tex = LoadTexture("resources/shelf.png");
    briefcase_tex = LoadTexture("resources/briefcase.png");
    item_bg_shader = LoadShader(NULL, "resources/item_bg_shader.fs");

    // Initialize item pool to have 2 of each item
    for(int32_t i = 0; i < 2; i++) {
        for(Item_Type item = 1; item < ITEM_COUNT; item++) {
            data.remaining_inventory[i * (ITEM_COUNT - 1) + (item - 1)] = item;
        }
    }

    // TODO: Verify that the above loop actually double initializes stuff

    // Shuffling algorithm
    for(int32_t i = (ITEM_COUNT - 1) * 2 - 1; i >= 0; i--) {
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
// Also sets up the briefcase

void choose_pickable() {
    for(uint32_t i = 0; i < 6; i++) {
        data.pickable[i] = data.remaining_inventory[game.current_day * 6 + i];
    }

    for(uint32_t i = 0; i < 4; i++) {
        data.picked[i] = ITEM_NONE;
    }
}

static Vector2 item_locations[10] = {
    // Shelf
    (Vector2) {610, 390},
    (Vector2) {750, 380},
    (Vector2) {610, 605},
    (Vector2) {780, 580},
    (Vector2) {600, 805},
    (Vector2) {780, 770},

    // Briefcase
    (Vector2) {1220, 805},
    (Vector2) {1460, 830},
    (Vector2) {1220, 670},
    (Vector2) {1480, 700}
};

static struct {
    bool is_selecting;
    uint32_t selected_index;
    Vector2 initial_location;
} selection_data;

void pick_items_update() {
    /* Code to draw top bar*/

    DrawTexture(shop_bg_tex, 0, 0, WHITE);
    // Draw the set of items on the shelf
    DrawTexture(shelf_tex, 400, 200, WHITE);
    // Draw the set of items in the briefcase
    DrawTexture(briefcase_tex, 1100, 430, WHITE);

    Oc_String_Builder builder;
    oc_sb_init(&builder, &arena);
    wprint(&builder.writer, "Day {}/4", game.current_day);
    string day_string = oc_sb_to_string(&builder);

    oc_sb_init(&builder, &arena);
    wprint(&builder.writer, "{}", game.prev_items_sold.count);
    string items_sold_string = oc_sb_to_string(&builder);

    // Draw extra info
    // retarted_clay_renderer() {
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
            }) { CLAY_TEXT(((Clay_String) { .length = day_string.len, .chars = day_string.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} })); }
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
                    CLAY_TEXT((CLAY_STRING("Items Sold")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
                    CLAY(CLAY_ID("PickItemsSoldCount"), {
                        .layout = {
                            .padding = {10, 10, 0, 0},
                        },
                        .backgroundColor = {77, 107, 226, 255},
                        .cornerRadius = CLAY_CORNER_RADIUS(6),
                    }) { CLAY_TEXT(((Clay_String) { .length = items_sold_string.len, .chars = items_sold_string.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} })); };
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
                    CLAY_TEXT((CLAY_STRING("4 Day Item Quota")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
                    CLAY(CLAY_ID("PickItemsQuotaCount"), {
                        .layout = {
                            .padding = {10, 10, 0, 0},
                        },
                        .backgroundColor = {77, 107, 226, 255},
                        .cornerRadius = CLAY_CORNER_RADIUS(6),
                    }) { CLAY_TEXT((CLAY_STRING("12")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} })); };
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
                    .custom = {
                        .customData = Clay_Hovered() ?
                            make_cool_background(.color1 = { 162, 32, 0, 255 }, .color2 = { 169, 33, 0, 255 }) :
                            make_cool_background(.color1 = { 244, 51, 0, 255 }, .color2 = { 252, 51, 0, 255 })
                    },
                    .cornerRadius = CLAY_CORNER_RADIUS(40),
                    .border = Clay_Hovered() ? (Clay_BorderElementConfig) { .width = { 3, 3, 3, 3, 0 }, .color = {113, 24, 0, 255} } : (Clay_BorderElementConfig) { .width = { 3, 3, 3, 3, 0 }, .color = {148, 31, 0, 255} }, // TODO: Make this consistent for other parts of UI
                }) {
                    CLAY_TEXT((CLAY_STRING("Start Day")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
                    if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { 
                        // TODO: Make sure that 4 items were selected
                        game_go_to_state(GAME_STATE_SELECT_ENCOUNTER);
                    }
                }
            }
        }
    // }

    /* Code to handle dragging and associated bs */

    #define PICKED_PICKABLE(i) (*((i) < 6 ? &data.pickable[i] : &data.picked[(i) - 6]))

    float selection_radius = 65.0f;
    Vector2 mouse = (Vector2) { (float)GetMouseX(), (float)GetMouseY() };

    for(int i = 0; i < 10; i++) {
        DrawCircle(item_locations[i].x, item_locations[i].y, selection_radius, (Color) {255, 0, 255, 100});
    }

    for(uint32_t i = 0; i < 6; i++) {
        if(PICKED_PICKABLE(i) == ITEM_NONE) continue;

        Texture2D texture = item_data[PICKED_PICKABLE(i)].texture;
        float x = item_locations[i].x - texture.width * 0.5;
        float y = item_locations[i].y - texture.height * 0.5;

    }

    for(uint32_t i = 0; i < 10; i++) {
        if(PICKED_PICKABLE(i) == ITEM_NONE) continue;

        Texture2D texture = item_data[PICKED_PICKABLE(i)].texture;
        float x = item_locations[i].x - texture.width * 0.5;
        float y = item_locations[i].y - texture.height * 0.5;

        // Check if this particular item is being selected
        if(selection_data.is_selecting && selection_data.selected_index == i) {
            continue;
            Vector2 delta = Vector2Subtract(mouse, selection_data.initial_location);
            x += delta.x;
            y += delta.y;
        }

        DrawTexture(texture, x, y, WHITE);
    }

    if (selection_data.is_selecting) {
        Texture2D texture = item_data[PICKED_PICKABLE(selection_data.selected_index)].texture;

        Vector2 delta = Vector2Subtract(mouse, selection_data.initial_location);
        float x = delta.x + item_locations[selection_data.selected_index].x - texture.width * 0.5;
        float y = delta.y + item_locations[selection_data.selected_index].y - texture.height * 0.5;

        DrawTexture(texture, x, y, WHITE);
    }

    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for(uint32_t i = 0; i < 10; i++) {
            float dist = Vector2Distance(mouse, item_locations[i]);

            if(dist < selection_radius && PICKED_PICKABLE(i) != ITEM_NONE) {
                selection_data.is_selecting = true;
                selection_data.selected_index = i;
                selection_data.initial_location = mouse;
                break;
            }
        }
    }

    string name = item_data[PICKED_PICKABLE(selection_data.selected_index)].name;
    string description = item_data[PICKED_PICKABLE(selection_data.selected_index)].description;

    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && selection_data.is_selecting) {
        CLAY(CLAY_ID("PickItemsInfo"), {
            .floating = { .offset = {420, 210}, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_TOP, CLAY_ATTACH_POINT_CENTER_TOP } },
            .layout = {
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_FIXED(500),
                    .height = CLAY_SIZING_FIT()
                },
                .padding = {20, 20, 10, 20}
            },
            .custom = { .customData = make_cool_background() },
            .backgroundColor = {255, 47, 47, 255},
            .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
            .cornerRadius = CLAY_CORNER_RADIUS(16)
        }) {
            CLAY_TEXT(((Clay_String) { .length = name.len, .chars = name.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
            CLAY_TEXT(((Clay_String) { .length = description.len, .chars = description.ptr }), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
            
        }
    }

    if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if(selection_data.is_selecting) {
            // Check if were close enough to another item to swap
            for(uint32_t i = 0; i < 10; i++) {
                float dist = Vector2Distance(mouse, item_locations[i]);

                if(dist < selection_radius) {
                    Item_Type temp = PICKED_PICKABLE(i);
                    PICKED_PICKABLE(i) = PICKED_PICKABLE(selection_data.selected_index);
                    PICKED_PICKABLE(selection_data.selected_index) = temp;
                }
            }
        }

        selection_data.is_selecting = false;
    }
}