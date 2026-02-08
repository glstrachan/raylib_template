#include "game.h"
#include "characters.h"
#include "raylib.h"

Character_Data character_data[] = {
    #define X(id, name) [id] = { lit(name) },
    CHARACTERS_LIST(X)
    #undef X
};

static const Vector2 position_left = {500,  750};
static const Vector2 position_right = {1150, 750};

static Texture2D salesman_tex;
static Texture2D oldlady_tex;
static Texture2D nerd_tex;
static Texture2D shotgunner_tex;
static Texture2D fatman_tex;

static Sound doorbells[4];

Texture2D* characters_get_texture(Character_Type type) {
    switch(type) {
        case CHARACTERS_SALESMAN:   return &salesman_tex;
        case CHARACTERS_OLDLADY:    return &oldlady_tex;
        case CHARACTERS_NERD:       return &nerd_tex;
        case CHARACTERS_SHOTGUNNER: return &shotgunner_tex;
        case CHARACTERS_FATMAN:     return &fatman_tex;
        case CHARACTERS_COUNT:
        case CHARACTERS_NONE: oc_assert(false); break;
    }
}

Sound characters_get_sound(Character_Type type) {
    switch(type) {
        case CHARACTERS_OLDLADY:    return doorbells[0];
        case CHARACTERS_NERD:       return doorbells[1];
        case CHARACTERS_SHOTGUNNER: return doorbells[2];
        case CHARACTERS_FATMAN:     return doorbells[3];
        case CHARACTERS_SALESMAN:
        case CHARACTERS_COUNT:
        case CHARACTERS_NONE: oc_assert(false); break;
    }
}

void characters_draw(Character_Type type, Character_Position pos) {
    Texture2D tex = *characters_get_texture(type);

    Vector2 position = (pos == CHARACTERS_LEFT ? position_left : position_right);
    position.y -= tex.height;

    DrawTexture(tex, position.x, position.y, WHITE);
}

void characters_init() {
    salesman_tex = LoadTexture("resources/salesman.png");
    SetTextureWrap(salesman_tex, TEXTURE_WRAP_CLAMP);
    oldlady_tex = LoadTexture("resources/oldlady.png");
    SetTextureWrap(oldlady_tex, TEXTURE_WRAP_CLAMP);
    nerd_tex = LoadTexture("resources/nerd.png");
    SetTextureWrap(nerd_tex, TEXTURE_WRAP_CLAMP);
    shotgunner_tex = LoadTexture("resources/shotgunner.png");
    SetTextureWrap(shotgunner_tex, TEXTURE_WRAP_CLAMP);
    fatman_tex = LoadTexture("resources/fatman.png");
    SetTextureWrap(fatman_tex, TEXTURE_WRAP_CLAMP);


    doorbells[0] = LoadSound("resources/sounds/doorbell_a.wav");
    doorbells[1] = LoadSound("resources/sounds/doorbell_b.wav");
    doorbells[2] = LoadSound("resources/sounds/doorbell_c.wav");
    doorbells[3] = LoadSound("resources/sounds/doorbell_d.wav");
}

void characters_cleanup() {
    UnloadTexture(salesman_tex);
    UnloadTexture(oldlady_tex);
    UnloadTexture(nerd_tex);
    UnloadTexture(shotgunner_tex);
    UnloadTexture(fatman_tex);
}
