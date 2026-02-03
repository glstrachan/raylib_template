#pragma once

#include "../external/core.h"
#include "raylib.h"

typedef uint64_t Font_Id;

typedef struct {
    Arena* arena;
    Array(uint32_t, Hash_Map(uint32_t, Font)) cache;
} Font_Manager;

Font font_manager_get_font(Font_Manager* fm, uint32_t font_face, uint32_t size) {
    if (font_face >= fm->cache.count) {
        uint32 old_count = fm->cache.count;
        oc_array_reserve(&fm->arena, fm->cache, font_face + 1);
        memset(&fm->cache.items[old_count], 0, sizeof(fm->cache.items[0]) * (font_face + 1 - old_count));
    }

    hash_map_get(&fm->arena, &fm->cache, size)
    if ()
}

Font_Id font_manager_get_font_id(Font_Manager* fm, uint32_t font_face, uint32_t size) {

}

