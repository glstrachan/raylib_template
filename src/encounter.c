#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.h"

#include "encounter.h"
#include "jump.h"
#include "pick_items.h"

Encounter_Sequence encounter_top_sequence = { 0 };
static Encounter_Fn current_encounter;

void encounter_start(Encounter_Fn encounter) {
    current_encounter = encounter;
    encounter_sequence_start(&encounter_top_sequence, encounter);
}

void encounter_update(void) {
    if (!current_encounter) return;
    encounter_sequence_update(&encounter_top_sequence);
}

void encounter_sequence_start(Encounter_Sequence* sequence, Encounter_Fn encounter) {
    if (!sequence->stack) {
        sequence->stack = malloc(10 * 1024);
    }
    sequence->encounter = encounter;
    sequence->first_time = true;

    uintptr_t top_of_stack = oc_align_forward(((uintptr_t)sequence->stack) + 10 * 1024 - 16, 16);

    static Encounter_Sequence* this_sequence;
    this_sequence = sequence; // needed since we modify rsp
    asm volatile("mov %%rsp, %0" : "=r" (this_sequence->old_stack));
    asm volatile("mov %0, %%rsp" :: "r" (top_of_stack));
    if (my_setjmp(this_sequence->jump_back_buf) == 0) {
        this_sequence->encounter();
    }
    asm volatile("mov %0, %%rsp" :: "r" (this_sequence->old_stack));
}

void encounter_sequence_update(Encounter_Sequence* sequence) {
    asm volatile("mov %%rsp, %0" : "=r" (sequence->old_stack));
    if (my_setjmp(sequence->jump_back_buf) == 0) {
        my_longjmp(sequence->jump_buf, 1);
    }
    asm volatile("mov %0, %%rsp" :: "r" (sequence->old_stack));
}

bool encounter_is_done(void) {
    return encounter_top_sequence.is_done;
}

void sample_encounter(void) {
    encounter_begin();

    // dialog_text("Old Lady", "Y'know back in my day you was either white or you was dead. You darn whippersnappers!!");
    // dialog_text("potato", "Good, you?");

    // switch (dialog_selection("Choose a Fruit", "Potato", "Cherry", "Tomato", "Apple")) {
    //     case 0: {
    //         dialog_text("Old Lady", "Wowwwww! you chose potato!");
    //     } break;
    //     case 1: {
    //         dialog_text("Old Lady", "Cherry's okay");
    //     } break;
    //     case 2: {
    //         dialog_text("Old Lady", "Oh... you chose tomato");
    //     } break;
    //     case 3: {
    //         dialog_text("Old Lady", "Apple? really?");
    //     } break;
    // }

    extern Minigame memory_game, smile_game;
    // encounter_minigame(&memory_game);

    // dialog_text("Old Lady", "Wow impressive!");
    // dialog_text("Old Lady", "Now try this");

    encounter_minigame(&smile_game);

    encounter_end();
}

Encounter sample_encounter_ = {
    .fn = sample_encounter,
    .name = lit("Old Lady"),
};

static int selected_item;

static Texture2D house_bg = { 0 };
static Texture2D briefcase_tex = { 0 };
static Shader item_bg_shader;

void pick_encounter_init(void) {
    if (!house_bg.id) house_bg = LoadTexture("resources/background.png");
    if (!briefcase_tex.id) briefcase_tex = LoadTexture("resources/briefcase.png");
    if (!item_bg_shader.id) item_bg_shader = LoadShader(NULL, "resources/item_bg_shader.fs");

    game.briefcase.items[0] = ITEM_VACUUM;
    game.briefcase.items[1] = ITEM_COMPUTER;
    game.briefcase.items[2] = ITEM_BOOKS;
    game.briefcase.items[3] = ITEM_KNIVES;
}

void pick_encounter(void) {
    game.current_character = CHARACTERS_OLDLADY;
    // game.encounter = &sample_encounter_;
    selected_item = -1;
}

static Vector2 pick_item_locations[] = {
    // Briefcase
    (Vector2) {1220, 805},
    (Vector2) {1460, 830},
    (Vector2) {1220, 670},
    (Vector2) {1480, 700}
};

Encounter_Fn get_encounter_fn(void);

void pick_encounter_update(void) {
    DrawTexture(house_bg, 0, 0, WHITE);

    // DrawTexture(briefcase_tex, 1100, 430, WHITE);
    const float item_location_offset_x = -400.0f;
    const float item_location_offset_y = -120.0f;


    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = (Vector2) { (float)GetMouseX(), (float)GetMouseY() };
        float selection_radius = 65.0f;

        for (uint32_t i = 0; i < oc_len(pick_item_locations); ++i) {
            Vector2 loc = { item_location_offset_x, item_location_offset_y };
            float dist = Vector2Distance(mouse, Vector2Add(pick_item_locations[i], loc));

            if (dist < selection_radius) {
                selected_item = i;
                break;
            }
        }
    }

    CLAY(CLAY_ID("PickEncounter"), {
        .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER } },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_PERCENT(0.5),
                // .height = CLAY_SIZING_PERCENT(0.6)
            },
            .padding = {40, 16, 30, 16},
            .childGap = 16
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .custom = { .customData = make_cool_background() },
        .cornerRadius = CLAY_CORNER_RADIUS(16)
    }) {
        CLAY_TEXT(oc_format(&frame_arena, "Selling to {}", character_data[game.current_character].name), CLAY_TEXT_CONFIG({ .fontSize = 61, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
        CLAY_TEXT(oc_format(&frame_arena, "Pick item to sell"), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));

        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER }
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(552),
                        .height = CLAY_SIZING_FIXED(556),
                    },
                },
                .image = { .imageData = &briefcase_tex },
            }) {
                for (uint32_t i = 0; i < oc_len(game.briefcase.items); i++) {
                    if (game.briefcase.used[i]) continue;
                    if (i == selected_item) continue;

                    Item_Type item_type = game.briefcase.items[i];
                    Texture2D texture = item_data[item_type].texture;
                    float x = pick_item_locations[i].x - texture.width * 0.5 + item_location_offset_x;
                    float y = pick_item_locations[i].y - texture.height * 0.5 + item_location_offset_y;

                    DrawTexture(texture, x, y, WHITE);
                }

                if (selected_item > -1) {
                    Item_Type item_type = game.briefcase.items[selected_item];
                    Texture2D texture = item_data[item_type].texture;
                    float x = pick_item_locations[selected_item].x - texture.width * 0.5 + item_location_offset_x;
                    float y = pick_item_locations[selected_item].y - texture.height * 0.5 + item_location_offset_y;

                    BeginShaderMode(item_bg_shader);
                        DrawTexturePro(texture, (Rectangle) { 0, 0, texture.width, texture.height }, (Rectangle) { x - 4, y - 4, texture.width + 8, texture.width + 8 }, (Vector2) { 0.0f, 0.0f }, 0, WHITE);
                    EndShaderMode();
                }
            }
        }
        CLAY(CLAY_ID("PickEncounterLower"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                },
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_BOTTOM }
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {
            CLAY(CLAY_ID("PickEncounterStartSelling"), {
                .layout = {
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                    .padding = { .left = 20, .right = 20, .top = 10, .bottom = 10 },
                },
                .custom = {
                    .customData = Clay_Hovered() ?
                        make_cool_background(.color1 = { 214, 51, 0, 255 }, .color2 = { 222, 51, 0, 255 }) :
                        make_cool_background(.color1 = { 244, 51, 0, 255 }, .color2 = { 252, 51, 0, 255 })
                },
                .cornerRadius = CLAY_CORNER_RADIUS(10000),
                .border = { .width = { 3, 3, 3, 3, 0 }, .color = {148, 31, 0, 255} },
            }) {
                CLAY_TEXT((CLAY_STRING("Start Selling")), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
                if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && selected_item != -1) {
                    // extern Encounter sample_encounter_;
                    // game.encounter = &sample_encounter_;
                    game.current_item = game.briefcase.items[selected_item];
                    game.encounter = get_encounter_fn();
                    game_go_to_state(GAME_STATE_IN_ENCOUNTER);
                }
            }
        }
    }
}

void day_summary_update(void) {
    DrawTexture(house_bg, 0, 0, WHITE);

    int sold = 0;
    for (int i = 0; i < 4 && game.items_sold_today[i].item != ITEM_NONE; ++i, ++sold);
    CLAY(CLAY_ID("DaySummary"), {
        .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER } },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_PERCENT(0.5),
                .height = CLAY_SIZING_PERCENT(0.6)
            },
            .padding = {40, 16, 30, 16},
            .childGap = 16
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .custom = { .customData = make_cool_background() },
        .cornerRadius = CLAY_CORNER_RADIUS(16)
    }) {
        CLAY_TEXT(oc_format(&frame_arena, "Day {} Summary", game.current_day), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
        CLAY_TEXT(oc_format(&frame_arena, "Sold {} items today", sold), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
        CLAY(CLAY_ID("ItemsList"), {
            .layout = { .childGap = 16, .layoutDirection = CLAY_TOP_TO_BOTTOM, .padding = {50} },
        }) {
            for (int i = 0; i < 4; ++i) {
                if (game.items_sold_today[i].item == ITEM_NONE) break;
                Item_Data* data = &item_data[game.items_sold_today[i].item];

                CLAY_AUTO_ID({
                    .layout = { .childGap = 16, .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }, .padding = {16, 16, 0, 0} },
                    .backgroundColor = {100, 100, 100, 0},
                }) {
                    CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_FIXED(100), .height = CLAY_SIZING_FIXED(100) } }, .image = { .imageData = &data->texture } });
                    CLAY_TEXT(data->name, CLAY_TEXT_CONFIG({ .fontSize = 35, .fontId = FONT_ROBOTO, .textColor = {255, 255, 255, 255} }));
                }
            }
        }

        CLAY_AUTO_ID({
            .layout = { .sizing = { .height = CLAY_SIZING_GROW() } }
        });
        CLAY(CLAY_ID("DialogContinue"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                    .height = CLAY_SIZING_FIXED(100)
                },
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_BOTTOM }
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {
            CLAY(CLAY_ID("PickItemsStartDay"), {
                .layout = {
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                    .padding = { .left = 20, .right = 20, .top = 10, .bottom = 10 },
                },
                .custom = {
                    .customData = Clay_Hovered() ?
                        make_cool_background(.color1 = { 214, 51, 0, 255 }, .color2 = { 222, 51, 0, 255 }) :
                        make_cool_background(.color1 = { 244, 51, 0, 255 }, .color2 = { 252, 51, 0, 255 })
                },
                .cornerRadius = CLAY_CORNER_RADIUS(10000),
                .border = { .width = { 3, 3, 3, 3, 0 }, .color = {148, 31, 0, 255} },
            }) {
                CLAY_TEXT((CLAY_STRING("Next Day")), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
                if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    game_go_to_state(GAME_STATE_SELECT_ITEMS);
                }
            }
        }
    }
}

const Encounter_Fn encounters[CHARACTERS_COUNT][ITEM_COUNT] = {
    [CHARACTERS_SALESMAN] = {
        [ITEM_VACUUM]   = sample_encounter,
        [ITEM_BOOKS]    = sample_encounter,
        [ITEM_KNIVES]   = sample_encounter,
        [ITEM_LOLLIPOP] = sample_encounter,
        [ITEM_COMPUTER] = sample_encounter,
    },
    [CHARACTERS_OLDLADY] = {
        [ITEM_VACUUM]   = sample_encounter,
        [ITEM_BOOKS]    = sample_encounter,
        [ITEM_KNIVES]   = sample_encounter,
        [ITEM_LOLLIPOP] = sample_encounter,
        [ITEM_COMPUTER] = sample_encounter,
    },
    [CHARACTERS_NERD] = {
        [ITEM_VACUUM]   = sample_encounter,
        [ITEM_BOOKS]    = sample_encounter,
        [ITEM_KNIVES]   = sample_encounter,
        [ITEM_LOLLIPOP] = sample_encounter,
        [ITEM_COMPUTER] = sample_encounter,
    },
    [CHARACTERS_SHOTGUNNER] = {
        [ITEM_VACUUM]   = sample_encounter,
        [ITEM_BOOKS]    = sample_encounter,
        [ITEM_KNIVES]   = sample_encounter,
        [ITEM_LOLLIPOP] = sample_encounter,
        [ITEM_COMPUTER] = sample_encounter,
    },
    [CHARACTERS_FATMAN] = {
        [ITEM_VACUUM]   = sample_encounter,
        [ITEM_BOOKS]    = sample_encounter,
        [ITEM_KNIVES]   = sample_encounter,
        [ITEM_LOLLIPOP] = sample_encounter,
        [ITEM_COMPUTER] = sample_encounter,
    },
};

Encounter_Fn get_encounter_fn(void) {
    return encounters[game.current_character][game.current_item];
}
