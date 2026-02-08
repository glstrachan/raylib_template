#include "raylib.h"
#include "rlgl.h"

#define OC_CORE_IMPLEMENTATION
#include "entity.h"
#include "game.h"
#include "encounter.h"
#include "dialog.h"
#include "characters.h"
#include "pick_items.h"
#include "font_manager.h"

#define CLAY_IMPLEMENTATION
#include "../external/clay/clay.h"
// #include "../external/clay/clay_renderer_raylib.c"

// #include "memory_game.h"

void memory_game_init();
void memory_game_update();

Oc_Arena arena = { 0 };
Oc_Arena frame_arena = { 0 };

void HandleClayErrors(Clay_ErrorData errorData) {
    // See the Clay_ErrorData struct for more information
    printf("%s", errorData.errorText.chars);
    switch(errorData.errorType) {
        default: break;
    }
}

static Texture2D shop_bg_tex, clipboard_text;
static Texture2D title_bg_tex;
static Game_Timer show_fail_timer;
static Game_Timer fade_timer;
static Texture2D bg_tex;
Game_Parameters game_parameters;

Game_State game = { 0 };
Game_Shaders game_shaders = { 0 };

Sound start_sounds[4];

extern Minigame memory_game;
extern Minigame smile_game;
extern Minigame shotgun_game;
extern Minigame rhythm_game;

static Sound failSound;
static Sound successSound;

static float score_needed = 0.7;

static bool is_transitioning = false;

static bool did_doorbell, did_switch;
void game_go_to_state(uint32_t next_state, bool transition) {
    if (!is_transitioning) {
        did_doorbell = false;
        did_switch = false;
    }
	if (transition) {
		timer_init(&fade_timer, 1500);
		game.next_state = next_state;
		is_transitioning = true;
		return;
	}
    switch (next_state) {
    // case GAME_STATE_TRANSITION: {
    //     Image screenshot = LoadImageFromScreen();
    //     game.screenshot = LoadTextureFromImage(screenshot);
    //     timer_init(&game.transition_timer, 2000);
    // } break;
    case GAME_STATE_SELECT_ITEMS: {
        choose_pickable();
    } break;
    case GAME_STATE_SELECT_ENCOUNTER: {
        pick_encounter();
    } break;
    case GAME_STATE_START_DAY: {
        memset(&game.items_sold_today, 0, sizeof(game.items_sold_today));
        pick_encounter_init();
        game_go_to_state(GAME_STATE_SELECT_ENCOUNTER, false);
        return;
    } break;
    case GAME_STATE_IN_ENCOUNTER: {
        game.minigame_scores = 0.0f;
        game.minigame_count = 0;
        encounter_start(game.encounter);
    } break;
    case GAME_STATE_DONE_ENCOUNTER: {
        if (game.minigame_scores < score_needed) {
            PlaySound(failSound);
            timer_init(&show_fail_timer, 4000);
        } else {
            PlaySound(successSound);
            game.current_encounter++;
            game.briefcase.used[game.current_item_index] = 1;

            if (game.current_encounter >= 4) {
                game_go_to_state(GAME_STATE_DAY_SUMMARY, true);
            } else {
                game_go_to_state(GAME_STATE_SELECT_ENCOUNTER, true);
            }
            return;
        }
    } break;
    case GAME_STATE_DAY_SUMMARY: break;
	case GAME_STATE_TUTORIAL: break;
    case GAME_STATE_TITLE: break;
    default: oc_assert(false);
    }

    game.state = next_state;
}

void game_update() {
	if (is_transitioning) {
		if (timer_update(&fade_timer)) {
			is_transitioning = false;
            goto DONE_TRANSITIONING;
		}
		if (game.state != game.next_state) {
			float f = timer_interpolate(&fade_timer);
            if (!did_doorbell && f >= 0.3f && game.next_state == GAME_STATE_IN_ENCOUNTER) {
                did_doorbell = true;
                Sound sound = characters_get_sound(game.current_character);
                PlaySound(sound);
            }
			if (!did_switch && f >= 0.5f) {
                did_switch = true;
				game_go_to_state(game.next_state, false);
			}
		}
	}
    DONE_TRANSITIONING: (void)0;
    switch (game.state) {
    case GAME_STATE_SELECT_ITEMS: {
        pick_items_update();
    } break;
    case GAME_STATE_SELECT_ENCOUNTER: {
        pick_encounter_update();
    } break;
    case GAME_STATE_IN_ENCOUNTER: {
        // TODO: Move this into dialog or smth so we can scriptably change the background
        DrawTexture(bg_tex, 0, 0, WHITE);
        characters_draw(CHARACTERS_SALESMAN, CHARACTERS_LEFT);
        characters_draw(game.current_character, CHARACTERS_RIGHT);

        if (is_transitioning) return;
        encounter_update();
        if (encounter_is_done()) {
            game_go_to_state(GAME_STATE_DONE_ENCOUNTER, true);
        }
    } break;
    case GAME_STATE_DAY_SUMMARY: {
        day_summary_update();
    } break;
    case GAME_STATE_DONE_ENCOUNTER: {
        if (game.minigame_scores < score_needed) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_PERCENT(1),
                        .height = CLAY_SIZING_PERCENT(1),
                    },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
            }) {
                CLAY_TEXT(lit("They did not buy the item"), CLAY_TEXT_CONFIG({ .fontId = FONT_ROBOTO, .fontSize = 80, .textColor = {200, 30, 0, 255} }));
                CLAY_TEXT(lit("FAIL"), CLAY_TEXT_CONFIG({ .fontId = FONT_ROBOTO, .fontSize = 60, .textColor = {200, 30, 0, 255} }));
            }
            if (!is_transitioning && timer_update(&show_fail_timer)) {
                game_go_to_state(GAME_STATE_SELECT_ITEMS, true);
            }
        }
    } break;
	case GAME_STATE_TUTORIAL: {
		DrawTexture(shop_bg_tex, 0, 0, WHITE);

		CLAY(CLAY_ID("Tutotorial"), {
			.floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER } },
			.layout = {
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
				.sizing = {
					.width = CLAY_SIZING_FIXED(clipboard_text.width),
					.height = CLAY_SIZING_FIXED(clipboard_text.height),
				},
				.padding = {70, 50, 200, 60},
				.childGap = 16,
				/* .childAlignment = { .x = CLAY_ALIGN_X_CENTER }, */
			},
			.image = { .imageData = &clipboard_text },
			.cornerRadius = CLAY_CORNER_RADIUS(16)
		}) {
			CLAY_TEXT(oc_format(&frame_arena, "SalesCorp Onboarding"), CLAY_TEXT_CONFIG({ .fontSize = 61, .fontId = FONT_ITIM, .textColor = {0, 0, 0, 255}, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));

			CLAY_TEXT(oc_format(&frame_arena, "Welcome to SalesCorp!"), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {0, 0, 0, 255} }));
			CLAY_AUTO_ID({ .layout = { .sizing = { .height = CLAY_SIZING_FIXED(20) } } });
			CLAY_TEXT(oc_format(&frame_arena, "As a principled salesman, you will be selling items to our valuable customers."), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {0, 0, 0, 255} }));
			CLAY_AUTO_ID({ .layout = { .sizing = { .height = CLAY_SIZING_FIXED(20) } } });
			CLAY_TEXT(oc_format(&frame_arena, "You have twelve items to sell, you can only sell four of them per day."), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {0, 0, 0, 255} }));
			CLAY_AUTO_ID({ .layout = { .sizing = { .height = CLAY_SIZING_FIXED(20) } } });
			CLAY_TEXT(oc_format(&frame_arena, "Try to match the best item for a customer to increase chance of a sale."), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {0, 0, 0, 255} }));

			CLAY_AUTO_ID({
				.layout = {
					.sizing = {
						.height = CLAY_SIZING_GROW(),
					},
				},
			});

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
					CLAY_TEXT((CLAY_STRING("Continue")), CLAY_TEXT_CONFIG({ .fontSize = 40, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255} }));
					if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
						PlaySound(game_sounds.button_click);
						game_go_to_state(GAME_STATE_SELECT_ITEMS, true);
					}
				}
			}
		}

		/* DrawTexture(clipboard_text, game_parameters.screen_width/2.0f - clipboard_text.width/2.0f, game_parameters.screen_height/2.0f - clipboard_text.height/2.0f, WHITE); */
	} break;
    case GAME_STATE_TITLE: {
        DrawTexture(title_bg_tex, 0, 0, WHITE);

        CLAY_AUTO_ID({
            .floating = { .offset = { -10, 10 }, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_RIGHT_TOP, CLAY_ATTACH_POINT_RIGHT_TOP } },
            .layout = {
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                .padding = {25, 25, 0, 0},
                .sizing = {
                    .width = CLAY_SIZING_FIT(0),
                    .height = CLAY_SIZING_FIT(0)
                },
            },
            // .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
            .backgroundColor = {255, 0, 0, 80},
            .cornerRadius = CLAY_CORNER_RADIUS(6),
        }) {
            CLAY_TEXT((CLAY_STRING("Early Access")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {0, 0, 0, 80} }));
        }

        CLAY_AUTO_ID({
            .floating = { .offset = { 0, -150 }, .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_BOTTOM, CLAY_ATTACH_POINT_CENTER_BOTTOM } },
            .layout = {
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                .padding = {25, 25, 10, 10},
                .sizing = {
                    .width = CLAY_SIZING_FIT(0),
                    .height = CLAY_SIZING_FIT(0)
                },
            },
            .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
            .custom = { .customData = make_cool_background() },
            .backgroundColor = {255, 0, 0, 0},
            .cornerRadius = CLAY_CORNER_RADIUS(6),
        }) {
            CLAY_TEXT((CLAY_STRING("Press Space to Start")), CLAY_TEXT_CONFIG({ .fontSize = 60, .fontId = FONT_ITIM, .textColor = {255, 255, 255, 255 * (0.3 + 0.5 * (sin(GetTime() * 2.0) * 0.5 + 0.5))} }));
        }

        if(IsKeyPressed(KEY_SPACE)) {
            game_go_to_state(GAME_STATE_TUTORIAL, true);
        }
    } break;
    default: oc_assert(false);
    }
}

void game_hover_audio(Clay_ElementId elementId, Clay_PointerData pointerInfo, void *userData) {
    PlaySound(*((Sound*)userData));
}

Clay_Arena global_clay_arena;
Font_Manager* global_font_manager;
Music hold_music;
Game_Sounds game_sounds = { 0 };

int entry_main(void)
{
    const int screenWidth = 1920;
    const int screenHeight = 1080;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Traveling Salesman Problems");
    InitAudioDevice();
    SetTargetFPS(0);

    failSound = LoadSound("resources/sounds/failure.wav");
    successSound = LoadSound("resources/sounds/success.wav");
    start_sounds[0] = LoadSound("resources/sounds/casino_a.mp3");
    start_sounds[1] = LoadSound("resources/sounds/casino_b.mp3");
    start_sounds[2] = LoadSound("resources/sounds/casino_c.mp3");
    start_sounds[3] = LoadSound("resources/sounds/casino_d.mp3");
    hold_music = LoadMusicStream("resources/music/hold music.wav");

	game_sounds.button_click = LoadSound("resources/sounds/ui_click-1.wav");
	game_sounds.button_click1 = LoadSound("resources/sounds/ui_click-2.wav");
	game_sounds.button_hover = LoadSound("resources/sounds/ui_hover.wav");
    shop_bg_tex = LoadTexture("resources/background_shop.png");
    title_bg_tex = LoadTexture("resources/title.png");
	clipboard_text = LoadTexture("resources/clipboard.png");

    Camera2D camera = { 0 };
    camera.offset = (Vector2) { 0.0f, 0.0f };
    camera.zoom = 1.0f;

    
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clay_arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(clay_arena, (Clay_Dimensions) { screenWidth, screenHeight }, (Clay_ErrorHandler) { HandleClayErrors, NULL });
    
    game_shaders.background_shader = LoadShader(0, "resources/dialogbackground.fs");

    Font_Manager font_manager = {
        .arena = &arena,
        .font_paths = (const char *[]) {
            // [0] = "",
            [0] = "resources/Roboto-Light.ttf",
            [FONT_ROBOTO] = "resources/Roboto-Light.ttf",
            [FONT_ITIM]   = "resources/Itim.ttf",
        },
    };
    font_manager_get_font(&font_manager, 2, 25);
    global_font_manager = &font_manager;
    Clay_SetMeasureTextFunction(Raylib_MeasureText, global_font_manager);

    game_parameters = (Game_Parameters) {
        .screen_width = screenWidth,
        .screen_height = screenHeight,
    };

    oc_arena_alloc(&frame_arena, 1);

    {
        Oc_Arena_Save save = oc_arena_save(&frame_arena);

        dialog_init();
        characters_init();
        pick_items_init();
        pick_encounter_init();
        items_init();
        choose_pickable();

        oc_arena_restore(&frame_arena, save);
    }


    bg_tex = LoadTexture("resources/background.png");

    game_go_to_state(GAME_STATE_TITLE, false);
    // game_go_to_state(GAME_STATE_IN_ENCOUNTER, true);
    // game.state = GAME_STATE_IN_ENCOUNTER;

    game.items_sold_today[0] = (Item_Sold) {
        .item = ITEM_VACUUM,
        .character = CHARACTERS_NERD,
    };

    PlayMusicStream(hold_music);
    SetMusicVolume(hold_music, 0.1f);

    // shotgun_game.init();
    // rhythm_game.init();
    // memory_game.init();
    // smile_game.init();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        Oc_Arena_Save save = oc_arena_save(&frame_arena);
        game_parameters.screen_width = GetScreenWidth();
        game_parameters.screen_height = GetScreenHeight();

        if (game.current_prerender) game.current_prerender();

        Clay_SetLayoutDimensions((Clay_Dimensions) { game_parameters.screen_width, game_parameters.screen_height });
        Clay_SetPointerState((Clay_Vector2) { GetMouseX(), GetMouseY() }, IsMouseButtonDown(MOUSE_LEFT_BUTTON));
        Clay_UpdateScrollContainers(true, (Clay_Vector2) { GetMouseWheelMove(), 0.0 }, dt);

        Clay_BeginLayout();

        UpdateMusicStream(hold_music);

        BeginDrawing();
            ClearBackground((Color){40, 40, 40, 255});
            BeginMode2D(camera);
                game_update();
                // pick_items_update();
                // smile_game.update();
                // shotgun_game.update();
                // rhythm_game.update();
                // memory_game.update();

                Clay_RenderCommandArray renderCommands = Clay_EndLayout();
                Clay_Raylib_Render(renderCommands, global_font_manager);
                

                if (is_transitioning) {
                    float interp = timer_interpolate(&fade_timer);
                    interp = interp * 2.0f - 1.0f;
                    if (interp < 0.0f) interp = -interp;
                    interp = 1.0f - interp;
                    interp = Clamp(interp * 2, 0.0f, 1.0f);

                    Clay_RenderCommand rcmd = {
                        .commandType = CLAY_RENDER_COMMAND_TYPE_RAYLIB,
                        .renderData = {
                            .raylib = {
                                .point = {0, 0},
                                .size = {game_parameters.screen_width, game_parameters.screen_height},
                                .color = { 0, 0, 0, 255 * interp }
                            },
                        },
                    };
                    Clay_RenderCommandArray arr = {
                        .length = 1,
                        .capacity = 1,
                        .internalArray = &rcmd,
                    };

                    Clay_Raylib_Render(arr, global_font_manager);
                }
            EndMode2D();

        EndDrawing();

        oc_arena_restore(&frame_arena, save);
    }

    UnloadTexture(bg_tex);

    items_cleanup();
    pick_items_cleanup();
    characters_cleanup();
    dialog_cleanup();

    CloseWindow();

    return 0;
}
