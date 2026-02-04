#pragma once

Enum(Character_Type, uint32_t,
    CHARACTERS_SALESMAN,
    CHARACTERS_OLDLADY,
    CHARACTERS_NERD,
    CHARACTERS_SHOTGUNNER,
    CHARACTERS_FATMAN,
    CHARACTERS_COUNT,
);

Enum(Character_Position, uint32_t,
    CHARACTERS_LEFT,
    CHARACTERS_RIGHT
);

void characters_draw(Character_Type type, Character_Position pos);
void characters_init();
void characters_cleanup();