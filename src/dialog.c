#include "raylib.h"
#include "rlgl.h"
#include "game.h"

Enum(Dialog_Type, uint32_t,
    DIALOG_TYPE_SELECTION,
    DIALOG_TYPE_TEXT,
    DIALOG_TYPE_SMTH_ELSE,
    DIALOG_TYPE_END
);

Enum(Dialog_Decoration_Type, uint32_t,
    DIALOG_TYPE_DEFAULT = 0,
    DIALOG_TYPE_ANGRY,
    DIALOG_TYPE_SURPRISED    
);

typedef struct {
    Dialog_Type type;

    union {
        struct {
            struct {
                string name; 
                uint32_t next_dialog_item;
            } entries[8];

            uint8_t count;
        } selections;
        
        struct {
            bool player_talking;
            string speaker_name;
            string text;
            Dialog_Decoration_Type decoration;
            uint32_t next_dialog_item;
        } text;

        struct {

        } smth_else;
    };
} _Dialog_Item;

typedef void (*Dialog_Item)();

static uint32_t total_chars;
static uint32_t printed_chars;
static Game_Timer char_timer;
static Dialog_Item current_dialog = NULL;

typedef struct {
    string name;
} Dialog_Selection;

static int dialog_branches[1024];
static int dialog_max_item;
static int dialog_current_item;
static int dialog_current_branch;
static bool first_time;

int _dialog_selection(int count, Dialog_Selection items[]) {
    return 0;
}

#define dialog_selection(...) ({                                 \
    int selection;                                               \
    if (dialog_current_item < dialog_max_item) {                 \
        dialog_current_item++;                                   \
        selection = dialog_branches[dialog_current_branch++];    \
    } else if (dialog_current_item == dialog_max_item) {         \
        Dialog_Selection items[] = { __VA_ARGS__ };              \
        selection = _dialog_selection(oc_len(items), items);     \
        if (selection < 0) { \
            first_time = false; \
            return;                               \
        } else {                                                   \
            dialog_branches[dialog_current_branch++] = selection;\
            dialog_current_item++;                               \
            dialog_max_item++;                                   \
            first_time = true; \
        }                                                        \
    } else {                                                     \
        oc_assert(false); \
    }                                                            \
    selection;                                                   \
})

bool _dialog_text(string speaker_name, string text, Dialog_Decoration_Type decoration) {

    // draw the text
    if (first_time) {
        total_chars = text.len;
        printed_chars = 0;
        timer_init(&char_timer, 100);
    }

    if(total_chars < printed_chars) {
        if (timer_update(&char_timer)) {
            timer_reset(&char_timer);
            printed_chars++;
        }
        
        if(IsKeyDown(KEY_SPACE)) {
            printed_chars = total_chars;
        }
    }

    // Figure out some bounds for the text box
    // Lets say its 40% of the screen wide
    // and 10% of the screen tall

    // DrawRectangle(game_parameters->screen_width / 2, game_parameters->screen_height - 100, game_parameters->screen_width * 0.8, 50, RED);

    // Draw a text box
    // Draw the speaker
    // Draw the text

    bool result = false;

    if(total_chars == printed_chars) {
        // Draw a continue prompt

        if (IsKeyPressed(KEY_E)) {
            result = true;
        }
    }

    
    Oc_String_Builder builder;
    oc_sb_init(&builder, &frame_arena);

    wprint(&builder.writer, "{}", string_slice(text, 0, printed_chars));
    string display_str = oc_sb_to_string(&builder);

    Vector2 text_size = MeasureTextEx(game_parameters.neutral_font, display_str.ptr, game_parameters.neutral_font.baseSize, 10);
    Vector2 text_position = { game_parameters.screen_width / 2.0f - text_size.x / 2.0f, game_parameters.screen_height / 2.0f};
    DrawTextEx(game_parameters.neutral_font, display_str.ptr, text_position, game_parameters.neutral_font.baseSize, 10, (Color){255, 255, 255, 255});

    return result;
}

#define dialog_text(speaker_name, text, decoration) do {          \
    if (dialog_current_item < dialog_max_item) {                  \
        dialog_current_item++;                                    \
    } else if (dialog_current_item == dialog_max_item) {           \
        if (_dialog_text(_Generic((speaker_name), string : (speaker_name), default: lit(speaker_name)), _Generic((text), string : (text), default: lit(text)), (decoration))) { \
            dialog_current_item++;                                \
            dialog_max_item++;                                    \
            first_time = true; \
        } else {                                                  \
            first_time = false; \
            return;                                               \
        }                                                         \
    } else oc_assert(false);                                      \
} while (0)

void sample_dialog() {
    dialog_text("john", "hello, how are you?", 0);

    switch (dialog_selection(
        { "Potato" },
        { "Apple" },
        { "Cellary" },
    )) {
        // case 0: handle_potato();
        // case 1: handle_apple();
        // case 2: handle_cellary();
    }}



// static inline
// void dialog_handle_text(Game_Parameters* game_parameters) {
// }

// static inline
// void dialog_handle_selection() {
//     // Draw all of the different options
//     // Needs to handle the user clicking and set the next dialog
// }

// static inline
// void dialog_handle_smth_else() {

// }


void dialog_play(Dialog_Item dialog) {
    // switch (dialog->type) {
    //     case DIALOG_TYPE_TEXT:
    //         total_chars = dialog->text.text.len;
    //         printed_chars = 0;
    //         timer_init(&char_timer, 100);
    //     break;
    //     // TODO: handle others
    // }

    first_time = true;
    current_dialog = dialog;
    dialog_max_item = 0;
}

void dialog_update() {
    if(!current_dialog) return;

    dialog_current_item = 0;
    dialog_current_branch = 0;

    current_dialog();

    // switch(current_dialog->type) {
    //     case DIALOG_TYPE_TEXT:
    //         dialog_handle_text(game_parameters);
    //     break;
    //     case DIALOG_TYPE_SELECTION:
    //         dialog_handle_selection();
    //     break;
    //     case DIALOG_TYPE_SMTH_ELSE:
    //         dialog_handle_smth_else();
    //     break;
    //     case DIALOG_TYPE_END:
    //         current_dialog = NULL;
    //     break;
    // }
}
