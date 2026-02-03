#pragma once

#include "clay.h"
#include "../raylib/raylib/include/raylib.h"
#include "raymath.h"
#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

typedef enum
{
    CUSTOM_LAYOUT_ELEMENT_TYPE_3D_MODEL,
    CUSTOM_LAYOUT_ELEMENT_TYPE_BACKGROUND
} CustomLayoutElementType;

typedef struct
{
    Model model;
    float scale;
    Vector3 position;
    Matrix rotation;
} CustomLayoutElement_3DModel;

typedef struct
{
    Shader shader;
    Color color1, color2;
} CustomLayoutElement_Background;

typedef struct
{
    CustomLayoutElementType type;
    union {
        CustomLayoutElement_3DModel model;
        CustomLayoutElement_Background background;
    } customData;
} CustomLayoutElement;

typedef struct
{
    uint32_t visible_chars;
} DialogTextUserData;

void Clay_Raylib_Render(Clay_RenderCommandArray renderCommands, void* fonts);
Clay_Dimensions Raylib_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData);
