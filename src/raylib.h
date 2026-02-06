#pragma once
#ifndef DONT_INCLUDE_RAYLIB_STUB
#include "../external/raylib/src/raylib.h"
#include "../external/clay/clay.h"

static inline void ClayDrawRectangleV(Vector2 point, Vector2 size, Color color) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_RECTANGLE,
        .color = color,
        .point = point,
        .size = size,
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawRectangle(int posX, int posY, int width, int height, Color color) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_RECTANGLE,
        .color = color,
        .point = { posX, posY },
        .size = { width, height },
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawRectangleRec(Rectangle rec, Color color) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_RECTANGLE,
        .color = color,
        .point = { rec.x, rec.y },
        .size = { rec.width, rec.height },
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawRectangleLinesEx(Rectangle rec, float thickness, Color color) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_RECTANGLE_LINES,
        .color = color,
        .point = { rec.x, rec.y },
        .size = { rec.width, rec.height },
        .thickness = thickness,
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawCircleV(Vector2 point, float radius, Color color) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_CIRCLE,
        .color = color,
        .point = point,
        .radius = radius,
    };
    Clay__Raylib(&config);
}


static inline void ClayDrawCircle(int posX, int posY, float radius, Color color) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_CIRCLE,
        .color = color,
        .point = { posX, posY },
        .radius = radius,
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawCircleLinesV(Vector2 point, float radius, Color color) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_CIRCLE_LINES,
        .color = color,
        .point = { point.x, point.y },
        .radius = radius,
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_TRIANGLE,
        .color = color,
        .v1 = v1, .v2 = v2, .v3 = v3,
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawTriangleLines(Vector2 v1, Vector2 v2, Vector2 v3, Color color) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_TRIANGLE_LINES,
        .color = color,
        .v1 = v1, .v2 = v2, .v3 = v3,
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_TEXT,
        .font = font,
        .color = tint,
        .text = text,
        .point = position,
        .fontSize = fontSize,
        .spacing = spacing,
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawTextureV(Texture2D texture, Vector2 point, Color tint) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_TEXTURE,
        .color = tint,
        .texture = texture,
        .point = point,
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawTexture(Texture2D texture, int posX, int posY, Color tint) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_TEXTURE,
        .color = tint,
        .texture = texture,
        .point = { posX, posY },
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawTexturePro(Texture2D texture, Rectangle dst, Rectangle src, Vector2 origin, float rotation, Color tint) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_TEXTURE_PRO,
        .color = tint,
        .texture = texture,
        .dst = dst,
        .src = src,
        .point = origin,
        .rotation = rotation,
    };
    Clay__Raylib(&config);
}

static inline void ClayBeginShaderMode(Shader shader) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_BEGIN_SHADER_MODE,
        .shader = shader,
    };
    Clay__Raylib(&config);
}

static inline void ClayEndShaderMode() {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_END_SHADER_MODE,
    };
    Clay__Raylib(&config);
}

static inline void ClaySetShaderValue(Shader shader, int locIndex, const void *value, int uniformType) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_SET_SHADER_VALUE,
        .shader = shader,
        .shaderLocIdx = locIndex,
        .shaderValue = value,
        .uniformType = uniformType,
    };
    Clay__Raylib(&config);
}

static inline void ClaySetShapesTexture(Texture2D texture, Rectangle rect) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_SET_SHAPES_TEXTURE,
        .point = { rect.x, rect.y },
        .size = { rect.width, rect.height },
        .texture = texture,
    };
    Clay__Raylib(&config);
}

static inline void ClayDrawSplineCatmullRom(const Vector2 *points, int pointCount, float thick, Color color) {
    Clay_RaylibElementConfig config = {
        .fn = CLAY_RAYLIB_FUNCTION_DRAW_SPLINE_CATMULL_ROM,
        .points = points,
        .pointsCount = pointCount,
        .thickness = thick,
        .color = color,
    };
    Clay__Raylib(&config);
}

#define DrawRectangleV(...) ClayDrawRectangleV(__VA_ARGS__)
#define DrawRectangle(...) ClayDrawRectangle(__VA_ARGS__)
#define DrawRectangleRec(...) ClayDrawRectangleRec(__VA_ARGS__)
#define DrawRectangleLinesEx(...) ClayDrawRectangleLinesEx(__VA_ARGS__)
#define DrawCircleV(...) ClayDrawCircleV(__VA_ARGS__)
#define DrawCircle(...) ClayDrawCircle(__VA_ARGS__)
#define DrawCircleLinesV(...) ClayDrawCircleLinesV(__VA_ARGS__)
#define DrawTriangle(...) ClayDrawTriangle(__VA_ARGS__)
#define DrawTriangleLines(...) ClayDrawTriangleLines(__VA_ARGS__)
#define DrawTextureV(...) ClayDrawTextureV(__VA_ARGS__)
#define DrawTextEx(...) ClayDrawTextEx(__VA_ARGS__)
#define DrawTexture(...) ClayDrawTexture(__VA_ARGS__)
#define DrawTexturePro(...) ClayDrawTexturePro(__VA_ARGS__)
#define BeginShaderMode(...) ClayBeginShaderMode(__VA_ARGS__)
#define EndShaderMode(...) ClayEndShaderMode(__VA_ARGS__)
#define SetShaderValue(...) ClaySetShaderValue(__VA_ARGS__)
#define SetShapesTexture(...) ClaySetShapesTexture(__VA_ARGS__)
#define DrawSplineCatmullRom(...) ClayDrawSplineCatmullRom(__VA_ARGS__)

#endif

// static inline void DrawRectangleV(Vector2 position, Vector2 size, Color color)