#include "raylib.h"
#include "rlgl.h"
#include "game.h"
#include "../external/clay/clay.h"
#include "../external/clay/clay_renderer_raylib.h"

Enum(Memory_Color, uint32_t,
    MEMORY_COLOR_RED,
    MEMORY_COLOR_GREEN,
    MEMORY_COLOR_BLUE,
    MEMORY_COLOR_YELLOW,

    COUNT_OF_MEMORY_COLOR,
);

Enum(Memory_Shape, uint32_t,
    MEMORY_SHAPE_SQUARE,
    MEMORY_SHAPE_TRIANGLE,
    MEMORY_SHAPE_CIRCLE,

    COUNT_OF_MEMORY_SHAPE,
);

Enum(Memory_Question, uint32_t,
    MEMORY_QUESTION_NTH_SHAPE,
    MEMORY_QUESTION_NTH_COLOR,
    MEMORY_QUESTION_FULL_SEQUENCE,

    COUNT_OF_MEMORY_QUESTION,
);

Enum(Memory_Game_State, uint32_t,
    MEMORY_GAME_STATE_SHOW_SHAPES,
    MEMORY_GAME_STATE_ASK_QUESTION,
    MEMORY_GAME_STATE_INPUT_ANSWER,
);

typedef struct {
    Memory_Shape shape;
    Memory_Color color;
} Memory_Object;

const static float SPACING = 150.0f;

static Memory_Game_State game_state = MEMORY_GAME_STATE_ASK_QUESTION;
static union {
    struct {
        Memory_Question question;
        uint8_t n;
    } game_question;
} game_data;

static int iterations;
static Game_Timer memorize_timer;

static Array(Memory_Object) memory_objects = { 0 };

void memory_game_switch_state(Memory_Game_State next_state) {
    switch (next_state) {
    case MEMORY_GAME_STATE_SHOW_SHAPES: {
        memory_objects.count = 0;
        int count = GetRandomValue(3, 8);
        for (int i = 0; i < count; ++i) {
            int shape = GetRandomValue(0, COUNT_OF_MEMORY_SHAPE-1);
            int color = GetRandomValue(0, COUNT_OF_MEMORY_COLOR-1);

            oc_array_append(&arena, &memory_objects, ((Memory_Object){shape, color}));
        }

        timer_init(&memorize_timer, 2000);
    } break;
    case MEMORY_GAME_STATE_ASK_QUESTION: {
        Memory_Question question = GetRandomValue(0, COUNT_OF_MEMORY_QUESTION - 1);
        game_data.game_question.question = question;
        switch (question) {
        case MEMORY_QUESTION_NTH_SHAPE:
        case MEMORY_QUESTION_NTH_COLOR:
            game_data.game_question.n = GetRandomValue(1, memory_objects.count);
            break;
        case MEMORY_QUESTION_FULL_SEQUENCE:
            break;
        }
    } break;
    }

    game_state = next_state;
}

extern void HandleClayErrors(Clay_ErrorData errorData);
static Clay_Context* this_ctx;
static Shader background_shader;

void memory_game_init() {
    memory_game_switch_state(MEMORY_GAME_STATE_SHOW_SHAPES);
    iterations = 0;

    Clay_Context* old_ctx = Clay_GetCurrentContext();

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_SetCurrentContext(NULL);
    this_ctx = Clay_Initialize(arena, (Clay_Dimensions) { game_parameters.screen_width, game_parameters.screen_height }, (Clay_ErrorHandler) { HandleClayErrors });
    Clay_SetCurrentContext(old_ctx);

    background_shader = LoadShader(0, "resources/dialogbackground.fs");
}

bool memory_game_update() {
    Clay_Context* old_ctx = Clay_GetCurrentContext();
    Clay_SetCurrentContext(this_ctx);
    // Clay_Initialize(global_clay_arena, (Clay_Dimensions) {game_parameters.screen_width, game_parameters.screen_height}, (Clay_ErrorHandler) { HandleClayErrors });

    Clay_SetLayoutDimensions((Clay_Dimensions) { game_parameters.screen_width, game_parameters.screen_height });
    Clay_SetPointerState((Clay_Vector2) { GetMouseX(), GetMouseY() }, IsMouseButtonDown(MOUSE_LEFT_BUTTON));
    Clay_UpdateScrollContainers(true, (Clay_Vector2) { GetMouseWheelMove(), 0.0 }, GetFrameTime());

    Clay_BeginLayout();

    CustomLayoutElement* customBackgroundData = oc_arena_alloc(&frame_arena, sizeof(CustomLayoutElement));
    customBackgroundData->type = CUSTOM_LAYOUT_ELEMENT_TYPE_BACKGROUND;
    customBackgroundData->customData.background = (CustomLayoutElement_Background) { background_shader };


    CLAY(CLAY_ID("DialogBox"), {
        .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .attachPoints = { CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER } },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_PERCENT(0.8),
                .height = CLAY_SIZING_PERCENT(0.6)
            },
            .padding = {16, 16, 16, 16},
            .childGap = 16
        },
        .border = { .width = { 3, 3, 3, 3, 0 }, .color = {135, 135, 135, 255} },
        .custom = { .customData = customBackgroundData },
        .cornerRadius = CLAY_CORNER_RADIUS(16)
    }) {
    }

    Clay_RenderCommandArray renderCommands = Clay_EndLayout();
    extern Font* global_clay_fonts;
    Clay_Raylib_Render(renderCommands, global_clay_fonts);
    Clay_SetCurrentContext(old_ctx);

    switch (game_state) {
    case MEMORY_GAME_STATE_SHOW_SHAPES: {
        if (iterations > 4) {
            return true;
        }
        if (timer_update(&memorize_timer)) {
            iterations++;
            memory_game_switch_state(MEMORY_GAME_STATE_ASK_QUESTION);
            break;
        }

        for (uint32_t i = 0; i < memory_objects.count; ++i) {
            Color c;
            switch (memory_objects.items[i].color) {
            case MEMORY_COLOR_RED:    c = (Color) { 255, 0, 0, 255 }; break;
            case MEMORY_COLOR_GREEN:  c = (Color) { 0, 255, 0, 255 }; break;
            case MEMORY_COLOR_BLUE:   c = (Color) { 0, 0, 255, 255 }; break;
            case MEMORY_COLOR_YELLOW: c = (Color) { 255, 255, 0, 255 }; break;
            default: oc_assert(false); break;
            }

            float x = ((float)game_parameters.screen_width) / 2.0f - SPACING * (memory_objects.count - 1) / 2.0f + i * SPACING;
            float y = (float)game_parameters.screen_height / 2.0f;
            const float size = 100.0f;

            switch (memory_objects.items[i].shape) {
            case MEMORY_SHAPE_SQUARE:   DrawRectangleV((Vector2){ x - size / 2.0f, y - size / 2.0f }, (Vector2) { size, size }, c); break;
            case MEMORY_SHAPE_CIRCLE:   DrawCircleV((Vector2){ x, y }, size / 2.0f, c); break;
            case MEMORY_SHAPE_TRIANGLE: DrawTriangle((Vector2){ x, y - size / 2.0f }, (Vector2){ x - size / 3.0f * 1.732f, y + size / 2.0f }, (Vector2){ x + size / 3.0f * 1.732f, y + size / 2.0f }, c); break;
            default: oc_assert(false); break;
            }
        }
    } break;
    case MEMORY_GAME_STATE_ASK_QUESTION: {
        const char* question;
        Oc_String_Builder builder;
        oc_sb_init(&builder, &frame_arena);
        switch (game_data.game_question.question) {
        case MEMORY_QUESTION_NTH_SHAPE:
            switch (game_data.game_question.n) {
            case 1: wprint(&builder.writer, "What was the shape of the 1st item?"); break;
            case 2: wprint(&builder.writer, "What was the shape of the 2nd item?"); break;
            case 3: wprint(&builder.writer, "What was the shape of the 3rd item?"); break;
            default: wprint(&builder.writer, "What was the shape of the {}th item?", game_data.game_question.n); break;
            }
            break;
        case MEMORY_QUESTION_NTH_COLOR:
            switch (game_data.game_question.n) {
            case 1: wprint(&builder.writer, "What was the color of the 1st item?"); break;
            case 2: wprint(&builder.writer, "What was the color of the 2nd item?"); break;
            case 3: wprint(&builder.writer, "What was the color of the 3rd item?"); break;
            default: wprint(&builder.writer, "What was the color of the {}th item?", game_data.game_question.n); break;
            }
            break;
        case MEMORY_QUESTION_FULL_SEQUENCE:
            wprint(&builder.writer, "What was the sequence of items?"); break;
            break;
        }

        string display_str = oc_sb_to_string(&builder);
        Vector2 text_size = MeasureTextEx(game_parameters.neutral_font, display_str.ptr, game_parameters.neutral_font.baseSize, 10);
        Vector2 text_position = { game_parameters.screen_width / 2.0f - text_size.x / 2.0f, game_parameters.screen_height / 2.0f};
        DrawTextEx(game_parameters.neutral_font, display_str.ptr, text_position, game_parameters.neutral_font.baseSize, 10, (Color){255, 255, 255, 255});

        Vector2 mouse = GetMousePosition();

        int hovered = -1;
        float hx, hy;

        const float size = 100.0f;
        for (uint32_t i = 0; i < COUNT_OF_MEMORY_SHAPE; ++i) {
            Color c = { 140, 140, 140, 255 };
            Color c1 = { 255,215,0, 255 };

            float x = ((float)game_parameters.screen_width) / 2.0f - SPACING * (COUNT_OF_MEMORY_SHAPE - 1) / 2.0f + i * SPACING;
            float y = text_position.y + game_parameters.neutral_font.baseSize + 150;

            float dx = x - mouse.x;
            float dy = y - mouse.y;

            if (dx * dx + dy * dy < size * size && hovered == -1) {
                hx = x;
                hy = y;
                hovered = i;
                c = (Color){ 200, 200, 200, 255 };
            }

            switch (i) {
            case MEMORY_SHAPE_SQUARE:   DrawRectangleV((Vector2){ x - size / 2.0f, y - size / 2.0f }, (Vector2) { size, size }, c); break;
            case MEMORY_SHAPE_CIRCLE:   DrawCircleV((Vector2){ x, y }, size / 2.0f, c); break;
            case MEMORY_SHAPE_TRIANGLE: DrawTriangle((Vector2){ x, y - size / 2.0f }, (Vector2){ x - size / 3.0f * 1.732f, y + size / 2.0f }, (Vector2){ x + size / 3.0f * 1.732f, y + size / 2.0f }, c); break;
            default: oc_assert(false); break;
            }

            switch (i) {
            case MEMORY_SHAPE_SQUARE:   DrawRectangleLinesEx((Rectangle){ x - size / 2.0f, y - size / 2.0f,  size, size  }, 2, c1); break;
            case MEMORY_SHAPE_CIRCLE:   DrawCircleLinesV((Vector2){ x, y }, size / 2.0f, c1); DrawCircleLinesV((Vector2){ x, y }, size / 2.0f - 1, c1); break;
            case MEMORY_SHAPE_TRIANGLE:
                DrawTriangleLines((Vector2){ x, y - size / 2.0f }, (Vector2){ x - size / 3.0f * 1.732f, y + size / 2.0f }, (Vector2){ x + size / 3.0f * 1.732f, y + size / 2.0f }, c1);
                DrawTriangleLines((Vector2){ x, y - size / 2.0f + 2 }, (Vector2){ x - size / 3.0f * 1.732f + 2, y + size / 2.0f - 1 }, (Vector2){ x + size / 3.0f * 1.732f - 2, y + size / 2.0f - 1 }, c1);
                break;
            default: oc_assert(false); break;
            }
        }

        float dx = hx - mouse.x;
        float dy = hy - mouse.y;

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && (dx * dx + dy * dy < size * size)) {
            memory_game_switch_state(MEMORY_GAME_STATE_SHOW_SHAPES);
            break;
        }

    } break;
    }

    return false;
}

Minigame memory_game = {
    .init = memory_game_init,
    .update = memory_game_update,
};
