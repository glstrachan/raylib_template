#include "game.h"
#include "characters.h"
#include "raylib.h"

static const Vector2 position_left = {500,  750};
static const Vector2 position_right = {1150, 750};

static Texture2D salesman_tex;
static Texture2D oldlady_tex;
static Texture2D nerd_tex;
static Texture2D shotgunner_tex;
static Texture2D fatman_tex;

void characters_draw(Character_Type type, Character_Position pos) {
    Texture2D tex;

    switch(type) {
        case CHARACTERS_SALESMAN:
            tex = salesman_tex;
            break;
        case CHARACTERS_OLDLADY:
            tex = oldlady_tex;
            break;
        case CHARACTERS_NERD:
            tex = nerd_tex;
            break;
        case CHARACTERS_SHOTGUNNER:
            tex = shotgunner_tex;
            break;
        case CHARACTERS_FATMAN:
            tex = fatman_tex;
            break;
    }

    Vector2 position = (pos == CHARACTERS_LEFT ? position_left : position_right);
    position.y -= tex.height;

    DrawTexture(tex, position.x, position.y, WHITE);
}

void characters_init() {
    salesman_tex = LoadTexture("resources/salesman.png");
    oldlady_tex = LoadTexture("resources/oldlady.png");
    nerd_tex = LoadTexture("resources/nerd.png");
    shotgunner_tex = LoadTexture("resources/shotgunner.png");
    fatman_tex = LoadTexture("resources/fatman.png");
}

void characters_cleanup() {
    UnloadTexture(salesman_tex);
    UnloadTexture(oldlady_tex);
    UnloadTexture(nerd_tex);
    UnloadTexture(shotgunner_tex);
    UnloadTexture(fatman_tex);
}