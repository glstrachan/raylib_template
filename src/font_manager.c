#include "font_manager.h"

Font font_manager_get_font_from_id(Font_Manager* fm, Font_Id font_id) {
    return font_manager_get_font(fm, font_id >> 32, font_id & 0xFFFFFFFF);
}

Font font_manager_get_font(Font_Manager* fm, uint32_t font_face, uint32_t size) {
    if (font_face >= fm->cache.count) {
        uint32 old_count = fm->cache.count;
        oc_array_reserve(fm->arena, &fm->cache, font_face + 1);
        memset(&fm->cache.items[old_count], 0, sizeof(fm->cache.items[0]) * (font_face + 1 - old_count));
    }

    Font* found_font = hash_map_get(&fm->arena, &fm->cache.items[font_face], size);
    if (found_font) {
        return *found_font;
    }

    Font font = LoadFontEx(fm->font_paths[font_face], size, NULL, 0);
    hash_map_put(fm->arena, &fm->cache.items[font_face], size, font);
    return font;
}

Font_Id font_manager_get_font_id(Font_Manager* fm, uint32_t font_face, uint32_t size) {
    (void)font_manager_get_font(fm, font_face, size);
    return FONT(font_face, size);
}
