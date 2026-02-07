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
    const int STACK_SIZE = 10 * 1024 * 1024;
    if (!sequence->stack) {
        sequence->stack = malloc(STACK_SIZE);
    }
    sequence->encounter = encounter;
    sequence->first_time = true;

    uintptr_t top_of_stack = oc_align_forward(((uintptr_t)sequence->stack) + STACK_SIZE - 16, 16);

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
    static Encounter_Sequence* static_sequence;
    static_sequence = sequence;
    asm volatile("mov %%rsp, %0" : "=r" (static_sequence->old_stack));
    if (my_setjmp(static_sequence->jump_back_buf) == 0) {
        my_longjmp(static_sequence->jump_buf, 1);
    }
    asm volatile("mov %0, %%rsp" :: "r" (static_sequence->old_stack) : "rsp");
}

bool encounter_is_done(void) {
    return encounter_top_sequence.is_done;
}

extern Minigame memory_game, smile_game, shotgun_game, rhythm_game;

static int selected_item;

static Texture2D house_bg = { 0 };
static Texture2D briefcase_tex = { 0 };
static Texture2D clipboard_text = { 0 };
static Shader item_bg_shader;
static Shader circle_shader;

void pick_encounter_init(void) {
    if (!house_bg.id) house_bg = LoadTexture("resources/background.png");
    if (!briefcase_tex.id) briefcase_tex = LoadTexture("resources/briefcase.png");
    if (!clipboard_text.id) clipboard_text = LoadTexture("resources/clipboard.png");
    if (!item_bg_shader.id) item_bg_shader = LoadShader(NULL, "resources/item_bg_shader.fs");
    if (!circle_shader.id) circle_shader = LoadShader(NULL, "resources/circle_texture.fs");
}

void pick_encounter(void) {
    game.current_character = CHARACTERS_FATMAN;
    // game.encounter = &sample_encounter_;
    selected_item = -1;
}

// static Vector2 pick_item_locations[] = {
//     // Briefcase
//     (Vector2) {1220, 805},
//     (Vector2) {1460, 830},
//     (Vector2) {1220, 670},
//     (Vector2) {1480, 700}
// };
static Vector2 pick_item_locations[] = {
    // Briefcase
    (Vector2) {1220, 860},
    (Vector2) {1320, 860},
    (Vector2) {1420, 860},
    (Vector2) {1520, 860}
};

Encounter_Fn get_encounter_fn(void);

void pick_encounter_update(void) {
    DrawTexture(house_bg, 0, 0, WHITE);

    const float item_location_offset_x = -400.0f;
    const float item_location_offset_y = -120.0f;


    Texture2D* t = characters_get_texture(game.current_character);
    CustomLayoutElement* customBackgroundData = oc_arena_alloc(&frame_arena, sizeof(CustomLayoutElement));
    customBackgroundData->type = CUSTOM_LAYOUT_ELEMENT_TYPE_BACKGROUND;
    customBackgroundData->customData.background = (CustomLayoutElement_Background) { .shader = circle_shader, .texture = *t };

    CLAY(CLAY_ID("PickEncounter"), {
        .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER } },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_FIXED(clipboard_text.width),
                .height = CLAY_SIZING_FIXED(clipboard_text.height),
            },
            .padding = {70, 50, 200, 60},
            .childGap = 16,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER },
        },
        .image = { .imageData = &clipboard_text },
        .cornerRadius = CLAY_CORNER_RADIUS(16)
    }) {
        CLAY_TEXT(oc_format(&frame_arena, "Selling to {}", character_data[game.current_character].name), CLAY_TEXT_CONFIG({ .fontSize = 61, .fontId = FONT_ITIM, .textColor = {0, 0, 0, 255} }));

        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER },
            },
        }) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(t->width),
                        .height = CLAY_SIZING_FIXED(t->width),
                    },
                },
                .custom = { .customData = customBackgroundData },
                .cornerRadius = CLAY_CORNER_RADIUS(1000),
                .border = { .width = { 5, 5, 5, 5, 0 }, .color = {55, 55, 55, 255} },
            });
        }
        CLAY_AUTO_ID({ .layout = { .sizing = { .height = CLAY_SIZING_FIXED(10) } } });


        CLAY_TEXT(oc_format(&frame_arena, "Pick item to sell"), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {0, 0, 0, 255} }));
        


        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_PERCENT(1.0),
                },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER }
            },
            .backgroundColor = {200, 0, 0, 0},
        }) {
            float base_x = 770.0f;
            float base_y = 680.0f;
            Vector2 mouse = (Vector2) { (float)GetMouseX(), (float)GetMouseY() };
            float selection_radius = 65.0f;

            if (selected_item > -1) {
                Item_Type item_type = game.briefcase.items[selected_item];
                Texture2D texture = item_data[item_type].texture;

                BeginShaderMode(item_bg_shader);
                    static int i = 4;
                    SetShaderValue(item_bg_shader, GetShaderLocation(item_bg_shader, "thickness"), &i, SHADER_UNIFORM_INT);
                    DrawTexturePro(texture, (Rectangle) { 0, 0, texture.width, texture.height }, (Rectangle) { base_x + selected_item * 100.0f - 4.0f, base_y - 4.0f, 108, 108 }, (Vector2) { 0.0f, 0.0f }, 0, RED);
                EndShaderMode();
            }

            for (uint32_t i = 0; i < oc_len(game.briefcase.items); i++) {
                if (game.briefcase.used[i]) continue;
                if (i == selected_item) continue;

                Item_Type item_type = game.briefcase.items[i];
                Texture2D texture = item_data[item_type].texture;

                DrawTexturePro(texture, (Rectangle) { 0, 0, texture.width, texture.height }, (Rectangle) { base_x + i * 100.0f, base_y, 100, 100 }, (Vector2) { 0.0f, 0.0f }, 0, WHITE);


                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Vector2 loc = { base_x + i * 100.0f + 50.0f, base_y + 50.0f };
                    float dist = Vector2Distance(mouse, loc);
                    if (dist < selection_radius) {
                        selected_item = i;
                    }
                }
            }


            // for (uint32_t i = 0; i < oc_len(game.briefcase.items); i++) {
            //     if (game.briefcase.used[i]) continue;

            //     Item_Type item_type = game.briefcase.items[i];
            //     Texture2D texture = item_data[item_type].texture;

            //     CLAY_AUTO_ID({
            //         .layout = {
            //             .sizing = {
            //                 .width = CLAY_SIZING_FIXED(100),
            //                 .height = CLAY_SIZING_FIXED(100),
            //             },
            //             .padding = {10, 10, 10, 10},
            //         },
            //         .border = (i == selected_item) ? (Clay_BorderElementConfig) { .width = { 3, 3, 3, 3, 0 }, .color = {148, 31, 0, 255} } : (Clay_BorderElementConfig) { 0 },
            //         .backgroundColor = Clay_Hovered() ? (Clay_Color) { 100, 100, 100, 100 } : (Clay_Color) { 0, 0, 0, 0 },
            //     }) {
            //         CLAY_AUTO_ID({
            //             .layout = {
            //                 .sizing = {
            //                     .width = CLAY_SIZING_GROW(0),
            //                     .height = CLAY_SIZING_GROW(0),
            //                 },
            //             },
            //             .image = { .imageData = &item_data[item_type].texture }
            //         })
            //         if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            //             selected_item = i;
            //         }
            //     }
            // }
        }

        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    .height = CLAY_SIZING_GROW(),
                },
            },
        });


        // CLAY_AUTO_ID({
        //     .layout = {
        //         .sizing = {
        //             .width = CLAY_SIZING_PERCENT(1.0),
        //         },
        //         .childAlignment = { .x = CLAY_ALIGN_X_CENTER }
        //     },
        //     .backgroundColor = {200, 0, 0, 0},
        // }) {
        //     CLAY_AUTO_ID({
        //         .layout = {
        //             .sizing = {
        //                 .width = CLAY_SIZING_FIXED(552),
        //                 .height = CLAY_SIZING_FIXED(556),
        //             },
        //         },
        //         .image = { .imageData = &briefcase_tex },
        //     }) {
        //         for (uint32_t i = 0; i < oc_len(game.briefcase.items); i++) {
        //             if (game.briefcase.used[i]) continue;
        //             if (i == selected_item) continue;

        //             Item_Type item_type = game.briefcase.items[i];
        //             Texture2D texture = item_data[item_type].texture;
        //             float x = pick_item_locations[i].x - texture.width * 0.5 + item_location_offset_x;
        //             float y = pick_item_locations[i].y - texture.height * 0.5 + item_location_offset_y;

        //             DrawTexture(texture, x, y, WHITE);
        //         }

        //         if (selected_item > -1) {
        //             Item_Type item_type = game.briefcase.items[selected_item];
        //             Texture2D texture = item_data[item_type].texture;
        //             float x = pick_item_locations[selected_item].x - texture.width * 0.5 + item_location_offset_x;
        //             float y = pick_item_locations[selected_item].y - texture.height * 0.5 + item_location_offset_y;

        //             BeginShaderMode(item_bg_shader);
        //                 DrawTexturePro(texture, (Rectangle) { 0, 0, texture.width, texture.height }, (Rectangle) { x - 4, y - 4, texture.width + 8, texture.width + 8 }, (Vector2) { 0.0f, 0.0f }, 0, WHITE);
        //             EndShaderMode();
        //         }
        //     }
        // }
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
            .layout = { .sizing = { .height = CLAY_SIZING_GROW(0) } }
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
// ============================================================================
// ENCOUNTERS - Door-to-Door Salesman Dialog System
// ============================================================================

// ============================================================================
// OLD LADY ENCOUNTERS
// ============================================================================

void encounter_oldlady_vacuum(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon, ma'am! Beautiful day, isn't it?");
    dialog_text(CHARACTERS_OLDLADY, "What? Speak up, I can't hear you through this screen door.");
    dialog_text(CHARACTERS_SALESMAN, "I said it's a BEAUTIFUL DAY!");
    dialog_text(CHARACTERS_OLDLADY, "Oh yes, it is. My husband Harold loved days like this. He's been dead for twelve years.");
    dialog_text(CHARACTERS_SALESMAN, "I'm... very sorry to hear that. But speaking of things that suck, have you considered upgrading your vacuum cleaner?");
    dialog_text(CHARACTERS_OLDLADY, "Did you just make a joke about my dead husband?");
    dialog_text(CHARACTERS_SALESMAN, "What? No! I was talking about suction! This vacuum has incredible suction power!");
    dialog_text(CHARACTERS_OLDLADY, "Harold never had problems with suction.");
    dialog_text(CHARACTERS_SALESMAN, "I'm going to pretend I didn't hear that.");
    
    switch (dialog_selection("How do you respond?", 
        dialog_options("Show her the attachments", 
        "Demonstrate the suction", 
        "Talk about the warranty",
        "Compliment her carpet"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "Let me show you these attachments. This one's for corners, this one's for upholstery...");
        dialog_text(CHARACTERS_OLDLADY, "My grandson has attachments too. To his computer. Never calls me.");
        dialog_text(CHARACTERS_SALESMAN, "That's... not the same kind of attachment, but I appreciate the segue.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "Watch this suction power! It could pick up a bowling ball!");
        dialog_text(CHARACTERS_OLDLADY, "Harold used to bowl. Did I mention he's dead?");
        dialog_text(CHARACTERS_SALESMAN, "You did mention that, yes.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "This baby comes with a lifetime warranty!");
        dialog_text(CHARACTERS_OLDLADY, "Whose lifetime? Mine or the vacuum's? Because at my age, that's an important distinction.");
        dialog_text(CHARACTERS_SALESMAN, "That's... a fair point, actually.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "I couldn't help but notice your lovely carpet. Is that shag?");
        dialog_text(CHARACTERS_OLDLADY, "It's from 1973. Like my hip.");
        dialog_text(CHARACTERS_SALESMAN, "A classic! This vacuum treats vintage carpets with the respect they deserve.");
        break;
    }
    
    dialog_text(CHARACTERS_OLDLADY, "Alright, young man. If you can clean that spot by the door, maybe I'll consider it.");
    dialog_text(CHARACTERS_SALESMAN, "Challenge accepted!");
    
    encounter_minigame(&smile_game);
    
    dialog_text(CHARACTERS_OLDLADY, "Well I'll be. You actually got that stain out. That's been there since Reagan was president.");
    dialog_text(CHARACTERS_SALESMAN, "The power of modern suction technology!");
    dialog_text(CHARACTERS_OLDLADY, "Fine. I'll take one. But only because you remind me of my grandson. If he ever visited.");
    
    encounter_end();
}

void encounter_oldlady_books(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! I'm here representing the World Knowledge Encyclopedia Company!");
    dialog_text(CHARACTERS_OLDLADY, "Encyclopedia? Honey, we have the internet now.");
    dialog_text(CHARACTERS_SALESMAN, "But ma'am, can the internet sit beautifully on your shelf and impress visitors?");
    dialog_text(CHARACTERS_OLDLADY, "What visitors? Everyone I know is dead or in Florida. Same thing, really.");
    dialog_text(CHARACTERS_SALESMAN, "Then these books can be your companions! Twenty-six volumes of human knowledge!");
    dialog_text(CHARACTERS_OLDLADY, "I already know too much. Did you know my neighbor is having an affair? With the mailman? At her age?");
    dialog_text(CHARACTERS_SALESMAN, "That's... not in these books, actually.");
    dialog_text(CHARACTERS_OLDLADY, "Then what good are they?");
    
    switch (dialog_selection("Make your pitch:", 
        dialog_options("Appeal to her grandchildren", 
        "Emphasize the leather binding", 
        "Mention the large print edition",
        "Talk about the history sections"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "Think of your grandchildren! They could use these for school projects!");
        dialog_text(CHARACTERS_OLDLADY, "My grandchildren use their phones for everything. Last Christmas, little Timmy didn't look up once. Not once!");
        dialog_text(CHARACTERS_SALESMAN, "All the more reason to give them something tangible!");
        dialog_text(CHARACTERS_OLDLADY, "I gave him a sweater. He called it 'cringe.' What does that even mean?");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "Feel this leather binding. Genuine craftsmanship!");
        dialog_text(CHARACTERS_OLDLADY, "My Harold had a leather jacket. Wore it on our first date to the sock hop.");
        dialog_text(CHARACTERS_SALESMAN, "These books will last just as long as that memory!");
        dialog_text(CHARACTERS_OLDLADY, "The jacket didn't last. He left it at a motel in Reno. I never asked why he was in Reno.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "We have a large print edition! Easy on the eyes!");
        dialog_text(CHARACTERS_OLDLADY, "Are you saying I'm old?");
        dialog_text(CHARACTERS_SALESMAN, "No! I'm saying you're... experienced! With refined tastes in font sizes!");
        dialog_text(CHARACTERS_OLDLADY, "Nice save, young man.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "The history sections are incredible! Wars, empires, the whole story of civilization!");
        dialog_text(CHARACTERS_OLDLADY, "I lived through most of it. I don't need a book to tell me about the Depression. I was there. We ate dirt.");
        dialog_text(CHARACTERS_SALESMAN, "Actual dirt?");
        dialog_text(CHARACTERS_OLDLADY, "Well, potatoes. But they tasted like dirt.");
        break;
    }
    
    dialog_text(CHARACTERS_SALESMAN, "Tell you what, let me prove these books are worth it. Quiz me on anything!");
    dialog_text(CHARACTERS_OLDLADY, "Anything? Alright then, smarty pants.");
    
    encounter_minigame(&smile_game);
    
    dialog_text(CHARACTERS_OLDLADY, "Hm. You actually knew that. Color me impressed.");
    dialog_text(CHARACTERS_SALESMAN, "All thanks to these encyclopedias!");
    dialog_text(CHARACTERS_OLDLADY, "Oh, fine. But you're carrying them inside. My back isn't what it used to be.");
    dialog_text(CHARACTERS_SALESMAN, "Of course, ma'am. That's actually included in the service.");
    dialog_text(CHARACTERS_OLDLADY, "While you're at it, there's a shelf that needs moving. And some boxes in the garage.");
    
    encounter_end();
}

void encounter_oldlady_knives(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon, ma'am! Can I interest you in the finest cutlery this side of—");
    dialog_text(CHARACTERS_OLDLADY, "Knives? You're selling knives door to door? In this economy?");
    dialog_text(CHARACTERS_SALESMAN, "These aren't just any knives! These are precision-crafted, German-engineered—");
    dialog_text(CHARACTERS_OLDLADY, "My Harold was in the war. Against the Germans. He'd roll over in his grave.");
    dialog_text(CHARACTERS_SALESMAN, "They're actually made in Ohio.");
    dialog_text(CHARACTERS_OLDLADY, "Oh, that's fine then. Harold had nothing against Ohio.");
    dialog_text(CHARACTERS_SALESMAN, "Great! So these knives can cut through anything. Tomatoes, bread, even a penny!");
    dialog_text(CHARACTERS_OLDLADY, "Why would I cut a penny? That's illegal, young man.");
    dialog_text(CHARACTERS_SALESMAN, "It's more of a demonstration of sharpness—");
    dialog_text(CHARACTERS_OLDLADY, "The government is always watching. I don't trust them.");
    
    switch (dialog_selection("Redirect the conversation:", 
        dialog_options("Show the bread knife", 
        "Demonstrate on vegetables", 
        "Talk about kitchen safety",
        "Mention the storage block"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "This bread knife cuts through crusty loaves like butter!");
        dialog_text(CHARACTERS_OLDLADY, "I do love a good sourdough. My doctor says I shouldn't have bread. I say my doctor should mind his business.");
        dialog_text(CHARACTERS_SALESMAN, "A woman who knows what she wants!");
        dialog_text(CHARACTERS_OLDLADY, "At my age, you stop caring what anyone thinks.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "Watch how cleanly it slices this tomato!");
        dialog_text(CHARACTERS_OLDLADY, "That's a nice tomato. Where'd you get it?");
        dialog_text(CHARACTERS_SALESMAN, "The grocery store?");
        dialog_text(CHARACTERS_OLDLADY, "Which one? Prices are outrageous these days. In my day, tomatoes were five cents.");
        dialog_text(CHARACTERS_SALESMAN, "Ma'am, I'm trying to show you the knife—");
        dialog_text(CHARACTERS_OLDLADY, "And you could get a whole meal for a quarter. A QUARTER!");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "These handles are ergonomically designed to prevent accidents!");
        dialog_text(CHARACTERS_OLDLADY, "Good. My hands shake sometimes. The doctor says it's nothing, but what does he know? He's twelve years old.");
        dialog_text(CHARACTERS_SALESMAN, "I'm sure he's older than—");
        dialog_text(CHARACTERS_OLDLADY, "Everyone's a child to me now. Even you. You look about eight.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "And it comes with this beautiful oak storage block!");
        dialog_text(CHARACTERS_OLDLADY, "Oak? My dining table is oak. Harold made it himself.");
        dialog_text(CHARACTERS_SALESMAN, "That's wonderful craftsmanship.");
        dialog_text(CHARACTERS_OLDLADY, "He wasn't very good at it, actually. The table wobbles. But I keep it because he's dead.");
        break;
    }
    
    dialog_text(CHARACTERS_OLDLADY, "Alright, show me what these fancy knives can really do.");
    dialog_text(CHARACTERS_SALESMAN, "Prepare to be amazed!");
    
    encounter_minigame(&smile_game);
    
    dialog_text(CHARACTERS_OLDLADY, "Well, would you look at that. Clean cuts every time.");
    dialog_text(CHARACTERS_SALESMAN, "Told you! German engineering... made in Ohio!");
    dialog_text(CHARACTERS_OLDLADY, "I'll take a set. But if they go dull, I'm coming to find you.");
    dialog_text(CHARACTERS_SALESMAN, "They have a lifetime warranty!");
    dialog_text(CHARACTERS_OLDLADY, "Again with the lifetime. You know what? At this point, I'll take those odds.");
    
    encounter_end();
}

void encounter_oldlady_lollipop(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon, ma'am! I'm selling artisanal lollipops!");
    dialog_text(CHARACTERS_OLDLADY, "Lollipops? What am I, five years old?");
    dialog_text(CHARACTERS_SALESMAN, "These aren't ordinary lollipops! They're gourmet, handcrafted—");
    dialog_text(CHARACTERS_OLDLADY, "In my day, candy cost a nickel and it wasn't 'artisanal.' It was just candy.");
    dialog_text(CHARACTERS_SALESMAN, "Times have changed! These are made with organic cane sugar and real fruit extracts!");
    dialog_text(CHARACTERS_OLDLADY, "Organic. Everything's organic now. You know what else is organic? Dirt. I don't eat dirt.");
    dialog_text(CHARACTERS_SALESMAN, "That's... a fair point. But these taste much better than dirt, I promise.");
    dialog_text(CHARACTERS_OLDLADY, "I would hope so. What flavors do you have?");
    
    switch (dialog_selection("Offer a flavor:", 
        dialog_options("Classic cherry", 
        "Lavender honey", 
        "Bourbon vanilla",
        "Green apple"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "Classic cherry! A timeless favorite!");
        dialog_text(CHARACTERS_OLDLADY, "Cherry reminds me of Harold's cough syrup. He coughed a lot near the end.");
        dialog_text(CHARACTERS_SALESMAN, "This is much more pleasant than cough syrup, I assure you.");
        dialog_text(CHARACTERS_OLDLADY, "Most things are.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "Lavender honey! Very sophisticated!");
        dialog_text(CHARACTERS_OLDLADY, "Lavender? That's soap flavor. Why would I want soap in my mouth?");
        dialog_text(CHARACTERS_SALESMAN, "It's more of a floral note—");
        dialog_text(CHARACTERS_OLDLADY, "I eat food, not notes.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "Bourbon vanilla! Very refined!");
        dialog_text(CHARACTERS_OLDLADY, "Bourbon? Now you're speaking my language.");
        dialog_text(CHARACTERS_SALESMAN, "It's just the flavoring, there's no actual alcohol—");
        dialog_text(CHARACTERS_OLDLADY, "Oh. Well, that's disappointing. My doctor says I can't drink anymore. So now I drink in secret.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "Green apple! Tart and refreshing!");
        dialog_text(CHARACTERS_OLDLADY, "I used to have an apple tree in my yard. Then the HOA made me cut it down. Fascists.");
        dialog_text(CHARACTERS_SALESMAN, "That's... I'm sorry for your loss?");
        dialog_text(CHARACTERS_OLDLADY, "The tree or my freedom? Both, I suppose.");
        break;
    }
    
    dialog_text(CHARACTERS_OLDLADY, "Fine, let me try one. But if it gets stuck in my dentures, you're paying the dental bill.");
    dialog_text(CHARACTERS_SALESMAN, "Here, I'll unwrap it for you!");
    
    encounter_minigame(&smile_game);
    
    dialog_text(CHARACTERS_OLDLADY, "Hm. That's actually not terrible.");
    dialog_text(CHARACTERS_SALESMAN, "High praise!");
    dialog_text(CHARACTERS_OLDLADY, "I'll take a dozen. My grandchildren never visit, but when they do, I'll have these. They'll go stale waiting.");
    dialog_text(CHARACTERS_SALESMAN, "They actually have a long shelf life—");
    dialog_text(CHARACTERS_OLDLADY, "Not as long as my grudge against their mother.");
    
    encounter_end();
}

void encounter_oldlady_computer(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon, ma'am! Have you considered entering the digital age?");
    dialog_text(CHARACTERS_OLDLADY, "The what now?");
    dialog_text(CHARACTERS_SALESMAN, "I'm selling personal computers! They can connect you with family, help you shop—");
    dialog_text(CHARACTERS_OLDLADY, "I don't trust those things. They have the email. That's where the scammers live.");
    dialog_text(CHARACTERS_SALESMAN, "Well, yes, but there's also so much more! Video calls with grandchildren, recipe websites—");
    dialog_text(CHARACTERS_OLDLADY, "My grandson tried to show me the YouTube once. So much shouting. Why is everyone shouting?");
    dialog_text(CHARACTERS_SALESMAN, "You can control the volume—");
    dialog_text(CHARACTERS_OLDLADY, "And everyone wants me to subscribe. Subscribe to what? I already have magazines I don't read.");
    
    switch (dialog_selection("Try a different angle:", 
        dialog_options("Emphasize family video calls", 
        "Talk about online shopping", 
        "Mention digital photo albums",
        "Discuss medical resources"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "You could see your grandchildren's faces, even if they're far away!");
        dialog_text(CHARACTERS_OLDLADY, "They're in the next town over. They're not far, they're just neglectful.");
        dialog_text(CHARACTERS_SALESMAN, "Well, this might make them more likely to call!");
        dialog_text(CHARACTERS_OLDLADY, "Nothing makes them more likely to call. I've tried guilt. I've tried bribery. Nothing works.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "You can shop from home! No driving, no crowds!");
        dialog_text(CHARACTERS_OLDLADY, "I like crowds. I like judging people's shopping carts. That woman last week had four tubs of ice cream. Four! I counted.");
        dialog_text(CHARACTERS_SALESMAN, "That's... a hobby, I suppose.");
        dialog_text(CHARACTERS_OLDLADY, "Don't judge me. I don't have television.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "You could store all your photos digitally! No more dusty albums!");
        dialog_text(CHARACTERS_OLDLADY, "I like dusty albums. Each speck of dust is a memory.");
        dialog_text(CHARACTERS_SALESMAN, "That's... poetic, actually.");
        dialog_text(CHARACTERS_OLDLADY, "I was a poet. Published three books. Nobody remembers.");
        dialog_text(CHARACTERS_SALESMAN, "I'd love to read them sometime.");
        dialog_text(CHARACTERS_OLDLADY, "That's what everyone says. No one means it.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "You can research health topics! Very useful at— I mean, for everyone!");
        dialog_text(CHARACTERS_OLDLADY, "Nice catch. At my age, is what you were going to say.");
        dialog_text(CHARACTERS_SALESMAN, "I was going to say 'at any age'—");
        dialog_text(CHARACTERS_OLDLADY, "No you weren't. But I appreciate the backpedaling. It shows character.");
        break;
    }
    
    dialog_text(CHARACTERS_OLDLADY, "Alright, show me what this contraption can do. But if it explodes, you're buying me a new house.");
    dialog_text(CHARACTERS_SALESMAN, "Computers don't typically explode—");
    dialog_text(CHARACTERS_OLDLADY, "That's what they said about my toaster. It exploded. Twice.");
    
    encounter_minigame(&smile_game);
    
    dialog_text(CHARACTERS_OLDLADY, "Well, I'll be. That wasn't as complicated as I thought.");
    dialog_text(CHARACTERS_SALESMAN, "See? The digital age awaits!");
    dialog_text(CHARACTERS_OLDLADY, "Fine. But you're setting it up. And showing me how to work it. And coming back when I forget.");
    dialog_text(CHARACTERS_SALESMAN, "That's technically not in my job description—");
    dialog_text(CHARACTERS_OLDLADY, "I'll make cookies.");
    dialog_text(CHARACTERS_SALESMAN, "I'll clear my schedule.");
    
    encounter_end();
}

// ============================================================================
// NERD ENCOUNTERS
// ============================================================================

void encounter_nerd_vacuum(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Hey there! I'm selling premium vacuum cleaners today!");
    dialog_text(CHARACTERS_NERD, "Is that a Dyson? Because the cyclonic separation technology is honestly overrated for the price point.");
    dialog_text(CHARACTERS_SALESMAN, "It's actually a different brand—");
    dialog_text(CHARACTERS_NERD, "What's the suction power in air watts? And don't give me the marketing number, I want the actual measured output.");
    dialog_text(CHARACTERS_SALESMAN, "I... don't actually know the exact—");
    dialog_text(CHARACTERS_NERD, "What about the filtration system? HEPA? And is it true HEPA or HEPA-style? Big difference.");
    dialog_text(CHARACTERS_SALESMAN, "Look, it sucks up dirt really well. That's the main thing, right?");
    dialog_text(CHARACTERS_NERD, "That's like saying a computer 'does computer things.' I need specifications.");
    
    switch (dialog_selection("How do you respond?", 
        dialog_options("Show the spec sheet", 
        "Challenge his vacuum knowledge", 
        "Appeal to the aesthetic design",
        "Mention the smart home features"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "Here's the spec sheet. Knock yourself out.");
        dialog_text(CHARACTERS_NERD, "Finally. Someone who speaks my language. Let me see... hm, interesting motor design. German?");
        dialog_text(CHARACTERS_SALESMAN, "You can tell that from the specs?");
        dialog_text(CHARACTERS_NERD, "You can tell a lot of things if you pay attention. Like the fact that you've been going door to door for about four hours based on your shoe wear.");
        dialog_text(CHARACTERS_SALESMAN, "That's... unsettling.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "Oh yeah? What vacuum do YOU use then?");
        dialog_text(CHARACTERS_NERD, "I built my own. Modified a shop vac with custom filtration and variable speed control.");
        dialog_text(CHARACTERS_SALESMAN, "Of course you did.");
        dialog_text(CHARACTERS_NERD, "It's not pretty, but it has seventeen percent more suction than anything on the market.");
        dialog_text(CHARACTERS_SALESMAN, "Did you measure that yourself?");
        dialog_text(CHARACTERS_NERD, "Obviously. I have a testing apparatus.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "Look at the sleek design though! It'll look great in your living room!");
        dialog_text(CHARACTERS_NERD, "I don't optimize for aesthetics. I optimize for function. My living room looks like a server farm, and I'm proud of that.");
        dialog_text(CHARACTERS_SALESMAN, "Fair enough. Function over form.");
        dialog_text(CHARACTERS_NERD, "Exactly. Finally, some respect for engineering principles.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "It has WiFi! You can control it with an app!");
        dialog_text(CHARACTERS_NERD, "A WiFi-connected vacuum? Do you want to know how many IoT devices get compromised every year?");
        dialog_text(CHARACTERS_SALESMAN, "Not really—");
        dialog_text(CHARACTERS_NERD, "Twelve million. Twelve million devices. Someone could hack your vacuum.");
        dialog_text(CHARACTERS_SALESMAN, "To do what? Steal my dirt?");
        dialog_text(CHARACTERS_NERD, "To map your house. Join your network. The vacuum is just the beginning.");
        break;
    }
    
    dialog_text(CHARACTERS_NERD, "Okay, I'll give you a chance to prove this thing works. But I'm timing you.");
    dialog_text(CHARACTERS_SALESMAN, "Of course you are.");
    
    encounter_minigame(&memory_game);
    
    dialog_text(CHARACTERS_NERD, "Not bad. Not as good as my custom build, but respectable.");
    dialog_text(CHARACTERS_SALESMAN, "So... are you interested?");
    dialog_text(CHARACTERS_NERD, "I'll take one. For parts. I can improve the motor efficiency by at least eight percent.");
    dialog_text(CHARACTERS_SALESMAN, "Whatever works for you, buddy.");
    
    encounter_end();
}

void encounter_nerd_books(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! Can I interest you in a complete encyclopedia set?");
    dialog_text(CHARACTERS_NERD, "Physical encyclopedias? You mean like Wikipedia but slower and immediately outdated?");
    dialog_text(CHARACTERS_SALESMAN, "There's something to be said for having a physical reference—");
    dialog_text(CHARACTERS_NERD, "By the time you shipped these, the science sections would already be wrong. We discover like a hundred new species a year.");
    dialog_text(CHARACTERS_SALESMAN, "Well, the historical sections don't change—");
    dialog_text(CHARACTERS_NERD, "Historical interpretation changes constantly. New archaeological evidence, revised theories, declassified documents.");
    dialog_text(CHARACTERS_SALESMAN, "You must be fun at parties.");
    dialog_text(CHARACTERS_NERD, "I don't go to parties. Too loud. Poor signal-to-noise ratio of conversation.");
    
    switch (dialog_selection("Try a different approach:", 
        dialog_options("Mention the collectors' value", 
        "Talk about the illustrations", 
        "Emphasize offline access",
        "Bring up the bibliography sections"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "These could be collectors' items someday!");
        dialog_text(CHARACTERS_NERD, "I collect things too. But useful things. Vintage computer hardware. First edition programming manuals.");
        dialog_text(CHARACTERS_SALESMAN, "See? You understand the value of physical books!");
        dialog_text(CHARACTERS_NERD, "I understand the value of SPECIFIC physical books. Generic encyclopedias aren't on that list.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "The illustrations are gorgeous! Hand-drawn anatomical diagrams, map overlays—");
        dialog_text(CHARACTERS_NERD, "Actually, that's interesting. What DPI are we talking? What printing method?");
        dialog_text(CHARACTERS_SALESMAN, "I have no idea what DPI means.");
        dialog_text(CHARACTERS_NERD, "Dots per inch. It determines image quality. Come on, this is basic stuff.");
        dialog_text(CHARACTERS_SALESMAN, "I sell books. I don't manufacture them.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "No internet required! Great for when the power goes out!");
        dialog_text(CHARACTERS_NERD, "My backup power system runs for seventy-two hours. I have a generator after that. I've prepared for scenarios you haven't imagined.");
        dialog_text(CHARACTERS_SALESMAN, "What about an EMP?");
        dialog_text(CHARACTERS_NERD, "Faraday cage in the basement. Nice try though.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "The bibliography sections are extensive! Great for research!");
        dialog_text(CHARACTERS_NERD, "Now that's actually useful. Primary sources are important. How extensive are we talking?");
        dialog_text(CHARACTERS_SALESMAN, "Each article has citations, and there's a master bibliography in the final volume.");
        dialog_text(CHARACTERS_NERD, "Interesting. A citation index in physical form. That's almost nostalgic.");
        break;
    }
    
    dialog_text(CHARACTERS_NERD, "Alright. Give me your best pitch on why I should buy these instead of just using Scholar or Arxiv.");
    dialog_text(CHARACTERS_SALESMAN, "Because sometimes it's nice to read without a screen giving you eye strain?");
    
    encounter_minigame(&memory_game);
    
    dialog_text(CHARACTERS_NERD, "Okay. That was a surprisingly coherent argument.");
    dialog_text(CHARACTERS_SALESMAN, "So you'll take a set?");
    dialog_text(CHARACTERS_NERD, "I'll take the science volumes only. The rest is padding. And I'm fact-checking them against current literature.");
    dialog_text(CHARACTERS_SALESMAN, "Wouldn't expect anything less.");
    
    encounter_end();
}

void encounter_nerd_knives(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! I'm selling professional-grade cutlery!");
    dialog_text(CHARACTERS_NERD, "What's the Rockwell hardness rating?");
    dialog_text(CHARACTERS_SALESMAN, "Excuse me?");
    dialog_text(CHARACTERS_NERD, "The hardness of the steel. It determines edge retention. You should know this.");
    dialog_text(CHARACTERS_SALESMAN, "I know they're sharp and they cut things—");
    dialog_text(CHARACTERS_NERD, "That's like saying a car 'goes places.' What's the blade geometry? Full tang or partial?");
    dialog_text(CHARACTERS_SALESMAN, "I think full tang? Let me check the pamphlet.");
    dialog_text(CHARACTERS_NERD, "You think? You're selling these and you don't know the construction?");
    
    switch (dialog_selection("Respond to his skepticism:", 
        dialog_options("Show him the blade up close", 
        "Mention the forging process", 
        "Talk about the handle material",
        "Discuss the edge angle"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "Here, look at the blade. You can see the quality.");
        dialog_text(CHARACTERS_NERD, "Hmm. The grain structure is decent. Not Damascus, but not stamped either. Forged?");
        dialog_text(CHARACTERS_SALESMAN, "Yes! Forged! It says that on the box!");
        dialog_text(CHARACTERS_NERD, "Don't get excited. Forged is the minimum for quality. It's not impressive, it's expected.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "These are hand-forged by master craftsmen!");
        dialog_text(CHARACTERS_NERD, "Where? Japan? Germany? Location matters. Metallurgical traditions vary significantly.");
        dialog_text(CHARACTERS_SALESMAN, "Ohio, actually.");
        dialog_text(CHARACTERS_NERD, "Ohio doesn't have a knife-making tradition. They're probably using German machinery with American labor.");
        dialog_text(CHARACTERS_SALESMAN, "Is that bad?");
        dialog_text(CHARACTERS_NERD, "It's fine. Just don't romanticize it.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "The handles are ergonomically designed with high-density polymer grips!");
        dialog_text(CHARACTERS_NERD, "What polymer? Not all plastics are created equal. If it's ABS, that's cheap. Glass-filled nylon is better.");
        dialog_text(CHARACTERS_SALESMAN, "I genuinely don't know. Do you want me to call the company?");
        dialog_text(CHARACTERS_NERD, "No, I can tell from the texture. It's POM. Decent choice, actually. Good durability, resists moisture.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "The edges are precision-ground at fifteen degrees!");
        dialog_text(CHARACTERS_NERD, "Fifteen degrees total or per side? Because that's a significant difference in sharpness and durability.");
        dialog_text(CHARACTERS_SALESMAN, "Per side, I think?");
        dialog_text(CHARACTERS_NERD, "So thirty degrees total. That's a Western standard. Fine for general use, but not ideal for fine cuts.");
        break;
    }
    
    dialog_text(CHARACTERS_NERD, "Let's test the edge. I have some paper over here.");
    dialog_text(CHARACTERS_SALESMAN, "Paper?");
    dialog_text(CHARACTERS_NERD, "Phone book paper. Thin. It reveals imperfections in the edge. This is how you test a knife.");
    
    encounter_minigame(&memory_game);
    
    dialog_text(CHARACTERS_NERD, "Clean cut. No tearing. The edge is better than I expected.");
    dialog_text(CHARACTERS_SALESMAN, "Does that mean I made a sale?");
    dialog_text(CHARACTERS_NERD, "I'll take the chef's knife and the paring knife. Not the bread knife. Serrated edges are impossible to sharpen properly without specialized equipment.");
    dialog_text(CHARACTERS_SALESMAN, "I'll take what I can get.");
    
    encounter_end();
}

void encounter_nerd_lollipop(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Hey! Want to try some artisanal lollipops?");
    dialog_text(CHARACTERS_NERD, "Define artisanal. That word has lost all meaning in modern marketing.");
    dialog_text(CHARACTERS_SALESMAN, "Small batch! Hand-crafted! Natural ingredients!");
    dialog_text(CHARACTERS_NERD, "Natural is meaningless too. Arsenic is natural. Doesn't mean I want it in my candy.");
    dialog_text(CHARACTERS_SALESMAN, "Fair point. How about 'made with care by people who take pride in their work'?");
    dialog_text(CHARACTERS_NERD, "That's... actually a better pitch. Still vague, but at least honest.");
    dialog_text(CHARACTERS_SALESMAN, "Thanks! I'm learning to read my customers.");
    dialog_text(CHARACTERS_NERD, "What's the sugar source? Cane, beet, or corn syrup?");
    
    switch (dialog_selection("Answer the sugar question:", 
        dialog_options("Organic cane sugar", 
        "Check the ingredients list", 
        "Offer a sugar-free option",
        "Deflect with flavor talk"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "One hundred percent organic cane sugar!");
        dialog_text(CHARACTERS_NERD, "Organic is mostly marketing, but cane sugar does have a better flavor profile than beet. The molasses content varies.");
        dialog_text(CHARACTERS_SALESMAN, "You know a lot about sugar.");
        dialog_text(CHARACTERS_NERD, "I know a lot about everything. It's a blessing and a curse.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "Let me check the label here...");
        dialog_text(CHARACTERS_NERD, "You don't have the ingredients memorized? You should know your product.");
        dialog_text(CHARACTERS_SALESMAN, "I sell six different products. I can't memorize everything.");
        dialog_text(CHARACTERS_NERD, "I could memorize it. I have an eidetic memory. But I understand not everyone does.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "We have sugar-free options with natural sweeteners!");
        dialog_text(CHARACTERS_NERD, "Which sweeteners? Not stevia, I hope. That aftertaste is unbearable.");
        dialog_text(CHARACTERS_SALESMAN, "It's a monk fruit blend.");
        dialog_text(CHARACTERS_NERD, "Acceptable. Monk fruit doesn't spike blood glucose. And no weird aftertaste if processed correctly.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "The flavors are what really matter! We have cherry, apple—");
        dialog_text(CHARACTERS_NERD, "Natural flavors or artificial? Because 'natural flavors' on labels can include some questionable compounds.");
        dialog_text(CHARACTERS_SALESMAN, "Is there anything you don't overanalyze?");
        dialog_text(CHARACTERS_NERD, "Sleep. I've given up on that. My brain won't stop.");
        break;
    }
    
    dialog_text(CHARACTERS_NERD, "Alright. I'll conduct a taste test. But I'm evaluating on a ten-point scale with three subcategories.");
    dialog_text(CHARACTERS_SALESMAN, "You have a scoring system for lollipops?");
    dialog_text(CHARACTERS_NERD, "I have a scoring system for everything.");
    
    encounter_minigame(&memory_game);
    
    dialog_text(CHARACTERS_NERD, "Seven point four. Above average. The flavor release timing was good, and the texture was consistent.");
    dialog_text(CHARACTERS_SALESMAN, "I'll take it! So you want some?");
    dialog_text(CHARACTERS_NERD, "I'll take one of each flavor. For complete data collection. And maybe because they taste okay.");
    dialog_text(CHARACTERS_SALESMAN, "Was that almost emotional?");
    dialog_text(CHARACTERS_NERD, "Don't push it.");
    
    encounter_end();
}

void encounter_nerd_computer(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! Can I interest you in a new computer?");
    dialog_text(CHARACTERS_NERD, "What are the specs?");
    dialog_text(CHARACTERS_SALESMAN, "It's got a great processor, lots of memory—");
    dialog_text(CHARACTERS_NERD, "That's not specs. That's vague adjectives. What processor? What generation? What clock speed?");
    dialog_text(CHARACTERS_SALESMAN, "I've got the spec sheet here somewhere...");
    dialog_text(CHARACTERS_NERD, "While you look, I should mention I built my current machine. Custom water cooling. Overclocked. Cable management took six hours.");
    dialog_text(CHARACTERS_SALESMAN, "Six hours for cables?");
    dialog_text(CHARACTERS_NERD, "Airflow matters. And aesthetics. Even if no one sees it, I know it's perfect.");
    
    switch (dialog_selection("Make your pitch:", 
        dialog_options("Focus on pre-built convenience", 
        "Highlight the warranty", 
        "Mention specific components",
        "Talk about software included"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "Don't you want something that just works? No building, no troubleshooting?");
        dialog_text(CHARACTERS_NERD, "Building IS the fun. Troubleshooting is where you learn things. Pre-built is for people who don't want to understand their machine.");
        dialog_text(CHARACTERS_SALESMAN, "Some people just want to use it, not marry it.");
        dialog_text(CHARACTERS_NERD, "And those people are living incomplete lives. In my opinion.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "Three-year warranty! Anything goes wrong, we fix it!");
        dialog_text(CHARACTERS_NERD, "What's the mean time between failures on your components? What's your repair turnaround?");
        dialog_text(CHARACTERS_SALESMAN, "I don't have those statistics on hand—");
        dialog_text(CHARACTERS_NERD, "Then the warranty is a black box. Could be great, could be terrible. I prefer known variables.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "Latest generation CPU, dedicated graphics card, NVMe storage—");
        dialog_text(CHARACTERS_NERD, "Which GPU specifically? That determines whether I can run neural networks locally.");
        dialog_text(CHARACTERS_SALESMAN, "You run neural networks at home?");
        dialog_text(CHARACTERS_NERD, "Doesn't everyone? I'm training a model to predict when my neighbor is going to mow his lawn. So far it's seventy-three percent accurate.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "It comes with a full productivity suite! Word processing, spreadsheets—");
        dialog_text(CHARACTERS_NERD, "I use Linux. None of that software works on Linux.");
        dialog_text(CHARACTERS_SALESMAN, "We have a Linux-compatible model!");
        dialog_text(CHARACTERS_NERD, "Wait, really? What distro is it set up for?");
        dialog_text(CHARACTERS_SALESMAN, "I have no idea, but I can find out!");
        break;
    }
    
    dialog_text(CHARACTERS_NERD, "Let's see what this machine can actually do. I'm going to run some benchmarks.");
    dialog_text(CHARACTERS_SALESMAN, "You carry benchmark software with you?");
    dialog_text(CHARACTERS_NERD, "On a USB drive. Always. You never know when you need to test hardware.");
    
    encounter_minigame(&memory_game);
    
    dialog_text(CHARACTERS_NERD, "Acceptable scores. Not as good as my custom build, but within ten percent.");
    dialog_text(CHARACTERS_SALESMAN, "Is that good?");
    dialog_text(CHARACTERS_NERD, "For a pre-built? Yes. I'm impressed they didn't cheap out on the thermal paste.");
    dialog_text(CHARACTERS_SALESMAN, "So... interested?");
    dialog_text(CHARACTERS_NERD, "I'll take one. For my guest room. So when people visit, they have something decent to use.");
    dialog_text(CHARACTERS_SALESMAN, "You get a lot of guests?");
    dialog_text(CHARACTERS_NERD, "No. But I'll be ready if I do.");
    
    encounter_end();
}

// ============================================================================
// SHOTGUNNER ENCOUNTERS
// ============================================================================

void encounter_shotgunner_vacuum(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! I'm selling—");
    dialog_text(CHARACTERS_SHOTGUNNER, "I can see what you're selling. You're on my property. State your business quick.");
    dialog_text(CHARACTERS_SALESMAN, "Right! Vacuum cleaners! Great for keeping the house clean!");
    dialog_text(CHARACTERS_SHOTGUNNER, "Do I look like I care about a clean house?");
    dialog_text(CHARACTERS_SALESMAN, "Well, I mean, cleanliness is next to—");
    dialog_text(CHARACTERS_SHOTGUNNER, "Don't say godliness. I've shot at preachers too.");
    dialog_text(CHARACTERS_SALESMAN, "I was going to say 'a good defense perimeter.' Dirt hides footprints. You can't see who's been in your house.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Huh. That's actually a fair point. Go on.");
    
    switch (dialog_selection("Continue the pitch:", 
        dialog_options("Emphasize noise level", 
        "Talk about durability", 
        "Mention intruder detection",
        "Discuss debris removal"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "This vacuum is extremely quiet. Won't alert anyone to your position.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Tactical vacuuming. I like your thinking.");
        dialog_text(CHARACTERS_SALESMAN, "Plus you can hear approaching threats better when you're not running a loud motor.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Now you're speaking my language, salesman.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "This thing is built like a tank. You could throw it at an intruder if needed.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Why would I throw a vacuum when I have a shotgun?");
        dialog_text(CHARACTERS_SALESMAN, "Backup plan? Ammunition runs out, vacuums don't.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Ammunition doesn't run out if you reload fast enough. But I respect the backup mentality.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "A clean floor means you can see if anything's been disturbed. Instant intruder detection.");
        dialog_text(CHARACTERS_SHOTGUNNER, "I have cameras for that.");
        dialog_text(CHARACTERS_SALESMAN, "Cameras can be hacked. A clean floor can't be hacked.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Damn. That's a good point. Low-tech has its advantages.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "You can pick up shell casings faster. No evidence.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Evidence of what? This is MY property. Castle doctrine.");
        dialog_text(CHARACTERS_SALESMAN, "Of course! I just meant for convenience. Stepping on casings is annoying.");
        dialog_text(CHARACTERS_SHOTGUNNER, "You're not wrong about that.");
        break;
    }
    
    dialog_text(CHARACTERS_SHOTGUNNER, "Alright. Prove this thing works. That rug hasn't been cleaned since ninety-four.");
    dialog_text(CHARACTERS_SALESMAN, "What happened in ninety-four?");
    dialog_text(CHARACTERS_SHOTGUNNER, "None of your business. Get to work.");
    
    encounter_minigame(&shotgun_game);
    
    dialog_text(CHARACTERS_SHOTGUNNER, "Well I'll be damned. It actually works.");
    dialog_text(CHARACTERS_SALESMAN, "Modern suction technology!");
    dialog_text(CHARACTERS_SHOTGUNNER, "Fine. I'll take one. But if it breaks, I'm coming to find you.");
    dialog_text(CHARACTERS_SALESMAN, "It has a warranty—");
    dialog_text(CHARACTERS_SHOTGUNNER, "I don't need a warranty. I have methods.");
    dialog_text(CHARACTERS_SALESMAN, "The warranty will be faster, I promise.");
    
    encounter_end();
}

void encounter_shotgunner_books(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! I'm selling encyclopedias—");
    dialog_text(CHARACTERS_SHOTGUNNER, "Books? You're selling books? What do I look like, a library?");
    dialog_text(CHARACTERS_SALESMAN, "Knowledge is power, sir.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Twelve gauge is power. Knowledge is secondary.");
    dialog_text(CHARACTERS_SALESMAN, "Fair enough. But did you know encyclopedias can stop small caliber rounds?");
    dialog_text(CHARACTERS_SHOTGUNNER, "Can they now?");
    dialog_text(CHARACTERS_SALESMAN, "Twenty-six volumes. That's basically body armor for your bookshelf.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Now that's interesting. Go on.");
    
    switch (dialog_selection("Expand on the pitch:", 
        dialog_options("Discuss tactical placement", 
        "Mention historical warfare", 
        "Talk about survivalism sections",
        "Emphasize weight and thickness"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "You could strategically place these around the house. Cover behind every corner.");
        dialog_text(CHARACTERS_SHOTGUNNER, "A literate killbox. I like your creativity.");
        dialog_text(CHARACTERS_SALESMAN, "Plus they look normal. Nobody expects the books to be defensive.");
        dialog_text(CHARACTERS_SHOTGUNNER, "The element of surprise. Crucial.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "The history volumes cover every major war. Strategies, tactics, lessons learned.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Those who don't learn from history are doomed to get shot by people who did.");
        dialog_text(CHARACTERS_SALESMAN, "That's... not the original quote, but I like yours better.");
        dialog_text(CHARACTERS_SHOTGUNNER, "I adapted it for practicality.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "There are sections on wilderness survival, first aid, emergency preparedness.");
        dialog_text(CHARACTERS_SHOTGUNNER, "The grid is going down someday. Everyone knows it.");
        dialog_text(CHARACTERS_SALESMAN, "These books work without electricity. Just like the founders intended.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Now you're making sense.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "Each volume is two inches thick. Leather bound. Heavy.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Heavy means stopping power. What caliber can they take?");
        dialog_text(CHARACTERS_SALESMAN, "I've seen them stop a .22 in demonstrations.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Twenty-two is nothing. What about .45?");
        dialog_text(CHARACTERS_SALESMAN, "Maybe stack two volumes?");
        break;
    }
    
    dialog_text(CHARACTERS_SHOTGUNNER, "Let me see one of these books up close.");
    dialog_text(CHARACTERS_SALESMAN, "Here you go. Volume A through B.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Hmm. Good weight. Dense pages.");
    
    encounter_minigame(&shotgun_game);
    
    dialog_text(CHARACTERS_SHOTGUNNER, "Quality construction. These will serve me well.");
    dialog_text(CHARACTERS_SALESMAN, "For reading, right?");
    dialog_text(CHARACTERS_SHOTGUNNER, "Sure. Reading. That too.");
    dialog_text(CHARACTERS_SALESMAN, "I'll put you down for a full set.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Two sets. One for each side of the house.");
    
    encounter_end();
}

void encounter_shotgunner_knives(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! I'm selling professional-grade cutlery!");
    dialog_text(CHARACTERS_SHOTGUNNER, "Knives? Finally, someone selling something useful.");
    dialog_text(CHARACTERS_SALESMAN, "Oh! Great! I wasn't expecting enthusiasm!");
    dialog_text(CHARACTERS_SHOTGUNNER, "A man needs a good knife. Backup weapon. Utility tool. Cooking.");
    dialog_text(CHARACTERS_SALESMAN, "Mostly cooking for most people—");
    dialog_text(CHARACTERS_SHOTGUNNER, "I'm not most people.");
    dialog_text(CHARACTERS_SALESMAN, "Clearly not. Well, these knives are German steel. Hold an edge forever.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Germans know metallurgy. What's the blade length?");
    
    switch (dialog_selection("Discuss the knives:", 
        dialog_options("Show the chef's knife", 
        "Present the hunting knife", 
        "Display the cleaver",
        "Reveal the throwing knives"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "Eight-inch chef's knife. Versatile. Good for everything.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Everything including self-defense?");
        dialog_text(CHARACTERS_SALESMAN, "I mean... technically? Kitchens can be dangerous.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Very dangerous. In the right hands.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "This hunting knife has a serrated spine. Good for sawing.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Sawing what?");
        dialog_text(CHARACTERS_SALESMAN, "Rope. Branches. Whatever needs sawing.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Good non-answer. I respect that.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "The cleaver. Heavy. Powerful. Makes quick work of bone.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Bone, you say?");
        dialog_text(CHARACTERS_SALESMAN, "Chicken bone. Beef bone. Normal cooking bones.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Sure. Normal cooking.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "We also have these throwing knives. Perfectly balanced.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Now we're talking. What's the range?");
        dialog_text(CHARACTERS_SALESMAN, "Effective up to about fifteen feet. Competition grade.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Fifteen feet is plenty for hallway defense.");
        dialog_text(CHARACTERS_SALESMAN, "I was thinking more like dart games—");
        dialog_text(CHARACTERS_SHOTGUNNER, "Sure. Dart games.");
        break;
    }
    
    dialog_text(CHARACTERS_SHOTGUNNER, "Let's test that edge. You see that can over there?");
    dialog_text(CHARACTERS_SALESMAN, "The one at twenty yards?");
    dialog_text(CHARACTERS_SHOTGUNNER, "That's the one.");
    
    encounter_minigame(&shotgun_game);
    
    dialog_text(CHARACTERS_SHOTGUNNER, "Good blade. Good balance. Acceptable accuracy.");
    dialog_text(CHARACTERS_SALESMAN, "So you're interested?");
    dialog_text(CHARACTERS_SHOTGUNNER, "Full set. Plus four extra throwing knives. For when I lose some.");
    dialog_text(CHARACTERS_SALESMAN, "Lose some?");
    dialog_text(CHARACTERS_SHOTGUNNER, "Practice. I lose them in practice. Deep in the target.");
    dialog_text(CHARACTERS_SALESMAN, "Right. Practice targets.");
    
    encounter_end();
}

void encounter_shotgunner_lollipop(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! I'm selling artisanal lollipops!");
    dialog_text(CHARACTERS_SHOTGUNNER, "You came to MY house to sell me candy?");
    dialog_text(CHARACTERS_SALESMAN, "I'm regretting it already, sir.");
    dialog_text(CHARACTERS_SHOTGUNNER, "You should be. I'm a grown man. Grown men don't eat lollipops.");
    dialog_text(CHARACTERS_SALESMAN, "Actually, these are very popular with adults. Complex flavors, not too sweet—");
    dialog_text(CHARACTERS_SHOTGUNNER, "What flavors?");
    dialog_text(CHARACTERS_SALESMAN, "Bourbon vanilla, whiskey caramel, smoked maple—");
    dialog_text(CHARACTERS_SHOTGUNNER, "Hold up. Bourbon AND whiskey flavors?");
    dialog_text(CHARACTERS_SALESMAN, "Yes sir. Very masculine flavors, if I may say.");
    
    switch (dialog_selection("Offer a sample:", 
        dialog_options("Bourbon vanilla", 
        "Whiskey caramel", 
        "Smoked maple",
        "Black coffee"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "The bourbon vanilla is our bestseller. Hint of oak, touch of sweetness.");
        dialog_text(CHARACTERS_SHOTGUNNER, "I do appreciate bourbon. Just the flavor though, right? I don't drink on duty.");
        dialog_text(CHARACTERS_SALESMAN, "What duty?");
        dialog_text(CHARACTERS_SHOTGUNNER, "Perimeter defense. Twenty-four seven.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "Whiskey caramel. Smooth with a bite at the end.");
        dialog_text(CHARACTERS_SHOTGUNNER, "A bite. I like things with a bite.");
        dialog_text(CHARACTERS_SALESMAN, "It's a metaphor for flavor—");
        dialog_text(CHARACTERS_SHOTGUNNER, "I know what a metaphor is. I'm paranoid, not stupid.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "Smoked maple. Like a campfire in candy form.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Reminds me of survival training. Good memories.");
        dialog_text(CHARACTERS_SALESMAN, "Military?");
        dialog_text(CHARACTERS_SHOTGUNNER, "Self-taught. YouTube and determination.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "Black coffee flavor. Bitter, intense, no sweetener.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Finally. A candy for serious people.");
        dialog_text(CHARACTERS_SALESMAN, "It's our most... challenging flavor.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Life is challenging. Candy should be too.");
        break;
    }
    
    dialog_text(CHARACTERS_SHOTGUNNER, "Alright. I'll try one. But if it's too sweet, you're leaving immediately.");
    dialog_text(CHARACTERS_SALESMAN, "Understood. Here you go.");
    
    encounter_minigame(&shotgun_game);
    
    dialog_text(CHARACTERS_SHOTGUNNER, "Huh. That's not bad. Not bad at all.");
    dialog_text(CHARACTERS_SALESMAN, "Manly, right?");
    dialog_text(CHARACTERS_SHOTGUNNER, "Don't push it. But yes. I'll take a dozen. For stakeouts.");
    dialog_text(CHARACTERS_SALESMAN, "Stakeouts of what?");
    dialog_text(CHARACTERS_SHOTGUNNER, "My property. I watch the perimeter. Gets boring sometimes.");
    dialog_text(CHARACTERS_SALESMAN, "A lollipop does pass the time.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Exactly. You're smarter than you look, salesman.");
    
    encounter_end();
}

void encounter_shotgunner_computer(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! Can I interest you in a new computer?");
    dialog_text(CHARACTERS_SHOTGUNNER, "Computer? You mean a government tracking device?");
    dialog_text(CHARACTERS_SALESMAN, "It's just a regular computer—");
    dialog_text(CHARACTERS_SHOTGUNNER, "Nothing is just regular. Everything has a purpose. What's the camera for?");
    dialog_text(CHARACTERS_SALESMAN, "Video calls?");
    dialog_text(CHARACTERS_SHOTGUNNER, "That's what they WANT you to think.");
    dialog_text(CHARACTERS_SALESMAN, "You know you can cover the camera with tape, right?");
    dialog_text(CHARACTERS_SHOTGUNNER, "Obviously. I'm not an amateur. What about the microphone?");
    
    switch (dialog_selection("Address his concerns:", 
        dialog_options("Mention the mute switch", 
        "Talk about air-gapped systems", 
        "Suggest offline use only",
        "Discuss encryption options"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "There's a hardware mute switch. Physically disconnects the mic.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Hardware switch. That's better than software. Can't hack hardware.");
        dialog_text(CHARACTERS_SALESMAN, "Exactly! Full control over your privacy!");
        dialog_text(CHARACTERS_SHOTGUNNER, "Privacy is an illusion, but a hardware switch helps.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "You could use it air-gapped. Never connect to the internet.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Air-gapped. You know that term?");
        dialog_text(CHARACTERS_SALESMAN, "I watch a lot of spy movies.");
        dialog_text(CHARACTERS_SHOTGUNNER, "Those aren't documentaries. But you're on the right track.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "Use it offline only. Word processing, spreadsheets, no network.");
        dialog_text(CHARACTERS_SHOTGUNNER, "What kind of spreadsheets?");
        dialog_text(CHARACTERS_SALESMAN, "I don't know... budgets? Inventory?");
        dialog_text(CHARACTERS_SHOTGUNNER, "Ammunition inventory. That's a good idea actually.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "You could encrypt everything. Military-grade encryption.");
        dialog_text(CHARACTERS_SHOTGUNNER, "What algorithm?");
        dialog_text(CHARACTERS_SALESMAN, "I don't... specifically know. But it says military-grade on the box.");
        dialog_text(CHARACTERS_SHOTGUNNER, "That's marketing speak. But I can install my own encryption.");
        break;
    }
    
    dialog_text(CHARACTERS_SHOTGUNNER, "Show me this machine works without any internet connection.");
    dialog_text(CHARACTERS_SALESMAN, "No problem. Watch this offline demo.");
    
    encounter_minigame(&shotgun_game);
    
    dialog_text(CHARACTERS_SHOTGUNNER, "Alright. It runs offline. That's acceptable.");
    dialog_text(CHARACTERS_SALESMAN, "So you'll take one?");
    dialog_text(CHARACTERS_SHOTGUNNER, "I'll take one. After I remove the WiFi card and disable the Bluetooth module.");
    dialog_text(CHARACTERS_SALESMAN, "That's... very thorough.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Thorough keeps you alive. Remember that, salesman.");
    dialog_text(CHARACTERS_SALESMAN, "I definitely will. Forever, probably.");
    
    encounter_end();
}

// ============================================================================
// FATMAN ENCOUNTERS
// ============================================================================

void encounter_fatman_vacuum(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon, sir! I'm selling premium vacuum cleaners!");
    dialog_text(CHARACTERS_FATMAN, "Vacuum cleaners? What for?");
    dialog_text(CHARACTERS_SALESMAN, "For... cleaning? Your floors?");
    dialog_text(CHARACTERS_FATMAN, "I don't really see the floor that much. Too much effort to look down.");
    dialog_text(CHARACTERS_SALESMAN, "But don't you want a clean living space?");
    dialog_text(CHARACTERS_FATMAN, "I want a comfortable living space. There's a difference.");
    dialog_text(CHARACTERS_SALESMAN, "Wouldn't it be more comfortable without crumbs everywhere?");
    dialog_text(CHARACTERS_FATMAN, "The crumbs remind me of good times. Each one is a memory.");
    
    switch (dialog_selection("Appeal to his interests:", 
        dialog_options("Mention snack preservation", 
        "Talk about comfort features", 
        "Emphasize minimal effort",
        "Discuss remote control option"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "If you keep the floor clean, snacks don't get stale from floor humidity!");
        dialog_text(CHARACTERS_FATMAN, "Wait, is that a real thing?");
        dialog_text(CHARACTERS_SALESMAN, "Sure! Clean floors, fresher snacks. Science.");
        dialog_text(CHARACTERS_FATMAN, "I do hate stale chips. They lose the crunch.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "This vacuum is so quiet, you can use it during TV time without missing dialogue!");
        dialog_text(CHARACTERS_FATMAN, "I watch a lot of TV. That's actually useful.");
        dialog_text(CHARACTERS_SALESMAN, "No more pausing your shows to clean!");
        dialog_text(CHARACTERS_FATMAN, "I wasn't pausing shows to clean before either. But I appreciate the thought.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "This vacuum is incredibly light. You barely have to move.");
        dialog_text(CHARACTERS_FATMAN, "How light?");
        dialog_text(CHARACTERS_SALESMAN, "You could push it with one hand while holding a sandwich in the other.");
        dialog_text(CHARACTERS_FATMAN, "Now that's good engineering. Practical design.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "There's a robot version! It cleans itself while you relax!");
        dialog_text(CHARACTERS_FATMAN, "Robot? It does the work for me?");
        dialog_text(CHARACTERS_SALESMAN, "Exactly! You just set it and forget it!");
        dialog_text(CHARACTERS_FATMAN, "That's my philosophy for everything. Set it and forget it.");
        break;
    }
    
    dialog_text(CHARACTERS_FATMAN, "Alright, show me what it can do. But I'm not getting up. Demonstrate from here.");
    dialog_text(CHARACTERS_SALESMAN, "I can work with that. Watch this!");
    
    encounter_minigame(&rhythm_game);
    
    dialog_text(CHARACTERS_FATMAN, "Huh. That actually worked. And you didn't even need me to move.");
    dialog_text(CHARACTERS_SALESMAN, "The future of cleaning!");
    dialog_text(CHARACTERS_FATMAN, "I'll take one. But you're delivering it inside. I'm not carrying anything.");
    dialog_text(CHARACTERS_SALESMAN, "Delivery is included, sir.");
    dialog_text(CHARACTERS_FATMAN, "Good. I knew I liked you.");
    
    encounter_end();
}

void encounter_fatman_books(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! Can I interest you in a complete encyclopedia set?");
    dialog_text(CHARACTERS_FATMAN, "Books? Those are heavy. I don't do heavy.");
    dialog_text(CHARACTERS_SALESMAN, "But they contain the sum of human knowledge!");
    dialog_text(CHARACTERS_FATMAN, "My phone has that too. And it's pocket-sized.");
    dialog_text(CHARACTERS_SALESMAN, "But you can't eat while scrolling through encyclopedias on your phone!");
    dialog_text(CHARACTERS_FATMAN, "Actually, I can. I'm very coordinated when food is involved.");
    dialog_text(CHARACTERS_SALESMAN, "Fair point. But physical books don't run out of battery.");
    dialog_text(CHARACTERS_FATMAN, "That's true. I hate getting up to plug things in.");
    
    switch (dialog_selection("Find an angle:", 
        dialog_options("Mention food-related entries", 
        "Talk about comfortable reading", 
        "Emphasize food history sections",
        "Discuss cookbook supplements"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "These encyclopedias have entries on every food in existence!");
        dialog_text(CHARACTERS_FATMAN, "Every food? Even regional specialties?");
        dialog_text(CHARACTERS_SALESMAN, "Every dish from every culture. Detailed descriptions.");
        dialog_text(CHARACTERS_FATMAN, "That's basically food tourism without leaving my couch. I'm intrigued.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "You can read these lying down. No screen glare.");
        dialog_text(CHARACTERS_FATMAN, "No blue light either. I read that blue light is bad.");
        dialog_text(CHARACTERS_SALESMAN, "Exactly! These are good for your eyes and your sleep!");
        dialog_text(CHARACTERS_FATMAN, "Good sleep means more energy for breakfast. I like that math.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "The history of cuisine is covered in depth. Pizza origins. Burger evolution.");
        dialog_text(CHARACTERS_FATMAN, "The evolution of burgers? That's a story I need to know.");
        dialog_text(CHARACTERS_SALESMAN, "It spans three pages. Very thorough.");
        dialog_text(CHARACTERS_FATMAN, "Three pages on burgers. Now that's quality content.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "If you buy the set, I can include a cookbook supplement!");
        dialog_text(CHARACTERS_FATMAN, "Now you're speaking my language. What kind of recipes?");
        dialog_text(CHARACTERS_SALESMAN, "Everything from simple snacks to elaborate meals!");
        dialog_text(CHARACTERS_FATMAN, "I'm mostly interested in the snacks. But elaborate meals look nice in pictures.");
        break;
    }
    
    dialog_text(CHARACTERS_FATMAN, "Let me see one of these books. But bring it here. I'm comfortable.");
    dialog_text(CHARACTERS_SALESMAN, "Here you go. Volume F through G.");
    dialog_text(CHARACTERS_FATMAN, "F through G? Perfect. That covers 'Food' and 'Gastronomy.'");
    
    encounter_minigame(&rhythm_game);
    
    dialog_text(CHARACTERS_FATMAN, "This is actually pretty good. Nice pictures too.");
    dialog_text(CHARACTERS_SALESMAN, "So you'll take a set?");
    dialog_text(CHARACTERS_FATMAN, "I'll take two sets. One for each arm of the couch.");
    dialog_text(CHARACTERS_SALESMAN, "That's... a lot of books.");
    dialog_text(CHARACTERS_FATMAN, "I like options within arm's reach. It's a lifestyle.");
    
    encounter_end();
}

void encounter_fatman_knives(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! I'm selling professional-grade cutlery!");
    dialog_text(CHARACTERS_FATMAN, "Knives? For cooking?");
    dialog_text(CHARACTERS_SALESMAN, "Among other things, yes! Cooking is the primary use.");
    dialog_text(CHARACTERS_FATMAN, "I like cooking. Well, I like eating what I cook. The cooking part is okay.");
    dialog_text(CHARACTERS_SALESMAN, "Good knives make cooking easier! Less effort, better results!");
    dialog_text(CHARACTERS_FATMAN, "Less effort is key. I optimize for minimum effort, maximum flavor.");
    dialog_text(CHARACTERS_SALESMAN, "These knives will cut your prep time in half!");
    dialog_text(CHARACTERS_FATMAN, "Half? That means I can cook twice as much in the same time.");
    
    switch (dialog_selection("Highlight cooking benefits:", 
        dialog_options("Show the vegetable knife", 
        "Present the meat carving knife", 
        "Display the bread knife",
        "Reveal the pizza cutter"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "This vegetable knife glides through anything. Onions, peppers, you name it.");
        dialog_text(CHARACTERS_FATMAN, "Does it work on potatoes? I eat a lot of potatoes.");
        dialog_text(CHARACTERS_SALESMAN, "It's perfect for potatoes. Cuts right through, no resistance.");
        dialog_text(CHARACTERS_FATMAN, "I do hate when potatoes fight back. This could change everything.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "The carving knife is perfect for roasts, turkeys, any large meat.");
        dialog_text(CHARACTERS_FATMAN, "I like large meat. This speaks to me.");
        dialog_text(CHARACTERS_SALESMAN, "You'll get perfect, even slices every time!");
        dialog_text(CHARACTERS_FATMAN, "Even slices? So each bite has the same amount of meat? That's beautiful.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "The bread knife has a serrated edge. Perfect for crusty loaves.");
        dialog_text(CHARACTERS_FATMAN, "I love bread. It loves me too. We have a relationship.");
        dialog_text(CHARACTERS_SALESMAN, "This knife will treat your bread with the respect it deserves.");
        dialog_text(CHARACTERS_FATMAN, "Finally, someone who understands bread appreciation.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "And of course, the pizza cutter. Sharp, smooth, perfect slices.");
        dialog_text(CHARACTERS_FATMAN, "Pizza cutter? That should lead the presentation.");
        dialog_text(CHARACTERS_SALESMAN, "I'll remember that for my next customer.");
        dialog_text(CHARACTERS_FATMAN, "The next customer doesn't appreciate pizza like I do. Trust me.");
        break;
    }
    
    dialog_text(CHARACTERS_FATMAN, "Alright, let's see what these knives can do. I have some cheese that needs cutting.");
    dialog_text(CHARACTERS_SALESMAN, "Cheese? I can demonstrate on that!");
    
    encounter_minigame(&rhythm_game);
    
    dialog_text(CHARACTERS_FATMAN, "Beautiful. That cheese was defiant, and now it's in perfect cubes.");
    dialog_text(CHARACTERS_SALESMAN, "Victory over cheese!");
    dialog_text(CHARACTERS_FATMAN, "I'll take the whole set. And do you have a sharpening stone? I want these to last forever.");
    dialog_text(CHARACTERS_SALESMAN, "Included with purchase!");
    dialog_text(CHARACTERS_FATMAN, "Good man. Good knives. Good day.");
    
    encounter_end();
}

void encounter_fatman_lollipop(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! I'm selling artisanal lollipops!");
    dialog_text(CHARACTERS_FATMAN, "Candy? You've come to the right house.");
    dialog_text(CHARACTERS_SALESMAN, "Oh! Great! Usually I have to convince people!");
    dialog_text(CHARACTERS_FATMAN, "Convince people to eat sugar? What's wrong with people?");
    dialog_text(CHARACTERS_SALESMAN, "These are gourmet lollipops! Complex flavor profiles, quality ingredients!");
    dialog_text(CHARACTERS_FATMAN, "Complex flavors? I'm listening. I have a sophisticated palate.");
    dialog_text(CHARACTERS_SALESMAN, "I can tell! You strike me as a man who appreciates quality!");
    dialog_text(CHARACTERS_FATMAN, "Quality and quantity. Why choose?");
    
    switch (dialog_selection("Offer flavors:", 
        dialog_options("Salted caramel", 
        "Chocolate truffle", 
        "Maple bacon",
        "Butter pecan"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "Salted caramel! Sweet and savory perfection!");
        dialog_text(CHARACTERS_FATMAN, "Salted caramel is the greatest invention of our generation.");
        dialog_text(CHARACTERS_SALESMAN, "A man of refined taste!");
        dialog_text(CHARACTERS_FATMAN, "I refined my taste through years of dedicated eating. It's a craft.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "Chocolate truffle! Rich, decadent, creamy!");
        dialog_text(CHARACTERS_FATMAN, "Chocolate is medicine. It heals the soul.");
        dialog_text(CHARACTERS_SALESMAN, "A profound truth!");
        dialog_text(CHARACTERS_FATMAN, "I'm full of profound truths. And chocolate.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "Maple bacon! Two breakfast favorites in one!");
        dialog_text(CHARACTERS_FATMAN, "Maple AND bacon? At the same time? This is visionary.");
        dialog_text(CHARACTERS_SALESMAN, "The perfect flavor combination!");
        dialog_text(CHARACTERS_FATMAN, "I've had dreams about this. Literal dreams.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "Butter pecan! Nutty, buttery, delightful!");
        dialog_text(CHARACTERS_FATMAN, "Butter pecan reminds me of my grandmother's cookies.");
        dialog_text(CHARACTERS_SALESMAN, "Nostalgic and delicious!");
        dialog_text(CHARACTERS_FATMAN, "She was a good woman. Made cookies every day. That's why I am who I am.");
        break;
    }
    
    dialog_text(CHARACTERS_FATMAN, "Let me sample some of these. For research purposes.");
    dialog_text(CHARACTERS_SALESMAN, "Of course! Here's an assortment!");
    
    encounter_minigame(&rhythm_game);
    
    dialog_text(CHARACTERS_FATMAN, "These are excellent. High marks across the board.");
    dialog_text(CHARACTERS_SALESMAN, "I'm glad you approve!");
    dialog_text(CHARACTERS_FATMAN, "I'll take five of each flavor. Bulk is more efficient.");
    dialog_text(CHARACTERS_SALESMAN, "Five each? That's a lot of lollipops!");
    dialog_text(CHARACTERS_FATMAN, "It's a starting point. I may order more later.");
    
    encounter_end();
}

void encounter_fatman_computer(void) {
    encounter_begin();
    
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! Can I interest you in a new computer?");
    dialog_text(CHARACTERS_FATMAN, "Computer? What would I use it for?");
    dialog_text(CHARACTERS_SALESMAN, "Oh, lots of things! Email, browsing, productivity—");
    dialog_text(CHARACTERS_FATMAN, "I'm not interested in productivity. What else?");
    dialog_text(CHARACTERS_SALESMAN, "Entertainment! Movies, shows, food delivery apps!");
    dialog_text(CHARACTERS_FATMAN, "Food delivery apps? Now we're talking.");
    dialog_text(CHARACTERS_SALESMAN, "You can order from any restaurant without getting up!");
    dialog_text(CHARACTERS_FATMAN, "Not getting up is my favorite activity.");
    
    switch (dialog_selection("Highlight features:", 
        dialog_options("Streaming capabilities", 
        "Food delivery convenience", 
        "Large screen for recipes",
        "Gaming potential"))) {
    case 0:
        dialog_text(CHARACTERS_SALESMAN, "All the streaming services! Endless shows and movies!");
        dialog_text(CHARACTERS_FATMAN, "Endless? That's a lot of couch time. I'm interested.");
        dialog_text(CHARACTERS_SALESMAN, "Multiple monitors means multiple shows at once!");
        dialog_text(CHARACTERS_FATMAN, "Multitasking entertainment. Peak efficiency.");
        break;
    case 1:
        dialog_text(CHARACTERS_SALESMAN, "Five food delivery apps at your fingertips!");
        dialog_text(CHARACTERS_FATMAN, "Five? I only use three. Which other two?");
        dialog_text(CHARACTERS_SALESMAN, "I can show you! One specializes in desserts only!");
        dialog_text(CHARACTERS_FATMAN, "A dedicated dessert app? The future is now.");
        break;
    case 2:
        dialog_text(CHARACTERS_SALESMAN, "The screen is big enough to read recipes while cooking!");
        dialog_text(CHARACTERS_FATMAN, "I usually cook from memory. But having recipes nearby isn't bad.");
        dialog_text(CHARACTERS_SALESMAN, "You can even watch cooking videos!");
        dialog_text(CHARACTERS_FATMAN, "Learn from the masters. Wise.");
        break;
    case 3:
        dialog_text(CHARACTERS_SALESMAN, "This can run the latest games! Farming simulators, cooking games—");
        dialog_text(CHARACTERS_FATMAN, "Cooking games? I can simulate cooking without the cleanup?");
        dialog_text(CHARACTERS_SALESMAN, "Exactly! All the joy, none of the dishes!");
        dialog_text(CHARACTERS_FATMAN, "You've just changed my life philosophy.");
        break;
    }
    
    dialog_text(CHARACTERS_FATMAN, "Alright, show me how this food delivery thing works.");
    dialog_text(CHARACTERS_SALESMAN, "Watch this! Just a few clicks and—");
    
    encounter_minigame(&rhythm_game);
    
    dialog_text(CHARACTERS_FATMAN, "Amazing. I just ordered a pizza without moving anything but my fingers.");
    dialog_text(CHARACTERS_SALESMAN, "The magic of technology!");
    dialog_text(CHARACTERS_FATMAN, "I'll take one. Set it up right here, by the couch.");
    dialog_text(CHARACTERS_SALESMAN, "Perfect placement for maximum convenience.");
    dialog_text(CHARACTERS_FATMAN, "You understand me, salesman. Few people do.");
    dialog_text(CHARACTERS_SALESMAN, "It's my job to understand needs.");
    dialog_text(CHARACTERS_FATMAN, "My needs are simple. Comfort and calories. This delivers both.");
    
    encounter_end();
}


// ============================================================================
// DOOR-TO-DOOR SALESMAN - DIALOG ENCOUNTERS
// 12 Unique Encounters (4 Characters x 3 Items)
// ============================================================================

// ----------------------------------------------------------------------------
// OLD LADY ENCOUNTERS
// ----------------------------------------------------------------------------

void encounter_oldlady_vase(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "Good afternoon, ma'am! Lovely garden you have out front.");
    dialog_text(CHARACTERS_OLDLADY, "Oh, thank you, dear. My Herbert planted those roses before he passed. That was... goodness, thirty years ago now.");
    dialog_text(CHARACTERS_SALESMAN, "Well, he had wonderful taste. And speaking of taste, I couldn't help but notice your mantle looks a little bare.");
    dialog_text(CHARACTERS_OLDLADY, "My mantle?");
    dialog_text(CHARACTERS_SALESMAN, "Yes, ma'am. What if I told you I had the perfect thing to fill that space? A hand-crafted porcelain vase, imported all the way from the finest kilns in Europe.");
    dialog_text(CHARACTERS_OLDLADY, "Oh my. Let me get my glasses.");
    dialog_text(CHARACTERS_SALESMAN, "Take your time.");
    dialog_text(CHARACTERS_OLDLADY, "Oh, that IS lovely. You know, I had a vase just like this once. My cat Harold knocked it off the shelf in 1987. I cried for a week.");
    dialog_text(CHARACTERS_SALESMAN, "Well, this could be Harold's way of making amends from beyond the grave.");
    dialog_text(CHARACTERS_OLDLADY, "Harold isn't dead, dear. He's in the kitchen.");

    switch (dialog_selection("Respond to the cat situation",
        dialog_options("That's one old cat!",
        "A different Harold?",
        "Cats and vases don't mix..."))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "That is one impressively old cat!");
            dialog_text(CHARACTERS_OLDLADY, "Oh, it's a different Harold. I name all my cats Harold. I'm on Harold the sixth now.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Wait, a different Harold?");
            dialog_text(CHARACTERS_OLDLADY, "Of course, dear. Cats don't live that long. This is Harold the sixth. Very well behaved compared to the third one.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "Well, you know, cats and vases have a bit of a complicated history...");
            dialog_text(CHARACTERS_OLDLADY, "Oh, don't worry about Harold. He mostly just sleeps on the heating vent these days. Hasn't knocked anything over in months.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Well then, this vase deserves a good home. And I think your mantle deserves this vase. What do you say we work out a deal?");
    dialog_text(CHARACTERS_OLDLADY, "Hmm, I don't know. Let me think about it. Can you do something to convince me?");

    encounter_minigame(&smile_game);

    dialog_text(CHARACTERS_OLDLADY, "Oh, well isn't that something! You remind me of my nephew. He's very persuasive too. Talked me into buying a timeshare in 2003.");
    dialog_text(CHARACTERS_SALESMAN, "I promise this is a much better investment than a timeshare.");
    dialog_text(CHARACTERS_OLDLADY, "That's a low bar, dear. But yes, I'll take it. Let me find my purse. It's around here somewhere...");
    dialog_text(CHARACTERS_SALESMAN, "Pleasure doing business with you, ma'am. You and Harold enjoy that vase.");

    encounter_end();
}

void encounter_oldlady_comicbook(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! How are we doing today?");
    dialog_text(CHARACTERS_OLDLADY, "Oh, hello there. I was just watching my stories. You made me miss whether Valentina found out about the affair.");
    dialog_text(CHARACTERS_SALESMAN, "My deepest apologies. But I think I have something even more dramatic than daytime television.");
    dialog_text(CHARACTERS_OLDLADY, "I sincerely doubt that, young man.");
    dialog_text(CHARACTERS_SALESMAN, "Feast your eyes on this. A genuine, mint condition comic book. We're talking a classic piece of American storytelling right here.");
    dialog_text(CHARACTERS_OLDLADY, "A comic book? Honey, I haven't read one of those since Eisenhower was in office.");
    dialog_text(CHARACTERS_SALESMAN, "That's what makes this special! It's a collector's item. This thing appreciates in value every single year.");
    dialog_text(CHARACTERS_OLDLADY, "So does my hip replacement, but I wouldn't call that exciting.");

    switch (dialog_selection("Pitch angle",
        dialog_options("It's a great investment",
        "Your grandkids would love it",
        "It's a piece of history"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Think of it as an investment, ma'am. This comic could be worth ten times what you pay today.");
            dialog_text(CHARACTERS_OLDLADY, "That's what my financial advisor said about Enron.");
            dialog_text(CHARACTERS_SALESMAN, "This is significantly less likely to commit fraud.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Think about it. Your grandkids would go absolutely wild for this.");
            dialog_text(CHARACTERS_OLDLADY, "My grandchildren only care about their phones and that TikTak thing.");
            dialog_text(CHARACTERS_SALESMAN, "TikTok. And trust me, vintage comics are making a huge comeback with kids.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "This isn't just a comic book, ma'am. It's a piece of American history. Right up there with apple pie and the moon landing.");
            dialog_text(CHARACTERS_OLDLADY, "I watched the moon landing live, you know. On a twelve-inch television set. Now THAT was something.");
            dialog_text(CHARACTERS_SALESMAN, "And this comic is from that same golden era!");
        } break;
    }

    dialog_text(CHARACTERS_OLDLADY, "Well, you're certainly persistent. I suppose that counts for something. Go ahead, make your case.");

    encounter_minigame(&smile_game);

    dialog_text(CHARACTERS_OLDLADY, "You know what, you've charmed me. I'll buy the silly thing. Maybe I'll read it during commercials.");
    dialog_text(CHARACTERS_SALESMAN, "You won't regret it! It's a real page-turner.");
    dialog_text(CHARACTERS_OLDLADY, "Just don't tell anyone at my bridge club. Marge already thinks I'm losing it.");

    encounter_end();
}

void encounter_oldlady_football(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "Afternoon, ma'am! Beautiful day, isn't it?");
    dialog_text(CHARACTERS_OLDLADY, "It's raining, dear.");
    dialog_text(CHARACTERS_SALESMAN, "Beautiful rain, though. Anyway, I have something here that's going to bring some real excitement into your life.");
    dialog_text(CHARACTERS_OLDLADY, "At my age, excitement usually means a doctor's appointment.");
    dialog_text(CHARACTERS_SALESMAN, "Not today! Today, I bring you this. A genuine, regulation-size football. Feel that leather. Go ahead.");
    dialog_text(CHARACTERS_OLDLADY, "A football? Young man, do I look like I play football?");
    dialog_text(CHARACTERS_SALESMAN, "Ma'am, football is for everyone. It's about the spirit of competition, the thrill of the game.");
    dialog_text(CHARACTERS_OLDLADY, "The last time I threw something, it was a shoe at a raccoon in my garbage.");

    switch (dialog_selection("Selling strategy",
        dialog_options("Perfect arm for football!",
        "It's decorative too",
        "Great gift for someone"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "See? You've already got the arm for it! Shoe, football, same throwing motion.");
            dialog_text(CHARACTERS_OLDLADY, "I missed the raccoon.");
            dialog_text(CHARACTERS_SALESMAN, "That's what practice is for!");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "And even if you're not tossing it around, it looks fantastic on a shelf. Very distinguished.");
            dialog_text(CHARACTERS_OLDLADY, "I do have that empty shelf since Harold knocked the vase off...");
            dialog_text(CHARACTERS_SALESMAN, "A football is much harder for a cat to knock over. It's practically cat-proof.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "And hey, even if it's not your thing, it makes a fantastic gift. Any grandkids?");
            dialog_text(CHARACTERS_OLDLADY, "Seven. They're all in ballet.");
            dialog_text(CHARACTERS_SALESMAN, "Ballet requires incredible footwork. This is basically cross-training.");
        } break;
    }

    dialog_text(CHARACTERS_OLDLADY, "You are the most creative liar I've ever met. And I was married twice.");
    dialog_text(CHARACTERS_SALESMAN, "I prefer the term 'optimistic reframer.' So what do you say? Give me a shot?");

    encounter_minigame(&smile_game);

    dialog_text(CHARACTERS_OLDLADY, "Oh, fine. I'll take the football. If nothing else, I can throw it at the raccoon next time.");
    dialog_text(CHARACTERS_SALESMAN, "Now THAT'S the spirit.");
    dialog_text(CHARACTERS_OLDLADY, "Don't push your luck, dear.");

    encounter_end();
}

// ----------------------------------------------------------------------------
// NERD ENCOUNTERS
// ----------------------------------------------------------------------------

void encounter_nerd_vase(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "Hey there! Got a minute?");
    dialog_text(CHARACTERS_NERD, "You have approximately ninety seconds before my raid group needs me back online, so talk fast.");
    dialog_text(CHARACTERS_SALESMAN, "A man who values efficiency. I respect that. Check this out. Handcrafted porcelain vase, one of a kind.");
    dialog_text(CHARACTERS_NERD, "A vase. You came to my door to sell me a vase.");
    dialog_text(CHARACTERS_SALESMAN, "Not just any vase. Look at the craftsmanship. The glaze on this thing is flawless.");
    dialog_text(CHARACTERS_NERD, "It does have a surprisingly high poly count. I mean, nice curvature. Whatever.");
    dialog_text(CHARACTERS_SALESMAN, "See, you've got an eye for quality!");
    dialog_text(CHARACTERS_NERD, "I've got an eye for polygon meshes, which is a completely different thing.");

    switch (dialog_selection("Appeal to the nerd",
        dialog_options("It's like a rare drop",
        "Great for your setup",
        "It's a limited edition"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Think of it like a legendary drop. You wouldn't pass up a legendary drop, would you?");
            dialog_text(CHARACTERS_NERD, "That depends entirely on the stat distribution. What are the stats on this vase?");
            dialog_text(CHARACTERS_SALESMAN, "Plus ten to interior decoration. Plus five to impressing guests.");
            dialog_text(CHARACTERS_NERD, "Those aren't real stats, but I appreciate the format.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Picture this on your desk, right next to your monitors. It classes up the whole setup.");
            dialog_text(CHARACTERS_NERD, "My desk has three monitors, a mechanical keyboard, and a figurine of Commander Shepard. There's no room.");
            dialog_text(CHARACTERS_SALESMAN, "Commander Shepard would want you to have this vase. It's a Paragon choice.");
            dialog_text(CHARACTERS_NERD, "Okay, don't use Shepard against me. That's not fair.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "This is a limited edition piece. They only made a handful. You know how limited runs work.");
            dialog_text(CHARACTERS_NERD, "I literally camped outside a store for six hours for a limited edition graphics card, so yes. I understand scarcity economics on a molecular level.");
            dialog_text(CHARACTERS_SALESMAN, "Then you know this is an opportunity you don't pass up.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Come on, what do you say? Live a little. Buy the vase.");
    dialog_text(CHARACTERS_NERD, "Fine. Prove to me you're worth buying from.");

    encounter_minigame(&memory_game);

    dialog_text(CHARACTERS_NERD, "Alright, I'm mildly impressed. I'll take it. But if my raid group asks, this was an ironic purchase.");
    dialog_text(CHARACTERS_SALESMAN, "Your secret's safe with me.");
    dialog_text(CHARACTERS_NERD, "It better be. I have a reputation to maintain.");

    encounter_end();
}

void encounter_nerd_comicbook(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "Hey! You look like a man of culture.");
    dialog_text(CHARACTERS_NERD, "That's either a compliment or a very targeted sales tactic. Either way, I'm listening.");
    dialog_text(CHARACTERS_SALESMAN, "Feast your eyes on this. Mint condition comic book. Original print run.");
    dialog_text(CHARACTERS_NERD, "Oh. Oh wow. Is that... let me see that.");
    dialog_text(CHARACTERS_SALESMAN, "Careful now. This thing is practically sacred.");
    dialog_text(CHARACTERS_NERD, "I know what this is. Do you know what this is? Because I don't think you know what this is.");
    dialog_text(CHARACTERS_SALESMAN, "It's a very nice comic book that I would love to sell you for a very reasonable price.");
    dialog_text(CHARACTERS_NERD, "A 'very nice comic book.' That's like calling the Mona Lisa a 'pretty okay painting.'");

    switch (dialog_selection("Handle the nerd's expertise",
        dialog_options("Enlighten me then",
        "I know exactly what it is",
        "Does that mean you want it?"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Alright, I can see you're the expert here. Enlighten me.");
            dialog_text(CHARACTERS_NERD, "This is a first print from the original run. The coloring on the cover alone suggests it's been stored properly, away from UV light. The spine isn't rolled. You could probably get a 9.2 grade on this. Maybe higher.");
            dialog_text(CHARACTERS_SALESMAN, "So what you're saying is... you want it?");
            dialog_text(CHARACTERS_NERD, "What I'm saying is you're undercharging. Which, as the buyer, I'm very okay with.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Of course I know what it is. I'm a professional.");
            dialog_text(CHARACTERS_NERD, "Really? What issue number is it?");
            dialog_text(CHARACTERS_SALESMAN, "It's the... the really good one.");
            dialog_text(CHARACTERS_NERD, "Yeah, that's what I thought.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "The real question is: do you want it?");
            dialog_text(CHARACTERS_NERD, "Do I WANT it? I've been searching for this issue for three years. I've got alerts set up on six different auction sites. I once flew to a convention in Ohio on a rumor that someone had a copy.");
            dialog_text(CHARACTERS_SALESMAN, "So that's a yes?");
            dialog_text(CHARACTERS_NERD, "Don't rush me. I need a moment.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Look, this comic and you were clearly meant to be together. Let me make this happen.");
    dialog_text(CHARACTERS_NERD, "Okay. Okay. Show me what you've got.");

    encounter_minigame(&memory_game);

    dialog_text(CHARACTERS_NERD, "Take my money. Take it right now before I change my mind or realize I need to eat this month.");
    dialog_text(CHARACTERS_SALESMAN, "A wise investment! You won't regret this.");
    dialog_text(CHARACTERS_NERD, "I'll need to recalibrate my humidity-controlled display case. This changes everything.");
    dialog_text(CHARACTERS_SALESMAN, "You have a humidity-controlled display case?");
    dialog_text(CHARACTERS_NERD, "I have three. Don't judge me.");

    encounter_end();
}

void encounter_nerd_football(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "What's up, my man? Got something awesome for you today.");
    dialog_text(CHARACTERS_NERD, "If it's not a graphics card or a first edition anything, I'm already skeptical.");
    dialog_text(CHARACTERS_SALESMAN, "Even better. Check out this beauty. Genuine leather football.");
    dialog_text(CHARACTERS_NERD, "...");
    dialog_text(CHARACTERS_SALESMAN, "Pretty sweet, right?");
    dialog_text(CHARACTERS_NERD, "You're joking. You came to THIS house, looked at the anime welcome mat, saw the LED strip lighting through the window, and thought 'this guy needs a football.'");
    dialog_text(CHARACTERS_SALESMAN, "Hey, don't knock it until you try it. You might be a natural.");
    dialog_text(CHARACTERS_NERD, "The last time I was involved with a football, it hit me in the face during gym class. In 2007. I still flinch when I hear a whistle.");

    switch (dialog_selection("Try to connect football to nerd culture",
        dialog_options("Football is basically strategy gaming",
        "Athletes are basically real-life RPG characters",
        "You could study the physics"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Think about it. Football is just real-time strategy with a bigger budget. Formations, play-calling, countering the opponent's moves...");
            dialog_text(CHARACTERS_NERD, "Huh. I never thought of the quarterback as a raid leader before, but you might be onto something.");
            dialog_text(CHARACTERS_SALESMAN, "See? Same skills. Different arena.");
            dialog_text(CHARACTERS_NERD, "The meta would be way harder to patch though.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Think about football players. They've got stats, they've got rankings, they've got build diversity. They're basically RPG characters.");
            dialog_text(CHARACTERS_NERD, "So you're saying a linebacker is just a strength-build tank?");
            dialog_text(CHARACTERS_SALESMAN, "Exactly. And the wide receiver is your glass cannon DPS.");
            dialog_text(CHARACTERS_NERD, "I hate that this makes sense to me.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "A guy like you could analyze the aerodynamics. The spiral, the trajectory calculations, the air resistance coefficients...");
            dialog_text(CHARACTERS_NERD, "A prolate spheroid does have interesting flight characteristics due to its asymmetric drag profile.");
            dialog_text(CHARACTERS_SALESMAN, "Right! What you said! So you're practically already into football.");
            dialog_text(CHARACTERS_NERD, "I'm into fluid dynamics. Those are VERY different things.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Come on, step outside your comfort zone. Every great character arc needs a twist.");
    dialog_text(CHARACTERS_NERD, "Did you just use narrative structure to sell me a football?");
    dialog_text(CHARACTERS_SALESMAN, "Is it working?");
    dialog_text(CHARACTERS_NERD, "Ugh. Maybe. Fine, let's see what you've got.");

    encounter_minigame(&memory_game);

    dialog_text(CHARACTERS_NERD, "Okay. I'll buy it. But purely as a physics experiment.");
    dialog_text(CHARACTERS_SALESMAN, "Whatever gets you to the end zone, my friend.");
    dialog_text(CHARACTERS_NERD, "I'm going to set up a high-speed camera and analyze the spiral dynamics.");
    dialog_text(CHARACTERS_SALESMAN, "That's the spirit! Kind of.");

    encounter_end();
}

// ----------------------------------------------------------------------------
// SHOTGUNNER ENCOUNTERS
// ----------------------------------------------------------------------------

void encounter_shotgunner_vase(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "Hello? Anybody ho--");
    dialog_text(CHARACTERS_SHOTGUNNER, "That's far enough. State your business.");
    dialog_text(CHARACTERS_SALESMAN, "Whoa, easy there! I'm just a salesman. See? Briefcase, tie, no weapons. Completely harmless.");
    dialog_text(CHARACTERS_SHOTGUNNER, "That's exactly what the last guy said. Turned out he was from the IRS.");
    dialog_text(CHARACTERS_SALESMAN, "Well, I am definitely not from the IRS. I'm here to show you something beautiful. A handcrafted porcelain vase.");
    dialog_text(CHARACTERS_SHOTGUNNER, "A vase.");
    dialog_text(CHARACTERS_SALESMAN, "A vase! Elegant, refined, a real conversation piece.");
    dialog_text(CHARACTERS_SHOTGUNNER, "I don't have conversations. I have confrontations.");

    switch (dialog_selection("Try to find an angle",
        dialog_options("Every home needs some class",
        "It could hold your ammo",
        "Your wife would love it"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Every home needs a touch of class, sir. Even a... fortified one.");
            dialog_text(CHARACTERS_SHOTGUNNER, "My home has plenty of class. I've got a mounted bass over the fireplace and a flag on the porch.");
            dialog_text(CHARACTERS_SALESMAN, "And a vase would tie the whole room together! Think of it as the finishing touch.");
            dialog_text(CHARACTERS_SHOTGUNNER, "Hmm. It WOULD fill that gap between the ammo shelf and the deer antlers.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Think about the practical applications. You could store things in it. Shells, cartridges, whatever you need handy.");
            dialog_text(CHARACTERS_SHOTGUNNER, "I already have an ammo can for that.");
            dialog_text(CHARACTERS_SALESMAN, "Sure, but does your ammo can have hand-painted floral patterns?");
            dialog_text(CHARACTERS_SHOTGUNNER, "...No. No it does not.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "I bet your wife would absolutely love this.");
            dialog_text(CHARACTERS_SHOTGUNNER, "My ex-wife took everything in the divorce. Including my last vase. And my dog.");
            dialog_text(CHARACTERS_SALESMAN, "Then this is a fresh start! A new vase for a new chapter.");
            dialog_text(CHARACTERS_SHOTGUNNER, "You're lucky I respect a man who can pivot that fast.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Look, just give me a chance here. I think you'll be surprised.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Fine. But if this is a waste of my time, you're leaving faster than you came.");

    encounter_minigame(&shotgun_game);

    dialog_text(CHARACTERS_SHOTGUNNER, "Huh. Not bad. Alright, I'll take the vase. But I'm paying cash and I don't want a receipt.");
    dialog_text(CHARACTERS_SALESMAN, "Cash is king! Pleasure doing business.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Now get off my property before I change my mind.");
    dialog_text(CHARACTERS_SALESMAN, "Already leaving. Have a great day, sir!");

    encounter_end();
}

void encounter_shotgunner_comicbook(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "Good afternoon, sir! I--");
    dialog_text(CHARACTERS_SHOTGUNNER, "There a sign at the end of my driveway. Did you read it?");
    dialog_text(CHARACTERS_SALESMAN, "The one that says 'No Trespassing, Survivors Will Be Prosecuted'? I assumed that was a joke.");
    dialog_text(CHARACTERS_SHOTGUNNER, "It ain't.");
    dialog_text(CHARACTERS_SALESMAN, "Well, before any prosecution happens, can I interest you in a genuine mint condition comic book?");
    dialog_text(CHARACTERS_SHOTGUNNER, "A comic book. You walked past my sign, up my driveway, and past my security cameras to sell me a comic book.");
    dialog_text(CHARACTERS_SALESMAN, "It's a really good comic book.");
    dialog_text(CHARACTERS_SHOTGUNNER, "I haven't read a comic book since I was in the service. We used to trade them around on base.");

    switch (dialog_selection("Explore his military connection",
        dialog_options("Which branch?",
        "Comics and military go way back",
        "This one has a soldier in it"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "You served? Which branch, if you don't mind me asking?");
            dialog_text(CHARACTERS_SHOTGUNNER, "Marines. Two tours. You?");
            dialog_text(CHARACTERS_SALESMAN, "Sales. Fourteen tours of the tri-county area.");
            dialog_text(CHARACTERS_SHOTGUNNER, "That's not even close to the same thing, but at least you've got some guts showing up here.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Comics and the military go way back. They used to hand these out to troops overseas. Piece of home, you know?");
            dialog_text(CHARACTERS_SHOTGUNNER, "Yeah. I remember. Some kid from Iowa had a whole footlocker full of them. We'd read them on watch when nothing was happening.");
            dialog_text(CHARACTERS_SALESMAN, "See? This isn't just a comic. It's a connection to those memories.");
            dialog_text(CHARACTERS_SHOTGUNNER, "Don't go getting sentimental on me.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "And get this, there's a military character in this issue. Real tactical stuff.");
            dialog_text(CHARACTERS_SHOTGUNNER, "Let me see that. ...This guy's holding his rifle wrong.");
            dialog_text(CHARACTERS_SALESMAN, "Well, he's a superhero. Maybe different rules apply?");
            dialog_text(CHARACTERS_SHOTGUNNER, "Trigger discipline applies to everyone, super or not.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "So what do you think? Want to add this to your collection?");
    dialog_text(CHARACTERS_SHOTGUNNER, "I don't have a collection. I have a house full of practical items and one weird salesman on my porch.");
    dialog_text(CHARACTERS_SALESMAN, "Let me change your mind.");

    encounter_minigame(&shotgun_game);

    dialog_text(CHARACTERS_SHOTGUNNER, "Alright. You earned it. I'll take the comic. Haven't had something to read besides the survivalist newsletter in months.");
    dialog_text(CHARACTERS_SALESMAN, "Excellent choice! Enjoy the read.");
    dialog_text(CHARACTERS_SHOTGUNNER, "If the ending's bad, I'm finding you.");
    dialog_text(CHARACTERS_SALESMAN, "I'm sure you'll love it. Goodbye now.");

    encounter_end();
}

void encounter_shotgunner_football(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "Hey there! Nice property you've got. Very... secure.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Barbed wire was on sale at the hardware store. What do you want?");
    dialog_text(CHARACTERS_SALESMAN, "Just a moment of your time and a chance to show you something great. A genuine leather football.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Football? Now you're talking a language I understand.");
    dialog_text(CHARACTERS_SALESMAN, "Oh yeah? You a fan?");
    dialog_text(CHARACTERS_SHOTGUNNER, "Played linebacker in high school. All-district, three years running. Coach said I hit like a freight train with anger issues.");
    dialog_text(CHARACTERS_SALESMAN, "That is... both impressive and terrifying.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Best four years of my life. Before the government started putting fluoride in the water.");

    switch (dialog_selection("Keep the football talk going",
        dialog_options("You've still got the arm, I bet",
        "This is game-quality leather",
        "Perfect for the backyard"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "A guy like you? I bet you've still got the arm.");
            dialog_text(CHARACTERS_SHOTGUNNER, "Threw a trespasser's briefcase over my fence last month. Clean spiral. Forty yards easy.");
            dialog_text(CHARACTERS_SALESMAN, "That's... wait, was that a salesman's briefcase?");
            dialog_text(CHARACTERS_SHOTGUNNER, "Might've been. They all start to blur together.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Feel that leather. This is the real deal. Game-quality material, perfect grip, regulation weight.");
            dialog_text(CHARACTERS_SHOTGUNNER, "Hmm. Good laces. Decent pebbling on the leather.");
            dialog_text(CHARACTERS_SALESMAN, "You clearly know your way around a football.");
            dialog_text(CHARACTERS_SHOTGUNNER, "I know my way around a lot of things. That's why I'm still here and the trespassers aren't.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "With a property this big, you've got your own personal football field out back.");
            dialog_text(CHARACTERS_SHOTGUNNER, "That's not a field. That's a perimeter zone.");
            dialog_text(CHARACTERS_SALESMAN, "A perimeter zone that's perfect for throwing a football!");
            dialog_text(CHARACTERS_SHOTGUNNER, "I suppose it would be a good way to stay sharp. Moving targets aren't the only thing worth tracking.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "So, linebacker, what do you say? Want to relive the glory days?");
    dialog_text(CHARACTERS_SHOTGUNNER, "Glory days never left. But yeah, show me what you've got.");

    encounter_minigame(&shotgun_game);

    dialog_text(CHARACTERS_SHOTGUNNER, "Sold. I'll take it. Haven't had a good football in years. My nephew comes by sometimes. Kid's got no arm, but he tries.");
    dialog_text(CHARACTERS_SALESMAN, "That's great! Family bonding through football.");
    dialog_text(CHARACTERS_SHOTGUNNER, "And if nobody's around, I can use it for target practice.");
    dialog_text(CHARACTERS_SALESMAN, "That's... one way to use it. Enjoy!");

    encounter_end();
}

// ----------------------------------------------------------------------------
// FATMAN ENCOUNTERS
// ----------------------------------------------------------------------------

void encounter_fatman_vase(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "Hello? Anyone home?");
    dialog_text(CHARACTERS_FATMAN, "Hold on... hold on, I'm coming. Give me a second.");
    dialog_text(CHARACTERS_SALESMAN, "Take your time!");
    dialog_text(CHARACTERS_FATMAN, "Alright. I'm here. This better be good because I paused my show and left a perfectly good sandwich on the couch.");
    dialog_text(CHARACTERS_SALESMAN, "It is absolutely worth your time, sir. I've got a stunning porcelain vase here that would look incredible in your home.");
    dialog_text(CHARACTERS_FATMAN, "A vase? What am I gonna do with a vase?");
    dialog_text(CHARACTERS_SALESMAN, "Display it! It's a centerpiece. It ties a room together.");
    dialog_text(CHARACTERS_FATMAN, "My coffee table ties my room together. It's where I put my food. Everything revolves around the food.");

    switch (dialog_selection("Appeal to his lifestyle",
        dialog_options("Put snacks in it",
        "It'll impress your friends",
        "It's a conversation starter"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "You could use it to hold snacks. Breadsticks, pretzel rods, those long wafer cookies...");
            dialog_text(CHARACTERS_FATMAN, "Wait. You can put food in a vase?");
            dialog_text(CHARACTERS_SALESMAN, "You can put whatever you want in a vase. It's YOUR vase.");
            dialog_text(CHARACTERS_FATMAN, "That's the most exciting thing anyone's said to me all week.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "When you have people over, this vase says 'I'm a man of taste and sophistication.'");
            dialog_text(CHARACTERS_FATMAN, "The only people who come over are the pizza delivery guys. And my buddy Doug. Doug doesn't care about vases.");
            dialog_text(CHARACTERS_SALESMAN, "Maybe Doug doesn't know he cares about vases yet. You could be the one to open his eyes.");
            dialog_text(CHARACTERS_FATMAN, "Doug's eyes are usually half closed. But I see your point.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "It's a real conversation starter. People see this vase and they want to know the story behind it.");
            dialog_text(CHARACTERS_FATMAN, "The story would be 'some guy knocked on my door and I was too tired to say no.'");
            dialog_text(CHARACTERS_SALESMAN, "Or, 'I have an eye for fine porcelain.' Sounds better, right?");
            dialog_text(CHARACTERS_FATMAN, "I mean, it does sound better than most things I say.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Come on, what do you say? Let me show you what I can do.");
    dialog_text(CHARACTERS_FATMAN, "Fine. But make it quick. My sandwich is getting cold. Well, it's a cold sandwich. But it's getting warm. Which is worse.");

    encounter_minigame(&rhythm_game);

    dialog_text(CHARACTERS_FATMAN, "You know what, sure. I'll take the vase. I'm gonna put cheese puffs in it.");
    dialog_text(CHARACTERS_SALESMAN, "A bold choice. I support it fully.");
    dialog_text(CHARACTERS_FATMAN, "It'll be the fanciest cheese puff container on the block.");
    dialog_text(CHARACTERS_SALESMAN, "Without a doubt. Enjoy, sir!");

    encounter_end();
}

void encounter_fatman_comicbook(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "Howdy! Hope I'm not catching you at a bad time.");
    dialog_text(CHARACTERS_FATMAN, "You're catching me in the middle of a meatball sub, so the clock is ticking. What've you got?");
    dialog_text(CHARACTERS_SALESMAN, "Something that pairs great with a meatball sub. A mint condition comic book!");
    dialog_text(CHARACTERS_FATMAN, "Nothing pairs with a meatball sub except another meatball sub.");
    dialog_text(CHARACTERS_SALESMAN, "Fair point. But picture this: you're on the couch, sub in one hand, comic book in the other. That's a perfect Saturday right there.");
    dialog_text(CHARACTERS_FATMAN, "I usually have my phone in the other hand. Scrolling through restaurant menus.");
    dialog_text(CHARACTERS_SALESMAN, "But a comic book is way more engaging! Superheroes, action, drama, all without needing to charge a battery.");
    dialog_text(CHARACTERS_FATMAN, "Does the superhero in this one eat? I find it hard to relate to characters who don't eat.");

    switch (dialog_selection("Address his food concern",
        dialog_options("There's definitely a diner scene",
        "Heroes need fuel too",
        "You could eat while you read"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Oh absolutely. There's a scene in a diner. Very atmospheric. I think there's pie involved.");
            dialog_text(CHARACTERS_FATMAN, "What kind of pie?");
            dialog_text(CHARACTERS_SALESMAN, "The... heroic kind?");
            dialog_text(CHARACTERS_FATMAN, "I'll accept that answer.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Every hero needs fuel! You think these guys fight evil on an empty stomach? No way.");
            dialog_text(CHARACTERS_FATMAN, "That's actually a really good point. You can't throw a punch if your blood sugar's low.");
            dialog_text(CHARACTERS_SALESMAN, "Exactly. This comic understands the importance of proper caloric intake.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "And the beautiful thing about a comic book is you can eat while you read it. Way easier than holding a phone with marinara sauce on your fingers.");
            dialog_text(CHARACTERS_FATMAN, "You make an extremely valid point. I cracked my phone screen with a chicken wing once.");
            dialog_text(CHARACTERS_SALESMAN, "A comic book can survive a chicken wing. Trust me on this.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "So what do you say? Add some entertainment to your next meal?");
    dialog_text(CHARACTERS_FATMAN, "My meals are already entertaining. But sure, let's see what you've got.");

    encounter_minigame(&rhythm_game);

    dialog_text(CHARACTERS_FATMAN, "Alright, sold. I need something to read anyway. I've memorized every takeout menu within a five-mile radius.");
    dialog_text(CHARACTERS_SALESMAN, "That's an impressive radius.");
    dialog_text(CHARACTERS_FATMAN, "Thank you. It took years of dedication. Now if you'll excuse me, my sub is calling.");
    dialog_text(CHARACTERS_SALESMAN, "Enjoy the comic and the sub!");

    encounter_end();
}

void encounter_fatman_football(void) {
    encounter_begin();

    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! You look like a man who appreciates the finer things in life.");
    dialog_text(CHARACTERS_FATMAN, "If by 'finer things' you mean the double bacon cheeseburger I just ordered, then yes. Absolutely.");
    dialog_text(CHARACTERS_SALESMAN, "I love the energy. But I've got something here that might surprise you. A genuine leather football!");
    dialog_text(CHARACTERS_FATMAN, "A football. Buddy, look at me. Do I look like I run routes?");
    dialog_text(CHARACTERS_SALESMAN, "You don't have to run routes to appreciate a quality football!");
    dialog_text(CHARACTERS_FATMAN, "The last time I ran was when I heard the ice cream truck in 2019. And I didn't even catch it.");
    dialog_text(CHARACTERS_SALESMAN, "That's heartbreaking.");
    dialog_text(CHARACTERS_FATMAN, "It still keeps me up at night.");

    switch (dialog_selection("Find the right angle",
        dialog_options("You can be an armchair quarterback",
        "Football watching is better with one",
        "Toss it around casually"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "You don't need to play to be a football guy. Armchair quarterbacking is a proud American tradition.");
            dialog_text(CHARACTERS_FATMAN, "I do yell at the TV a lot. My neighbors have complained.");
            dialog_text(CHARACTERS_SALESMAN, "Having an actual football in your hands while you do it? That's the premium experience. You're not just watching the game, you're PART of it.");
            dialog_text(CHARACTERS_FATMAN, "I do like the idea of holding something during the game that isn't a remote or a plate.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Picture this: Sunday afternoon, big game on TV, snacks on the table, and a real football sitting right there on the couch with you.");
            dialog_text(CHARACTERS_FATMAN, "You paint a beautiful picture. Would the snacks be included?");
            dialog_text(CHARACTERS_SALESMAN, "The snacks are your department. But the football? That's on me.");
            dialog_text(CHARACTERS_FATMAN, "I can work with that division of labor.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "You and your buddy could toss it around. Nothing intense, just casual backyard throws.");
            dialog_text(CHARACTERS_FATMAN, "Doug and I tried to play catch once. With a sandwich. Long story. Didn't end well for the sandwich.");
            dialog_text(CHARACTERS_SALESMAN, "A football is way more aerodynamic than a sandwich.");
            dialog_text(CHARACTERS_FATMAN, "That depends entirely on the sandwich.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Come on, big guy. Let me show you this is worth it.");
    dialog_text(CHARACTERS_FATMAN, "Alright, alright. You've piqued my interest. And that's hard to do when there's food waiting.");

    encounter_minigame(&rhythm_game);

    dialog_text(CHARACTERS_FATMAN, "You know what, fine. I'll take the football. It'll look good next to the TV. Maybe I'll throw it at Doug when he eats the last slice.");
    dialog_text(CHARACTERS_SALESMAN, "That's the competitive spirit!");
    dialog_text(CHARACTERS_FATMAN, "Nobody takes my last slice, man. Nobody.");
    dialog_text(CHARACTERS_SALESMAN, "Enjoy the football, and good luck to Doug.");

    encounter_end();
}


// ============================================================================
// SALESMAN GAME - DIALOG ENCOUNTERS
// 16 unique encounters: 4 characters x 4 items
// ============================================================================

// ----------------------------------------------------------------------------
// OLD LADY ENCOUNTERS
// ----------------------------------------------------------------------------

void encounter_oldlady_ak47(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon, ma'am! Beautiful day, isn't it?");
    dialog_text(CHARACTERS_OLDLADY, "Oh, why yes it is, dear. Can I help you with something?");
    dialog_text(CHARACTERS_SALESMAN, "Actually, I'm here to help YOU. Do you ever worry about home security?");
    dialog_text(CHARACTERS_OLDLADY, "Well, there was that raccoon that got into my bird feeder last Tuesday...");
    dialog_text(CHARACTERS_SALESMAN, "A raccoon! That's practically an invasion. What if I told you I had the ULTIMATE home defense solution?");
    dialog_text(CHARACTERS_OLDLADY, "Oh my, is it one of those motion-sensor lights? My friend Doris got one of those.");
    dialog_text(CHARACTERS_SALESMAN, "Even better.");
    dialog_text(CHARACTERS_OLDLADY, "A deadbolt?");
    dialog_text(CHARACTERS_SALESMAN, "Ma'am, I present to you... the AK-47. Seventy-five rounds per minute of pure peace of mind.");
    dialog_text(CHARACTERS_OLDLADY, "Oh heavens! That's a... that's a machine gun!");
    dialog_text(CHARACTERS_SALESMAN, "Assault rifle, technically. Very reliable. You could leave it in your garden for a year and it would still fire.");

    switch (dialog_selection("Choose your pitch angle", dialog_options("Home defense", "Gardening tool", "Raccoon deterrent", "Collector's item"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Think about it. You hear a noise at 2 AM. Instead of calling the police and waiting twenty minutes, you've got instant protection.");
            dialog_text(CHARACTERS_OLDLADY, "Well, I DO have trouble sleeping anyway...");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Plus, the bayonet attachment is great for aerating soil. Two birds, one stone.");
            dialog_text(CHARACTERS_OLDLADY, "I HAVE been meaning to turn over my vegetable patch...");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "One burst in the air and I guarantee that raccoon relocates to the next county.");
            dialog_text(CHARACTERS_OLDLADY, "He DID eat all my tomatoes last summer...");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "These are becoming very rare, ma'am. Your grandchildren will fight over who inherits it. Great conversation piece for the mantle.");
            dialog_text(CHARACTERS_OLDLADY, "It WOULD look nice next to Harold's bowling trophies...");
        } break;
    }

    dialog_text(CHARACTERS_OLDLADY, "I don't know, dear. It seems a bit much. My late husband Harold was in the army, you know.");
    dialog_text(CHARACTERS_SALESMAN, "Then consider it a tribute to Harold's service! He'd want you protected.");
    dialog_text(CHARACTERS_OLDLADY, "He always did say I needed to toughen up. Alright, let me see what this thing can do.");
    encounter_minigame(&smile_game);
    dialog_text(CHARACTERS_OLDLADY, "Oh my! That was quite something! My hip is vibrating!");
    dialog_text(CHARACTERS_SALESMAN, "So, what do you think?");
    dialog_text(CHARACTERS_OLDLADY, "I'll take two. One for me and one for Doris. That woman needs more than a motion-sensor light.");
    encounter_end();
}

void encounter_oldlady_cookies(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Hello there! Lovely garden you've got.");
    dialog_text(CHARACTERS_OLDLADY, "Oh, thank you, sweetheart! I've been working on the petunias all morning. Are you from the neighborhood?");
    dialog_text(CHARACTERS_SALESMAN, "Just passing through, ma'am. I've got something here that I think you'll appreciate. Girl Scout Cookies!");
    dialog_text(CHARACTERS_OLDLADY, "Girl Scout Cookies? Oh, I used to BE a Girl Scout, back when Eisenhower was president.");
    dialog_text(CHARACTERS_SALESMAN, "No kidding! Then you know these are the real deal. Thin Mints, Samoas, Tagalongs... the whole lineup.");
    dialog_text(CHARACTERS_OLDLADY, "Now hold on. Where's the little girl? Aren't these usually sold by the scouts themselves?");

    switch (dialog_selection("Explain the situation", dialog_options("I'm filling in for my daughter", "The scouts outsourced", "She's in the car", "Dodge the question"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "My daughter's home sick, and she was SO close to earning her cookie badge. I couldn't let her down.");
            dialog_text(CHARACTERS_OLDLADY, "Oh, what a devoted father! That's so sweet.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Budget cuts, ma'am. The scouts have contracted independent distributors. I'm employee of the month, actually.");
            dialog_text(CHARACTERS_OLDLADY, "The world sure has changed since my day.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "She's waiting in the van. She's a little shy.");
            dialog_text(CHARACTERS_OLDLADY, "Oh, the poor thing. Well, I remember how nerve-wracking it was going door to door.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "The important thing, ma'am, is the cookies. Just look at this box. Tell me that doesn't take you back.");
            dialog_text(CHARACTERS_OLDLADY, "Well, I suppose the cookies are what matters...");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "These Thin Mints are straight out of the freezer. That's where the magic happens.");
    dialog_text(CHARACTERS_OLDLADY, "Frozen Thin Mints! My Harold used to hide those in the garage freezer so I wouldn't eat them all.");
    dialog_text(CHARACTERS_SALESMAN, "Smart man. These things are dangerously good. Want to do a taste test?");
    dialog_text(CHARACTERS_OLDLADY, "I really shouldn't. My doctor says I need to watch my sugar.");
    dialog_text(CHARACTERS_SALESMAN, "Ma'am, with all due respect, your doctor isn't here. And life's too short for bad cookies.");
    dialog_text(CHARACTERS_OLDLADY, "You make a compelling point, young man.");
    encounter_minigame(&smile_game);
    dialog_text(CHARACTERS_OLDLADY, "Those are just as good as I remember. Maybe better.");
    dialog_text(CHARACTERS_SALESMAN, "Can I put you down for a few boxes?");
    dialog_text(CHARACTERS_OLDLADY, "Give me eight boxes. And if you see my doctor, you were never here.");
    encounter_end();
}

void encounter_oldlady_steak(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Afternoon, ma'am! Something smells wonderful in there. Are you cooking?");
    dialog_text(CHARACTERS_OLDLADY, "Just warming up some soup, dear. It's about all I make for myself these days.");
    dialog_text(CHARACTERS_SALESMAN, "Soup? Ma'am, I hate to say it, but you deserve better. What if I told you I have prime USDA-grade ribeye steaks?");
    dialog_text(CHARACTERS_OLDLADY, "Steaks? Oh, I haven't had a good steak since Harold was alive. He used to grill every Sunday.");
    dialog_text(CHARACTERS_SALESMAN, "Every Sunday? The man had taste. Look at this marbling. You can practically hear it sizzling.");
    dialog_text(CHARACTERS_OLDLADY, "That IS beautiful. But I don't really cook for just myself anymore. It feels like too much trouble.");
    dialog_text(CHARACTERS_SALESMAN, "Ma'am, cooking a steak this good isn't trouble. It's therapy.");

    switch (dialog_selection("Appeal to her", dialog_options("Nostalgia angle", "Health benefits", "Invite the neighbors", "Simple recipe"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Close your eyes. Imagine the smell of that grill on a Sunday afternoon. Harold would want you to keep that tradition alive.");
            dialog_text(CHARACTERS_OLDLADY, "Don't you go making me cry on my own porch, young man.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Red meat is full of iron and B12. Your doctor would actually approve of this one.");
            dialog_text(CHARACTERS_OLDLADY, "Really? Well, I have been feeling a bit tired lately...");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "Get a few of these, invite Doris and the girls over. Make it an event!");
            dialog_text(CHARACTERS_OLDLADY, "A dinner party! I haven't hosted one of those in ages!");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "Salt, pepper, butter, four minutes each side. That's it. Even Harold couldn't mess that up.");
            dialog_text(CHARACTERS_OLDLADY, "Ha! He DID burn the burgers more than once, rest his soul.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Tell you what. Let me show you the quality of this cut. One demo and you'll be a believer.");
    dialog_text(CHARACTERS_OLDLADY, "Alright, alright. But only because you remind me of my grandson. He could sell ice to an eskimo too.");
    encounter_minigame(&smile_game);
    dialog_text(CHARACTERS_OLDLADY, "Oh my word. That is the most tender thing I've had in years.");
    dialog_text(CHARACTERS_SALESMAN, "So? Can I interest you in a few?");
    dialog_text(CHARACTERS_OLDLADY, "I'll take a dozen. And give me your card. Doris is going to lose her mind when she tastes these.");
    encounter_end();
}

void encounter_oldlady_console(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! You must be the lady of the house.");
    dialog_text(CHARACTERS_OLDLADY, "I'm the ONLY lady of the house. What can I do for you, young man?");
    dialog_text(CHARACTERS_SALESMAN, "I have something here that's going to change your evenings forever. A brand new game console!");
    dialog_text(CHARACTERS_OLDLADY, "A game console? Like a Nintendo? My grandchildren have one of those.");
    dialog_text(CHARACTERS_SALESMAN, "This makes a Nintendo look like a toaster. We're talking 4K resolution, surround sound, hundreds of games.");
    dialog_text(CHARACTERS_OLDLADY, "Dear, I can barely work the remote control. Last week I accidentally ordered a movie in Spanish and watched the whole thing because I couldn't figure out how to change it.");
    dialog_text(CHARACTERS_SALESMAN, "And did you enjoy it?");
    dialog_text(CHARACTERS_OLDLADY, "Actually, yes. It was quite romantic.");

    switch (dialog_selection("Sell her on it", dialog_options("Grandkids will visit more", "Brain training games", "It plays movies too", "It's very simple"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Imagine this. You get this console, and suddenly your grandkids are BEGGING to come over every weekend.");
            dialog_text(CHARACTERS_OLDLADY, "They do usually only come when I bribe them with cookies...");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "They've got puzzle games, brain teasers, the works. Studies show gaming keeps the mind sharp well into your nineties.");
            dialog_text(CHARACTERS_OLDLADY, "My mind is plenty sharp, thank you. I beat Doris at bridge every single week.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "Forget the games. This thing streams movies, shows, music. And it's all in English. Mostly.");
            dialog_text(CHARACTERS_OLDLADY, "Can I watch my stories on it? I love my stories.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "It's one button to turn on, and then you just wave your hands around. No remote needed.");
            dialog_text(CHARACTERS_OLDLADY, "Wave my hands? Like I'm casting a spell?");
            dialog_text(CHARACTERS_SALESMAN, "Exactly like that, yes.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Just give it a try. Five minutes, that's all I'm asking.");
    dialog_text(CHARACTERS_OLDLADY, "I suppose five minutes can't hurt. But if it explodes, you're paying for my curtains.");
    encounter_minigame(&smile_game);
    dialog_text(CHARACTERS_OLDLADY, "I just... I got the highest score? Is that good?");
    dialog_text(CHARACTERS_SALESMAN, "That's INCREDIBLE for a first try!");
    dialog_text(CHARACTERS_OLDLADY, "Move over, grandchildren. Grandma's got a new hobby. I'll take it.");
    encounter_end();
}

// ----------------------------------------------------------------------------
// NERD ENCOUNTERS
// ----------------------------------------------------------------------------

void encounter_nerd_ak47(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Hey there! Got a minute?");
    dialog_text(CHARACTERS_NERD, "That depends. Are you selling religion, solar panels, or a streaming service? Because I'm not interested in any of those.");
    dialog_text(CHARACTERS_SALESMAN, "None of the above. I'm selling something way more interesting. How do you feel about firearms?");
    dialog_text(CHARACTERS_NERD, "Firearms? Like... real ones? I have a level 47 marksman in Fallout, if that counts.");
    dialog_text(CHARACTERS_SALESMAN, "It absolutely does not. This is an AK-47. The real thing. Mikhail Kalashnikov's masterpiece.");
    dialog_text(CHARACTERS_NERD, "Oh, I know what an AK-47 is. Iconic Soviet design, 1947. Gas-operated, rotating bolt, 7.62 by 39 millimeter cartridge. I've read the Wikipedia article like four times.");
    dialog_text(CHARACTERS_SALESMAN, "Okay, so you know your stuff. But have you ever held one?");
    dialog_text(CHARACTERS_NERD, "I've held a replica at Comic-Con. Does that count?");
    dialog_text(CHARACTERS_SALESMAN, "Again, absolutely not.");

    switch (dialog_selection("Pick your angle", dialog_options("Zombie apocalypse prep", "It's like a real-life loot drop", "Historical significance", "Home defense stats"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Let me ask you something. When the zombie apocalypse hits, and we both know it's a matter of WHEN, what's your plan?");
            dialog_text(CHARACTERS_NERD, "I have a detailed spreadsheet, actually. But the weapons column is mostly baseball bats and kitchen knives.");
            dialog_text(CHARACTERS_SALESMAN, "That's tragic. This fills that gap beautifully.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Think of this as a legendary loot drop. Right here on your doorstep. What are the odds?");
            dialog_text(CHARACTERS_NERD, "Statistically pretty low, I'll admit. The RNG gods are smiling on me today.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "This design hasn't changed in almost eighty years because it didn't need to. It's the most produced firearm in history. You'd own a piece of engineering history.");
            dialog_text(CHARACTERS_NERD, "That IS a compelling provenance. Like owning a first-edition print of a classic.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "Ninety-nine percent reliability rate. Exposed to mud, sand, water, doesn't matter. It fires every single time.");
            dialog_text(CHARACTERS_NERD, "Ninety-nine percent uptime? That's better than most cloud servers.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Here. Just feel the weight of it. No commitment.");
    dialog_text(CHARACTERS_NERD, "Oh man. Okay. This is... this is significantly heavier than the Comic-Con one.");
    dialog_text(CHARACTERS_SALESMAN, "That's the weight of quality. Want to really see what it can do?");
    dialog_text(CHARACTERS_NERD, "I mean, I probably shouldn't... but also I definitely need to for research purposes.");
    encounter_minigame(&memory_game);
    dialog_text(CHARACTERS_NERD, "Oh my GOD. My hands are shaking. That was the most visceral experience of my entire life, and I include the time I met Gabe Newell.");
    dialog_text(CHARACTERS_SALESMAN, "So, can I interest you?");
    dialog_text(CHARACTERS_NERD, "Shut up and take my money. This is going right above my display shelf next to my Master Sword replica.");
    encounter_end();
}

void encounter_nerd_cookies(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Hi! Couldn't help but notice the Millennium Falcon doormat. Nice touch.");
    dialog_text(CHARACTERS_NERD, "Thanks, it was a limited run from ThinkGeek before they shut down. What do you want?");
    dialog_text(CHARACTERS_SALESMAN, "I come bearing gifts. Well, not gifts. Merchandise. Girl Scout Cookies!");
    dialog_text(CHARACTERS_NERD, "Girl Scout Cookies? During a non-cookie season? That's suspicious.");
    dialog_text(CHARACTERS_SALESMAN, "I prefer to think of it as exclusive early access.");
    dialog_text(CHARACTERS_NERD, "Early access. So they're in beta?");
    dialog_text(CHARACTERS_SALESMAN, "Sure. Yeah. The cookies are in beta. And I need taste testers. Interested?");

    switch (dialog_selection("Pitch the cookies", dialog_options("Limited edition flavors", "Perfect gaming fuel", "Support a good cause", "Ingredient stats"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "These are limited edition. Once they're gone, they're gone. Like a rare drop.");
            dialog_text(CHARACTERS_NERD, "FOMO is a well-documented psychological manipulation tactic and I am fully susceptible to it. Go on.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Think about it. Late-night raid, you're three hours deep, your blood sugar is crashing. You reach over, grab a Thin Mint. Problem solved.");
            dialog_text(CHARACTERS_NERD, "I usually just drink energy drinks, but my cardiologist has expressed concerns.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "These fund programs that teach young girls about entrepreneurship and leadership. Basically building the next generation of tech founders.");
            dialog_text(CHARACTERS_NERD, "So I'd be a venture capitalist in the cookie space? I like the framing.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "Look at the macros on these bad boys. High caloric density, long shelf life, excellent sugar-to-fat ratio for sustained energy.");
            dialog_text(CHARACTERS_NERD, "You're speaking my language. I optimize everything else in my life, why not snacks?");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Come on. One taste. What's the worst that could happen?");
    dialog_text(CHARACTERS_NERD, "Historically, every time someone says that to me, something catches fire. But fine.");
    encounter_minigame(&memory_game);
    dialog_text(CHARACTERS_NERD, "Okay. Those Samoas are... those are S-tier. Easily S-tier. Maybe even S-plus.");
    dialog_text(CHARACTERS_SALESMAN, "How many boxes can I put you down for?");
    dialog_text(CHARACTERS_NERD, "Give me ten. And I'm going to need a spreadsheet to track my consumption rate so I can reorder before I run out.");
    dialog_text(CHARACTERS_SALESMAN, "You do that, buddy.");
    encounter_end();
}

void encounter_nerd_steak(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Hey! Is that a drone on your roof?");
    dialog_text(CHARACTERS_NERD, "Weather station, actually. I track local atmospheric data and upload it to a community database. What do you need?");
    dialog_text(CHARACTERS_SALESMAN, "I need you to look at this steak and tell me it isn't the most beautiful thing you've ever seen.");
    dialog_text(CHARACTERS_NERD, "A steak? You're selling... steaks? Door to door?");
    dialog_text(CHARACTERS_SALESMAN, "Premium, USDA Prime grade ribeye. Direct from the ranch to your door. No middleman.");
    dialog_text(CHARACTERS_NERD, "Peer-to-peer steak distribution. That's either genius or deeply concerning.");
    dialog_text(CHARACTERS_SALESMAN, "It's genius. Look at that marbling. That's not just fat. That's flavor architecture.");

    switch (dialog_selection("Win him over", dialog_options("Maillard reaction science", "Protein for brain power", "It's organic and ethical", "Cooking is like chemistry"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "You know about the Maillard reaction, right? When you sear this at exactly 375 degrees, the amino acids and sugars undergo a non-enzymatic browning reaction that creates over 600 distinct flavor compounds.");
            dialog_text(CHARACTERS_NERD, "I... actually didn't know the exact number. Six hundred? That's a significant flavor vector.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "You run that brain hard, right? Steak has creatine, B12, zinc, iron. It's basically a CPU upgrade for your body.");
            dialog_text(CHARACTERS_NERD, "I have been looking into nootropics. A dietary approach could be more sustainable.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "These are grass-fed, pasture-raised, ethically sourced. If that cow had a LinkedIn, it would have had an impressive resume.");
            dialog_text(CHARACTERS_NERD, "I appreciate the ethical supply chain transparency. That matters to me.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "Cooking a steak is basically applied chemistry. Temperature control, timing, chemical reactions. It's the most delicious experiment you'll ever run.");
            dialog_text(CHARACTERS_NERD, "Can I use my sous vide machine? I bought it two years ago and I've only used it to heat up ramen.");
        } break;
    }

    dialog_text(CHARACTERS_NERD, "I mostly eat instant noodles and microwaveable burritos, if I'm being honest.");
    dialog_text(CHARACTERS_SALESMAN, "And that ends today. Let me show you what real food tastes like.");
    dialog_text(CHARACTERS_NERD, "Okay but if I don't like it, I'm leaving you a one-star review on every platform I can find.");
    encounter_minigame(&memory_game);
    dialog_text(CHARACTERS_NERD, "I'm having a culinary awakening right now. Everything I thought I knew about food was wrong.");
    dialog_text(CHARACTERS_SALESMAN, "Welcome to the world of real cooking, my friend.");
    dialog_text(CHARACTERS_NERD, "I'll take four. And do you have a recommended internal temperature chart? I want to cook these with PRECISION.");
    encounter_end();
}

void encounter_nerd_console(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Hey there! Got something I think you might--");
    dialog_text(CHARACTERS_NERD, "Is that the XStation Pro? Are you SERIOUS right now?");
    dialog_text(CHARACTERS_SALESMAN, "I haven't even opened my mouth yet.");
    dialog_text(CHARACTERS_NERD, "You don't need to. I can see the box. That's the matte black limited edition. Those have been sold out for MONTHS.");
    dialog_text(CHARACTERS_SALESMAN, "So you're familiar with the product.");
    dialog_text(CHARACTERS_NERD, "Familiar? I've watched every teardown video. I know the specs better than the engineers who built it. Custom 8-core processor, 16 teraflops, ray tracing, SSD with 9.8 gigabytes per second read speed. Where did you even GET one?");
    dialog_text(CHARACTERS_SALESMAN, "I have my sources. The question is, do you want it?");

    switch (dialog_selection("Seal the deal", dialog_options("Below retail price", "Exclusive launch title included", "VR compatible", "Last one in stock"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "I can do this below retail. Consider it a fellow-enthusiast discount.");
            dialog_text(CHARACTERS_NERD, "Below retail for a sold-out console? What's the catch? Is it stolen? I won't judge, I just need to know.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "It comes bundled with an exclusive launch title that hasn't even hit digital stores yet.");
            dialog_text(CHARACTERS_NERD, "An EXCLUSIVE? My Twitch followers are going to lose their collective minds.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "This model is VR-ready right out of the box. No adapters, no dongles, just plug and play.");
            dialog_text(CHARACTERS_NERD, "Dongle-free? In THIS economy? That's practically unheard of.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "This is the last one I have. After this, you're back to refreshing store pages and crying into your keyboard.");
            dialog_text(CHARACTERS_NERD, "Don't remind me. I've lost six online drops. SIX. My F5 key is worn smooth.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Let me boot it up. You can see for yourself.");
    dialog_text(CHARACTERS_NERD, "This is the best thing that has happened to me since I found that first-edition Charizard at a garage sale. I need a moment.");
    dialog_text(CHARACTERS_SALESMAN, "Take your time, buddy.");
    dialog_text(CHARACTERS_NERD, "I'm fine. I'm fine. Let's do this.");
    encounter_minigame(&memory_game);
    dialog_text(CHARACTERS_NERD, "The frame rate. The FRAME RATE. It's like butter. It's like warm, beautiful, 120-frames-per-second butter.");
    dialog_text(CHARACTERS_SALESMAN, "So I'm going to take that as a yes?");
    dialog_text(CHARACTERS_NERD, "I would sell a kidney for this. Fortunately I have money. I'll take it. Can you also leave before I start crying? I'd like to do that in private.");
    encounter_end();
}

// ----------------------------------------------------------------------------
// SHOTGUNNER ENCOUNTERS
// ----------------------------------------------------------------------------

void encounter_shotgunner_ak47(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Howdy! Nice property you've got here.");
    dialog_text(CHARACTERS_SHOTGUNNER, "That's far enough, stranger. You've got about ten seconds to tell me why you're on my land.");
    dialog_text(CHARACTERS_SALESMAN, "Easy there, friend. I'm not looking for trouble. I'm looking to make a sale.");
    dialog_text(CHARACTERS_SHOTGUNNER, "I don't need no magazines, no vacuum cleaners, and I already found Jesus. He was behind the couch the whole time.");
    dialog_text(CHARACTERS_SALESMAN, "How about an AK-47?");
    dialog_text(CHARACTERS_SHOTGUNNER, "... I'm listening.");
    dialog_text(CHARACTERS_SALESMAN, "Thought that might get your attention. Genuine AK-47. Factory fresh.");
    dialog_text(CHARACTERS_SHOTGUNNER, "I know what an AK is, boy. Got a gun safe with thirty-seven firearms in it. Why would I need a Russian one?");

    switch (dialog_selection("Convince him", dialog_options("Compliment his collection", "Reliability argument", "It's a patriotic duty", "Trade-up offer"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Thirty-seven? That's impressive. But a collection that fine? It NEEDS an AK. It's like a museum without a Picasso.");
            dialog_text(CHARACTERS_SHOTGUNNER, "Don't compare my guns to some fancy art. But... I see your point. There IS a gap on the wall.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Look, your shotgun is great for close range. But what about 300 yards out? This fills that gap in your defense perimeter.");
            dialog_text(CHARACTERS_SHOTGUNNER, "Hmm. I HAVE been worried about the treeline to the north. Can't cover it with buckshot.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "Know your enemy, right? Understanding Soviet engineering makes you a better American.");
            dialog_text(CHARACTERS_SHOTGUNNER, "That... that's the smartest thing a salesman has ever said to me. And I hate that you're right.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "I could take one of your current pieces in trade and give you a discount. Everybody wins.");
            dialog_text(CHARACTERS_SHOTGUNNER, "Trade in one of my babies? You've got some nerve. But... which one would you take?");
        } break;
    }

    dialog_text(CHARACTERS_SHOTGUNNER, "Alright, salesman. You talked your way this far. But I ain't buying nothing until I see it shoot.");
    dialog_text(CHARACTERS_SALESMAN, "Wouldn't have it any other way.");
    dialog_text(CHARACTERS_SHOTGUNNER, "See that old washing machine out by the fence? If it can hit that, we'll talk.");
    encounter_minigame(&shotgun_game);
    dialog_text(CHARACTERS_SHOTGUNNER, "Well I'll be damned. Blew that Maytag clean in half.");
    dialog_text(CHARACTERS_SALESMAN, "So? We got a deal?");
    dialog_text(CHARACTERS_SHOTGUNNER, "I respect a good firearm, regardless of country of origin. You got yourself a deal, boy. But if you tell anyone I bought a Russian gun, I'll use it on you.");
    dialog_text(CHARACTERS_SALESMAN, "Wouldn't dream of it, sir.");
    encounter_end();
}

void encounter_shotgunner_cookies(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Hello there! I--");
    dialog_text(CHARACTERS_SHOTGUNNER, "Stop right there. You see that sign? NO SOLICITORS. You blind or just stupid?");
    dialog_text(CHARACTERS_SALESMAN, "I saw it. I just figured you'd make an exception for Girl Scout Cookies.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Girl Scout... hold on. You don't look like no Girl Scout.");
    dialog_text(CHARACTERS_SALESMAN, "I didn't make the uniform cutoff. But the cookies are the real deal, I promise.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Hmph. My ex-wife used to buy those. Only good decision she ever made.");

    switch (dialog_selection("Approach carefully", dialog_options("Bond over the cookies", "Man's man approach", "Limited supply urgency", "Challenge his toughness"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "See? Even in the darkest times, cookies bring light. These could be a fresh start.");
            dialog_text(CHARACTERS_SHOTGUNNER, "Don't get philosophical on my porch. But... she always got the Tagalongs. Those were decent.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "These aren't sissy snacks, friend. They're compact, high-calorie, long shelf life. Basically field rations.");
            dialog_text(CHARACTERS_SHOTGUNNER, "Field rations? Now you're speaking my language.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "I've only got a few boxes left. Once they're gone, you'll be cookie-less until next season. That's months away.");
            dialog_text(CHARACTERS_SHOTGUNNER, "I don't like being told what I can't have.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "Most guys are too proud to admit they like cookies. Takes a real man to own it.");
            dialog_text(CHARACTERS_SHOTGUNNER, "You trying to psychologize me? On my OWN porch?");
            dialog_text(CHARACTERS_SALESMAN, "Is it working?");
            dialog_text(CHARACTERS_SHOTGUNNER, "... Maybe.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Just try one. If you don't like it, I'll leave and never come back. Scout's honor.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Fine. But I'm holding my shotgun the entire time.");
    dialog_text(CHARACTERS_SALESMAN, "Totally reasonable.");
    encounter_minigame(&shotgun_game);
    dialog_text(CHARACTERS_SHOTGUNNER, "...");
    dialog_text(CHARACTERS_SALESMAN, "Well?");
    dialog_text(CHARACTERS_SHOTGUNNER, "These Thin Mints are... these are really good. Don't look at me like that.");
    dialog_text(CHARACTERS_SALESMAN, "I'm not looking at you like anything.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Give me five boxes. And if you tell ANYONE about this, there will be consequences.");
    encounter_end();
}

void encounter_shotgunner_steak(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Afternoon! I noticed the smoker in your backyard. You a grilling man?");
    dialog_text(CHARACTERS_SHOTGUNNER, "That ain't a smoker, that's a COMPETITION smoker. Custom-built. Won three county fairs with that thing. What do you want?");
    dialog_text(CHARACTERS_SALESMAN, "Well then today is your lucky day, sir. Because I've got premium ribeye steaks that would be HONORED to meet that smoker.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Premium? I buy my beef from Dale's ranch down the road. Known that man thirty years. Why would I buy steaks from some stranger?");
    dialog_text(CHARACTERS_SALESMAN, "Because Dale's steaks are good. These are GREAT. Look at this marbling. Go ahead. I'll wait.");
    dialog_text(CHARACTERS_SHOTGUNNER, "... That is some serious marbling. Where'd you source these?");

    switch (dialog_selection("Win his trust", dialog_options("Small ranch, free range", "Challenge his grilling", "Bulk deal for the smoker", "Competition quality"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Small family ranch up north. Grass-fed, pasture-raised, no hormones. These cows lived better than most people.");
            dialog_text(CHARACTERS_SHOTGUNNER, "That's how it should be. I respect a man who raises cattle right.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "I bet that smoker could do things with this steak that would make a grown man weep. But maybe that's too much to ask...");
            dialog_text(CHARACTERS_SHOTGUNNER, "Too much? Boy, there ain't a cut of meat on this earth my smoker can't handle. Give me that.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "I'll cut you a bulk deal. Twelve steaks for the price of ten. Stock up that freezer for the next cookout.");
            dialog_text(CHARACTERS_SHOTGUNNER, "A bulk deal? Now you're talking sense.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "You win county fairs with Dale's beef? Imagine what you'd do with THIS. You could go STATE level.");
            dialog_text(CHARACTERS_SHOTGUNNER, "State level... Hank Morrison won state last year and he won't shut up about it. I need to dethrone that man.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Let me show you what this cut can do. You've got the skills, just let the steak speak for itself.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Alright, hotshot. But I'm cooking it MY way. None of that medium-rare nonsense.");
    dialog_text(CHARACTERS_SALESMAN, "Your house, your rules.");
    encounter_minigame(&shotgun_game);
    dialog_text(CHARACTERS_SHOTGUNNER, "Alright. I'm man enough to admit it. That's a damn good steak.");
    dialog_text(CHARACTERS_SALESMAN, "Better than Dale's?");
    dialog_text(CHARACTERS_SHOTGUNNER, "Don't push your luck. But yeah. Better than Dale's. Give me twenty. And don't tell Dale.");
    dialog_text(CHARACTERS_SALESMAN, "My lips are sealed, sir.");
    encounter_end();
}

void encounter_shotgunner_console(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "How's it going? I've got something here that--");
    dialog_text(CHARACTERS_SHOTGUNNER, "If that's one of them robot vacuums, my dog shot the last one.");
    dialog_text(CHARACTERS_SALESMAN, "Your DOG shot a-- never mind. No, this is a game console.");
    dialog_text(CHARACTERS_SHOTGUNNER, "A game console? Do I look like a twelve-year-old to you?");
    dialog_text(CHARACTERS_SALESMAN, "You look like a man who enjoys competition. And this has the best hunting and shooting simulators on the market.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Hunting simulators? Why would I simulate hunting when I can just... go hunting?");
    dialog_text(CHARACTERS_SALESMAN, "Because it's February, deer season's over, and I can see from here that your trigger finger is getting restless.");

    switch (dialog_selection("Reel him in", dialog_options("Hunting game demo", "It has fishing games too", "Online multiplayer competition", "It's also a DVD player"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "They've got elk, moose, bear, even African big game. Stuff you can't hunt legally in this state.");
            dialog_text(CHARACTERS_SHOTGUNNER, "African big game? From my living room? That's... not the worst idea.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "And the fishing simulators? Bass, trout, deep sea marlin. No license required.");
            dialog_text(CHARACTERS_SHOTGUNNER, "No license? So the government can't track my catches? I'm intrigued.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "You can play online against other people. Real competition. Leaderboards. Prove you're the best shot in the country without leaving your house.");
            dialog_text(CHARACTERS_SHOTGUNNER, "Nationwide leaderboards? Hank Morrison would eat his heart out.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "Plus, it plays DVDs, Blu-rays, streams movies. All your action movies in one place.");
            dialog_text(CHARACTERS_SHOTGUNNER, "I do watch a lot of Chuck Norris. My DVD player's been broken since the dog incident.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Just give it a shot. Pun intended.");
    dialog_text(CHARACTERS_SHOTGUNNER, "That pun was terrible. But fine. Set it up. If it's stupid, you're taking it and leaving.");
    encounter_minigame(&shotgun_game);
    dialog_text(CHARACTERS_SHOTGUNNER, "I just bagged a twelve-point virtual buck and I'm feeling things I wasn't prepared to feel.");
    dialog_text(CHARACTERS_SALESMAN, "That's the magic of modern gaming, friend.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Alright, you win. But this stays between us. My buddies at the range can never know about this.");
    dialog_text(CHARACTERS_SALESMAN, "I don't even know your name, sir.");
    dialog_text(CHARACTERS_SHOTGUNNER, "Good. Keep it that way. I'll take it.");
    encounter_end();
}

// ----------------------------------------------------------------------------
// FAT MAN ENCOUNTERS
// ----------------------------------------------------------------------------

void encounter_fatman_ak47(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Hey there! Beautiful day to--");
    dialog_text(CHARACTERS_FATMAN, "Hold on, hold on... let me catch my breath. I just walked from the couch to the door.");
    dialog_text(CHARACTERS_SALESMAN, "Take your time.");
    dialog_text(CHARACTERS_FATMAN, "Okay. What do you want? And make it quick, my show comes on in ten minutes.");
    dialog_text(CHARACTERS_SALESMAN, "I'll be brief. How would you like to own an AK-47?");
    dialog_text(CHARACTERS_FATMAN, "An AK-47? Like, a gun? What would I do with a gun?");
    dialog_text(CHARACTERS_SALESMAN, "Protect yourself! Defend your home! Look cool!");
    dialog_text(CHARACTERS_FATMAN, "The only thing I need to defend is the last slice of pizza from my roommate. And I do that just fine with a fork.");

    switch (dialog_selection("Convince him", dialog_options("Home delivery protection", "It's lighter than you think", "Impress the neighbors", "It's an investment"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "What about delivery drivers? They bring you food, but what if one day they try to TAKE your food?");
            dialog_text(CHARACTERS_FATMAN, "That's... I never thought about that. That's terrifying.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "This only weighs about eight pounds. Less than a large meat lover's pizza.");
            dialog_text(CHARACTERS_FATMAN, "I carry those with one hand, so that tracks. Not bad.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "Picture it. You, on your porch, AK in hand. Nobody's ever going to mess with you or your property again.");
            dialog_text(CHARACTERS_FATMAN, "The HOA has been on my case about the lawn. This could shift that dynamic.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "These appreciate in value. It's like a savings account that can also defend your snack stash.");
            dialog_text(CHARACTERS_FATMAN, "A financial instrument AND a snack defender? That's dual-purpose. I like efficiency.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Why don't you just try holding it? See how it feels?");
    dialog_text(CHARACTERS_FATMAN, "I mean, I'm already standing up. Might as well make the trip worth it.");
    dialog_text(CHARACTERS_SALESMAN, "That's the spirit.");
    encounter_minigame(&rhythm_game);
    dialog_text(CHARACTERS_FATMAN, "Whoa! The recoil gave my arms more exercise than they've had in months!");
    dialog_text(CHARACTERS_SALESMAN, "See? It's practically a workout machine.");
    dialog_text(CHARACTERS_FATMAN, "A workout machine that shoots? Now THAT I can get behind. I'll take it. Can you carry it inside for me though? My arms are tired now.");
    encounter_end();
}

void encounter_fatman_cookies(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Good afternoon! I've got something special today.");
    dialog_text(CHARACTERS_FATMAN, "If it's edible, you have my full attention. If it's not, I'm going back to my burrito.");
    dialog_text(CHARACTERS_SALESMAN, "It is VERY edible. Girl Scout Cookies! Thin Mints, Samoas, Tagalongs, the whole spread.");
    dialog_text(CHARACTERS_FATMAN, "Girl Scout Cookies? Oh man. Oh MAN. I haven't had those since... okay, since last week. My neighbor's kid sells them. But I already ate everything she had.");
    dialog_text(CHARACTERS_SALESMAN, "Well, the universe has provided. I've got full stock right here.");
    dialog_text(CHARACTERS_FATMAN, "Full stock? As in, how many boxes are we talking?");
    dialog_text(CHARACTERS_SALESMAN, "How many do you want?");
    dialog_text(CHARACTERS_FATMAN, "That's a dangerous question to ask me.");

    switch (dialog_selection("Close the deal", dialog_options("Bulk discount", "New flavors available", "Suggest a sampler", "All-you-can-eat pitch"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "I'll do a bulk rate. Buy ten boxes, I'll throw in two free. Can't beat that math.");
            dialog_text(CHARACTERS_FATMAN, "Free cookies? That's the most beautiful phrase in the English language.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "I've also got a new flavor. Caramel Chocolate Chip. Limited run. Most people haven't even heard of it.");
            dialog_text(CHARACTERS_FATMAN, "A new flavor I haven't tried? This is like Christmas morning.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "How about a sampler pack? One of everything. Cover all your bases.");
            dialog_text(CHARACTERS_FATMAN, "A sampler is a good START. Then I'll know which ones to order by the case.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "Look, I'll level with you. I've got a van full of cookies and nowhere to store them. Help me out and I'll practically give them away.");
            dialog_text(CHARACTERS_FATMAN, "You had me at 'van full of cookies.' Say no more.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Let's crack open a box and make sure they meet your standards.");
    dialog_text(CHARACTERS_FATMAN, "My standards are 'is it food' and 'is it nearby,' so the bar is pretty achievable. But yes, let's taste test.");
    encounter_minigame(&rhythm_game);
    dialog_text(CHARACTERS_FATMAN, "These are perfect. The texture, the sweetness, the way they just DISAPPEAR. I ate six during that conversation.");
    dialog_text(CHARACTERS_SALESMAN, "I know. I watched. It was impressive.");
    dialog_text(CHARACTERS_FATMAN, "Give me everything you've got. ALL of it. I'll figure out storage later.");
    encounter_end();
}

void encounter_fatman_steak(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Afternoon! Is that a deep fryer I see through your window?");
    dialog_text(CHARACTERS_FATMAN, "That's one of three. What's it to you?");
    dialog_text(CHARACTERS_SALESMAN, "A man of culinary ambition! I've got something right up your alley. Premium ribeye steaks.");
    dialog_text(CHARACTERS_FATMAN, "Steaks. Now you're talking. Come closer. Let me see them.");
    dialog_text(CHARACTERS_SALESMAN, "Look at this. Two inches thick. Perfect marbling. You could cut this with a stern look.");
    dialog_text(CHARACTERS_FATMAN, "Oh, that's gorgeous. That right there is a work of art. I want to frame it. And then eat the frame.");

    switch (dialog_selection("Seal the deal", dialog_options("Pairs great with sides", "Suggest deep-frying it", "Premium butter-baste method", "Go for quantity"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "Imagine this with a loaded baked potato, garlic bread, maybe some mac and cheese on the side...");
            dialog_text(CHARACTERS_FATMAN, "Stop. You're making me emotional. Keep going.");
            dialog_text(CHARACTERS_SALESMAN, "... topped with sour cream, chives, and extra cheese.");
            dialog_text(CHARACTERS_FATMAN, "I think I'm in love.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "Now, hear me out... have you ever deep-fried a steak?");
            dialog_text(CHARACTERS_FATMAN, "Deep-fried a steak? No. But I feel like I was born to do exactly that.");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "You butter-baste this in a cast iron skillet with garlic and rosemary. The fat bastes the steak. It's like giving it a spa day before you eat it.");
            dialog_text(CHARACTERS_FATMAN, "A spa day for a steak. That is the most beautiful sentence I've ever heard.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "I can do a case of twenty-four. That's a steak for every day of the week, three times over, with some left for midnight snacks.");
            dialog_text(CHARACTERS_FATMAN, "Midnight steak. You just invented my new favorite meal.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "What do you say we fire up one of those fryers and give this a test run?");
    dialog_text(CHARACTERS_FATMAN, "Brother, I thought you'd never ask. Step into my kitchen. Well, don't actually step in, there's not a lot of room. Stand in the doorway.");
    encounter_minigame(&rhythm_game);
    dialog_text(CHARACTERS_FATMAN, "This is the greatest thing I have ever put in my mouth. And I once ate an entire wedding cake by myself.");
    dialog_text(CHARACTERS_SALESMAN, "An entire wedding cake?");
    dialog_text(CHARACTERS_FATMAN, "It wasn't my wedding. Long story. Anyway, give me the full case. Actually, give me two cases. One for eating and one for emotional support.");
    encounter_end();
}

void encounter_fatman_console(void) {
    encounter_begin();
    dialog_text(CHARACTERS_SALESMAN, "Hey! I can hear you've already got a TV going in there. Sounds like a big one.");
    dialog_text(CHARACTERS_FATMAN, "Seventy-five inches. It's the centerpiece of my life. What do you want?");
    dialog_text(CHARACTERS_SALESMAN, "I want to make that TV reach its full potential. I've got a brand new game console here.");
    dialog_text(CHARACTERS_FATMAN, "A game console? I already have one from... I think 2018? It still works. Mostly. The fan sounds like a jet engine and it takes forty minutes to load anything, but it works.");
    dialog_text(CHARACTERS_SALESMAN, "My friend, that is not working. That is suffering. This right here loads games in under two seconds.");
    dialog_text(CHARACTERS_FATMAN, "Two seconds? I currently use loading screens as bathroom breaks. What would I even do with all that free time?");
    dialog_text(CHARACTERS_SALESMAN, "Play more games. Eat more snacks. Live your best life.");

    switch (dialog_selection("Win him over", dialog_options("Couch co-op games", "Food delivery apps built in", "4K on his 75-inch TV", "Voice controlled"))) {
        case 0: {
            dialog_text(CHARACTERS_SALESMAN, "This has the best couch co-op library on the market. You don't even have to get up to have a social life.");
            dialog_text(CHARACTERS_FATMAN, "A social life from my couch? Technology has finally caught up to my lifestyle.");
        } break;
        case 1: {
            dialog_text(CHARACTERS_SALESMAN, "It's got built-in apps for every food delivery service. Order a pizza without pausing your game.");
            dialog_text(CHARACTERS_FATMAN, "Seamless food-to-gaming pipeline? Where has this been all my life?");
        } break;
        case 2: {
            dialog_text(CHARACTERS_SALESMAN, "On your seventy-five-inch screen, this thing will pump out 4K visuals that will make your eyeballs weep with joy.");
            dialog_text(CHARACTERS_FATMAN, "My eyeballs DO deserve joy. They work really hard.");
        } break;
        case 3: {
            dialog_text(CHARACTERS_SALESMAN, "Fully voice-controlled. You don't even need to reach for the remote. Just talk and it obeys.");
            dialog_text(CHARACTERS_FATMAN, "No reaching? You just eliminated the hardest part of my day.");
        } break;
    }

    dialog_text(CHARACTERS_SALESMAN, "Let me hook it up to that beast of a TV and you'll see what I mean.");
    dialog_text(CHARACTERS_FATMAN, "Alright, come in. Watch the pizza boxes by the door. And the ones by the couch. And the hallway.");
    encounter_minigame(&rhythm_game);
    dialog_text(CHARACTERS_FATMAN, "Oh no. Oh NO. Everything I've been playing looks like garbage now. You've ruined my old console for me.");
    dialog_text(CHARACTERS_SALESMAN, "You're welcome.");
    dialog_text(CHARACTERS_FATMAN, "I hate you. But I also need this. Take my money before I change my mind. Actually, I won't change my mind. Take my money at whatever speed is convenient.");
    encounter_end();
}



// ============================================================================
// ENCOUNTER FUNCTION POINTER ARRAY
// ============================================================================

const Encounter_Fn encounters[CHARACTERS_COUNT][ITEM_COUNT] = {
    [CHARACTERS_OLDLADY] = {
        [ITEM_VACUUM] = encounter_oldlady_vacuum,
        [ITEM_BOOKS] = encounter_oldlady_books,
        [ITEM_KNIVES] = encounter_oldlady_knives,
        [ITEM_LOLLIPOP] = encounter_oldlady_lollipop,
        [ITEM_COMPUTER] = encounter_oldlady_computer,
        [ITEM_VASE]              = encounter_oldlady_vase,
        [ITEM_COMIC_BOOK]        = encounter_oldlady_comicbook,
        [ITEM_FOOTBALL]          = encounter_oldlady_football,
        [ITEM_AK47]              = encounter_oldlady_ak47,
        [ITEM_GIRLSCOUT_COOKIES] = encounter_oldlady_cookies,
        [ITEM_STEAK]             = encounter_oldlady_steak,
        [ITEM_GAME_CONSOLE]      = encounter_oldlady_console,
    },
    [CHARACTERS_NERD] = {
        [ITEM_VACUUM] = encounter_nerd_vacuum,
        [ITEM_BOOKS] = encounter_nerd_books,
        [ITEM_KNIVES] = encounter_nerd_knives,
        [ITEM_LOLLIPOP] = encounter_nerd_lollipop,
        [ITEM_COMPUTER] = encounter_nerd_computer,
        [ITEM_VASE]              = encounter_nerd_vase,
        [ITEM_COMIC_BOOK]        = encounter_nerd_comicbook,
        [ITEM_FOOTBALL]          = encounter_nerd_football,
        [ITEM_AK47]              = encounter_nerd_ak47,
        [ITEM_GIRLSCOUT_COOKIES] = encounter_nerd_cookies,
        [ITEM_STEAK]             = encounter_nerd_steak,
        [ITEM_GAME_CONSOLE]      = encounter_nerd_console,
    },
    [CHARACTERS_SHOTGUNNER] = {
        [ITEM_VACUUM] = encounter_shotgunner_vacuum,
        [ITEM_BOOKS] = encounter_shotgunner_books,
        [ITEM_KNIVES] = encounter_shotgunner_knives,
        [ITEM_LOLLIPOP] = encounter_shotgunner_lollipop,
        [ITEM_COMPUTER] = encounter_shotgunner_computer,
        [ITEM_VASE]              = encounter_shotgunner_vase,
        [ITEM_COMIC_BOOK]        = encounter_shotgunner_comicbook,
        [ITEM_FOOTBALL]          = encounter_shotgunner_football,
        [ITEM_AK47]              = encounter_shotgunner_ak47,
        [ITEM_GIRLSCOUT_COOKIES] = encounter_shotgunner_cookies,
        [ITEM_STEAK]             = encounter_shotgunner_steak,
        [ITEM_GAME_CONSOLE]      = encounter_shotgunner_console,
    },
    [CHARACTERS_FATMAN] = {
        [ITEM_VACUUM] = encounter_fatman_vacuum,
        [ITEM_BOOKS] = encounter_fatman_books,
        [ITEM_KNIVES] = encounter_fatman_knives,
        [ITEM_LOLLIPOP] = encounter_fatman_lollipop,
        [ITEM_COMPUTER] = encounter_fatman_computer,
        [ITEM_VASE]              = encounter_fatman_vase,
        [ITEM_COMIC_BOOK]        = encounter_fatman_comicbook,
        [ITEM_FOOTBALL]          = encounter_fatman_football,
        [ITEM_AK47]              = encounter_fatman_ak47,
        [ITEM_GIRLSCOUT_COOKIES] = encounter_fatman_cookies,
        [ITEM_STEAK]             = encounter_fatman_steak,
        [ITEM_GAME_CONSOLE]      = encounter_fatman_console,
    },
};


// const Encounter_Fn encounters[CHARACTERS_COUNT][ITEM_COUNT] = {
//     [CHARACTERS_SALESMAN] = {
//         [ITEM_VACUUM]            = sample_encounter
//         [ITEM_BOOKS]             = sample_encounter
//         [ITEM_KNIVES]            = sample_encounter
//         [ITEM_LOLLIPOP]          = sample_encounter
//         [ITEM_COMPUTER]          = sample_encounter
//         [ITEM_VASE]              = sample_encounter
//         [ITEM_COMIC_BOOK]        = sample_encounter
//         [ITEM_FOOTBALL]          = sample_encounter
//         [ITEM_AK47]              = sample_encounter
//         [ITEM_GIRLSCOUT_COOKIES] = sample_encounter
//         [ITEM_STEAK]             = sample_encounter
//         [ITEM_GAME_CONSOLE]      = sample_encounter
//     },
//     [CHARACTERS_OLDLADY] = {
//         [ITEM_VACUUM]            = sample_encounter
//         [ITEM_BOOKS]             = sample_encounter
//         [ITEM_KNIVES]            = sample_encounter
//         [ITEM_LOLLIPOP]          = sample_encounter
//         [ITEM_COMPUTER]          = sample_encounter
//         [ITEM_VASE]              = sample_encounter
//         [ITEM_COMIC_BOOK]        = sample_encounter
//         [ITEM_FOOTBALL]          = sample_encounter
//         [ITEM_AK47]              = sample_encounter
//         [ITEM_GIRLSCOUT_COOKIES] = sample_encounter
//         [ITEM_STEAK]             = sample_encounter
//         [ITEM_GAME_CONSOLE]      = sample_encounter
//     },
//     [CHARACTERS_NERD] = {
//         [ITEM_VACUUM]            = sample_encounter
//         [ITEM_BOOKS]             = sample_encounter
//         [ITEM_KNIVES]            = sample_encounter
//         [ITEM_LOLLIPOP]          = sample_encounter
//         [ITEM_COMPUTER]          = sample_encounter
//         [ITEM_VASE]              = sample_encounter
//         [ITEM_COMIC_BOOK]        = sample_encounter
//         [ITEM_FOOTBALL]          = sample_encounter
//         [ITEM_AK47]              = sample_encounter
//         [ITEM_GIRLSCOUT_COOKIES] = sample_encounter
//         [ITEM_STEAK]             = sample_encounter
//         [ITEM_GAME_CONSOLE]      = sample_encounter
//     },
//     [CHARACTERS_SHOTGUNNER] = {
//         [ITEM_VACUUM]            = sample_encounter
//         [ITEM_BOOKS]             = sample_encounter
//         [ITEM_KNIVES]            = sample_encounter
//         [ITEM_LOLLIPOP]          = sample_encounter
//         [ITEM_COMPUTER]          = sample_encounter
//         [ITEM_VASE]              = sample_encounter
//         [ITEM_COMIC_BOOK]        = sample_encounter
//         [ITEM_FOOTBALL]          = sample_encounter
//         [ITEM_AK47]              = sample_encounter
//         [ITEM_GIRLSCOUT_COOKIES] = sample_encounter
//         [ITEM_STEAK]             = sample_encounter
//         [ITEM_GAME_CONSOLE]      = sample_encounter
//     },
//     [CHARACTERS_FATMAN] = {
//         [ITEM_VACUUM]            = sample_encounter
//         [ITEM_BOOKS]             = sample_encounter
//         [ITEM_KNIVES]            = sample_encounter
//         [ITEM_LOLLIPOP]          = sample_encounter
//         [ITEM_COMPUTER]          = sample_encounter
//         [ITEM_VASE]              = sample_encounter
//         [ITEM_COMIC_BOOK]        = sample_encounter
//         [ITEM_FOOTBALL]          = sample_encounter
//         [ITEM_AK47]              = sample_encounter
//         [ITEM_GIRLSCOUT_COOKIES] = sample_encounter
//         [ITEM_STEAK]             = sample_encounter
//         [ITEM_GAME_CONSOLE]      = sample_encounter
//     },
// };

Encounter_Fn get_encounter_fn(void) {
    return encounters[game.current_character][game.current_item];
}
