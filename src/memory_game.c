#include "raylib.h"
#include "rlgl.h"
#include "game.h"

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

void memory_game_init(Game_Parameters parameters) {
    memory_game_switch_state(MEMORY_GAME_STATE_SHOW_SHAPES);
}

void memory_game_update(Game_Parameters parameters) {

    switch (game_state) {
    case MEMORY_GAME_STATE_SHOW_SHAPES: {
        if (timer_update(&memorize_timer, GetFrameTime())) {
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

            float x = ((float)parameters.screen_width) / 2.0f - SPACING * (memory_objects.count - 1) / 2.0f + i * SPACING;
            float y = (float)parameters.screen_height / 2.0f;
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
        Vector2 text_size = MeasureTextEx(parameters.neutral_font, display_str.ptr, parameters.neutral_font.baseSize, 10);
        Vector2 text_position = { parameters.screen_width / 2.0f - text_size.x / 2.0f, parameters.screen_height / 2.0f};
        DrawTextEx(parameters.neutral_font, display_str.ptr, text_position, parameters.neutral_font.baseSize, 10, (Color){255, 255, 255, 255});

        Vector2 mouse = GetMousePosition();

        int hovered = -1;
        float hx, hy;

        const float size = 100.0f;
        for (uint32_t i = 0; i < COUNT_OF_MEMORY_SHAPE; ++i) {
            Color c = { 140, 140, 140, 255 };
            Color c1 = { 255,215,0, 255 };

            float x = ((float)parameters.screen_width) / 2.0f - SPACING * (COUNT_OF_MEMORY_SHAPE - 1) / 2.0f + i * SPACING;
            float y = text_position.y + parameters.neutral_font.baseSize + 150;

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

}
