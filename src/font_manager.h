#pragma once

#include <stdint.h>

#include "../external/core.h"
#include "raylib.h"

typedef uint64_t Font_Id;

typedef struct {
    Oc_Arena* arena;
    Array(Hash_Map(uint32_t, Font)) cache;
    const char* const* font_paths;
} Font_Manager;

#define FONT(font, size) (((uint64_t)font_face << 32) | size)

Font font_manager_get_font_from_id(Font_Manager* fm, Font_Id font_id);
Font font_manager_get_font(Font_Manager* fm, uint32_t font_face, uint32_t size);
Font_Id font_manager_get_font_id(Font_Manager* fm, uint32_t font_face, uint32_t size);
