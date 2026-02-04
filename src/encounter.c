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

    // dialog_text("Old Lady", "You darn whippersnappers!!");
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

extern Minigame memory_game, smile_game;
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
        "Show her the attachments", 
        "Demonstrate the suction", 
        "Talk about the warranty",
        "Compliment her carpet")) {
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
        "Appeal to her grandchildren", 
        "Emphasize the leather binding", 
        "Mention the large print edition",
        "Talk about the history sections")) {
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
        "Show the bread knife", 
        "Demonstrate on vegetables", 
        "Talk about kitchen safety",
        "Mention the storage block")) {
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
        "Classic cherry", 
        "Lavender honey", 
        "Bourbon vanilla",
        "Green apple")) {
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
        "Emphasize family video calls", 
        "Talk about online shopping", 
        "Mention digital photo albums",
        "Discuss medical resources")) {
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
        "Show the spec sheet", 
        "Challenge his vacuum knowledge", 
        "Appeal to the aesthetic design",
        "Mention the smart home features")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Mention the collectors' value", 
        "Talk about the illustrations", 
        "Emphasize offline access",
        "Bring up the bibliography sections")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Show him the blade up close", 
        "Mention the forging process", 
        "Talk about the handle material",
        "Discuss the edge angle")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Organic cane sugar", 
        "Check the ingredients list", 
        "Offer a sugar-free option",
        "Deflect with flavor talk")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Focus on pre-built convenience", 
        "Highlight the warranty", 
        "Mention specific components",
        "Talk about software included")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Emphasize noise level", 
        "Talk about durability", 
        "Mention intruder detection",
        "Discuss debris removal")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Discuss tactical placement", 
        "Mention historical warfare", 
        "Talk about survivalism sections",
        "Emphasize weight and thickness")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Show the chef's knife", 
        "Present the hunting knife", 
        "Display the cleaver",
        "Reveal the throwing knives")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Bourbon vanilla", 
        "Whiskey caramel", 
        "Smoked maple",
        "Black coffee")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Mention the mute switch", 
        "Talk about air-gapped systems", 
        "Suggest offline use only",
        "Discuss encryption options")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Mention snack preservation", 
        "Talk about comfort features", 
        "Emphasize minimal effort",
        "Discuss remote control option")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Mention food-related entries", 
        "Talk about comfortable reading", 
        "Emphasize food history sections",
        "Discuss cookbook supplements")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Show the vegetable knife", 
        "Present the meat carving knife", 
        "Display the bread knife",
        "Reveal the pizza cutter")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Salted caramel", 
        "Chocolate truffle", 
        "Maple bacon",
        "Butter pecan")) {
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
    
    encounter_minigame(&smile_game);
    
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
        "Streaming capabilities", 
        "Food delivery convenience", 
        "Large screen for recipes",
        "Gaming potential")) {
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
    
    encounter_minigame(&smile_game);
    
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
// ENCOUNTER FUNCTION POINTER ARRAY
// ============================================================================

const Encounter_Fn encounters[CHARACTERS_COUNT][ITEM_COUNT] = {
    [CHARACTERS_OLDLADY] = {
        [ITEM_VACUUM] = encounter_oldlady_vacuum,
        [ITEM_BOOKS] = encounter_oldlady_books,
        [ITEM_KNIVES] = encounter_oldlady_knives,
        [ITEM_LOLLIPOP] = encounter_oldlady_lollipop,
        [ITEM_COMPUTER] = encounter_oldlady_computer,
    },
    [CHARACTERS_NERD] = {
        [ITEM_VACUUM] = encounter_nerd_vacuum,
        [ITEM_BOOKS] = encounter_nerd_books,
        [ITEM_KNIVES] = encounter_nerd_knives,
        [ITEM_LOLLIPOP] = encounter_nerd_lollipop,
        [ITEM_COMPUTER] = encounter_nerd_computer,
    },
    [CHARACTERS_SHOTGUNNER] = {
        [ITEM_VACUUM] = encounter_shotgunner_vacuum,
        [ITEM_BOOKS] = encounter_shotgunner_books,
        [ITEM_KNIVES] = encounter_shotgunner_knives,
        [ITEM_LOLLIPOP] = encounter_shotgunner_lollipop,
        [ITEM_COMPUTER] = encounter_shotgunner_computer,
    },
    [CHARACTERS_FATMAN] = {
        [ITEM_VACUUM] = encounter_fatman_vacuum,
        [ITEM_BOOKS] = encounter_fatman_books,
        [ITEM_KNIVES] = encounter_fatman_knives,
        [ITEM_LOLLIPOP] = encounter_fatman_lollipop,
        [ITEM_COMPUTER] = encounter_fatman_computer,
    },
};


// const Encounter_Fn encounters[CHARACTERS_COUNT][ITEM_COUNT] = {
//     [CHARACTERS_SALESMAN] = {
//         [ITEM_VACUUM]   = sample_encounter,
//         [ITEM_BOOKS]    = sample_encounter,
//         [ITEM_KNIVES]   = sample_encounter,
//         [ITEM_LOLLIPOP] = sample_encounter,
//         [ITEM_COMPUTER] = sample_encounter,
//     },
//     [CHARACTERS_OLDLADY] = {
//         [ITEM_VACUUM]   = sample_encounter,
//         [ITEM_BOOKS]    = sample_encounter,
//         [ITEM_KNIVES]   = sample_encounter,
//         [ITEM_LOLLIPOP] = sample_encounter,
//         [ITEM_COMPUTER] = sample_encounter,
//     },
//     [CHARACTERS_NERD] = {
//         [ITEM_VACUUM]   = sample_encounter,
//         [ITEM_BOOKS]    = sample_encounter,
//         [ITEM_KNIVES]   = sample_encounter,
//         [ITEM_LOLLIPOP] = sample_encounter,
//         [ITEM_COMPUTER] = sample_encounter,
//     },
//     [CHARACTERS_SHOTGUNNER] = {
//         [ITEM_VACUUM]   = sample_encounter,
//         [ITEM_BOOKS]    = sample_encounter,
//         [ITEM_KNIVES]   = sample_encounter,
//         [ITEM_LOLLIPOP] = sample_encounter,
//         [ITEM_COMPUTER] = sample_encounter,
//     },
//     [CHARACTERS_FATMAN] = {
//         [ITEM_VACUUM]   = sample_encounter,
//         [ITEM_BOOKS]    = sample_encounter,
//         [ITEM_KNIVES]   = sample_encounter,
//         [ITEM_LOLLIPOP] = sample_encounter,
//         [ITEM_COMPUTER] = sample_encounter,
//     },
// };

Encounter_Fn get_encounter_fn(void) {
    return encounters[game.current_character][game.current_item];
}
