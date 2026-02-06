#define DONT_INCLUDE_RAYLIB_STUB
#include "./clay_renderer_raylib.h"
#include "../../src/font_manager.h"

#define CLAY_RECTANGLE_TO_RAYLIB_RECTANGLE(rectangle) (Rectangle) { .x = rectangle.x, .y = rectangle.y, .width = rectangle.width, .height = rectangle.height }
#define CLAY_COLOR_TO_RAYLIB_COLOR(color) (Color) { .r = (unsigned char)roundf(color.r), .g = (unsigned char)roundf(color.g), .b = (unsigned char)roundf(color.b), .a = (unsigned char)roundf(color.a) }

Camera Raylib_camera;

// Get a ray trace from the screen position (i.e mouse) within a specific section of the screen
Ray GetScreenToWorldPointWithZDistance(Vector2 position, Camera camera, int screenWidth, int screenHeight, float zDistance)
{
    Ray ray = { 0 };

    // Calculate normalized device coordinates
    // NOTE: y value is negative
    float x = (2.0f*position.x)/(float)screenWidth - 1.0f;
    float y = 1.0f - (2.0f*position.y)/(float)screenHeight;
    float z = 1.0f;

    // Store values in a vector
    Vector3 deviceCoords = { x, y, z };

    // Calculate view matrix from camera look at
    Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);

    Matrix matProj = MatrixIdentity();

    if (camera.projection == CAMERA_PERSPECTIVE)
    {
        // Calculate projection matrix from perspective
        matProj = MatrixPerspective(camera.fovy*DEG2RAD, ((double)screenWidth/(double)screenHeight), 0.01f, zDistance);
    }
    else if (camera.projection == CAMERA_ORTHOGRAPHIC)
    {
        double aspect = (double)screenWidth/(double)screenHeight;
        double top = camera.fovy/2.0;
        double right = top*aspect;

        // Calculate projection matrix from orthographic
        matProj = MatrixOrtho(-right, right, -top, top, 0.01, 1000.0);
    }

    // Unproject far/near points
    Vector3 nearPoint = Vector3Unproject((Vector3){ deviceCoords.x, deviceCoords.y, 0.0f }, matProj, matView);
    Vector3 farPoint = Vector3Unproject((Vector3){ deviceCoords.x, deviceCoords.y, 1.0f }, matProj, matView);

    // Calculate normalized direction vector
    Vector3 direction = Vector3Normalize(Vector3Subtract(farPoint, nearPoint));

    ray.position = farPoint;

    // Apply calculated vectors to ray
    ray.direction = direction;

    return ray;
}


Clay_Dimensions Raylib_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    // Measure string size for Font
    Clay_Dimensions textSize = { 0 };

    float maxTextWidth = 0.0f;
    float lineTextWidth = 0;
    int maxLineCharCount = 0;
    int lineCharCount = 0;

    float textHeight = config->fontSize;

    Font_Manager* fm = userData;
    Font fontToUse = font_manager_get_font(fm, config->fontId, config->fontSize);

    // Font failed to load, likely the fonts are in the wrong place relative to the execution dir.
    // RayLib ships with a default font, so we can continue with that built in one. 
    if (!fontToUse.glyphs) {
        fontToUse = GetFontDefault();
    }

    float scaleFactor = config->fontSize/(float)fontToUse.baseSize;

    for (int i = 0; i < text.length; ++i, lineCharCount++)
    {
        if (text.chars[i] == '\n') {
            maxTextWidth = fmax(maxTextWidth, lineTextWidth);
            maxLineCharCount = CLAY__MAX(maxLineCharCount, lineCharCount);
            lineTextWidth = 0;
            lineCharCount = 0;
            continue;
        }
        int index = text.chars[i] - 32;
        if (fontToUse.glyphs[index].advanceX != 0) lineTextWidth += fontToUse.glyphs[index].advanceX;
        else lineTextWidth += (fontToUse.recs[index].width + fontToUse.glyphs[index].offsetX);
    }

    maxTextWidth = fmax(maxTextWidth, lineTextWidth);
    maxLineCharCount = CLAY__MAX(maxLineCharCount, lineCharCount);

    textSize.width = maxTextWidth * scaleFactor + (lineCharCount * config->letterSpacing);
    textSize.height = textHeight;

    return textSize;
}

void Clay_Raylib_Initialize(int width, int height, const char *title, unsigned int flags) {
    SetConfigFlags(flags);
    InitWindow(width, height, title);
//    EnableEventWaiting();
}

// A MALLOC'd buffer, that we keep modifying inorder to save from so many Malloc and Free Calls.
// Call Clay_Raylib_Close() to free
static char *temp_render_buffer = NULL;
static int temp_render_buffer_len = 0;

// Call after closing the window to clean up the render buffer
void Clay_Raylib_Close()
{
    if(temp_render_buffer) free(temp_render_buffer);
    temp_render_buffer_len = 0;

    CloseWindow();
}


void Clay_Raylib_Render(Clay_RenderCommandArray renderCommands, void* fonts)
{
    for (int j = 0; j < renderCommands.length; j++)
    {
        Clay_RenderCommand *renderCommand = Clay_RenderCommandArray_Get(&renderCommands, j);
        Clay_BoundingBox boundingBox = {roundf(renderCommand->boundingBox.x), roundf(renderCommand->boundingBox.y), roundf(renderCommand->boundingBox.width), roundf(renderCommand->boundingBox.height)};
        switch (renderCommand->commandType)
        {
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_TextRenderData *textData = &renderCommand->renderData.text;

                Font fontToUse = font_manager_get_font(fonts, textData->fontId, textData->fontSize);
                
                DialogTextUserData* userData = (DialogTextUserData*)renderCommand->userData;
                uint32_t visibleChars = userData ? userData->visible_chars : UINT32_MAX;

                uint32_t offset = textData->stringContents.chars - textData->stringContents.baseChars;

                float x = boundingBox.x;
                float y = boundingBox.y;
                float scaleFactor = textData->fontSize / (float)fontToUse.baseSize;
                
                for (int i = 0; i < textData->stringContents.length; i++) {
                    char c = textData->stringContents.chars[i];
                    
                    if (c == '\n') {
                        x = boundingBox.x;
                        y += textData->fontSize;
                        continue;
                    }
                    
                    uint32_t actualPosition = offset + i;
                    
                    Color color = CLAY_COLOR_TO_RAYLIB_COLOR(textData->textColor);
                    if (actualPosition >= visibleChars) {
                        color.a = 0;  // Transparent
                    }
                    
                    DrawTextCodepoint(fontToUse, c, (Vector2){x, y}, textData->fontSize, color);
                    
                    int index = c - 32;
                    if (fontToUse.glyphs[index].advanceX != 0) 
                        x += fontToUse.glyphs[index].advanceX * scaleFactor;
                    else 
                        x += (fontToUse.recs[index].width + fontToUse.glyphs[index].offsetX) * scaleFactor;
                    
                    x += textData->letterSpacing;
                }

                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
                Texture2D imageTexture = *(Texture2D *)renderCommand->renderData.image.imageData;
                Clay_Color tintColor = renderCommand->renderData.image.backgroundColor;
                if (tintColor.r == 0 && tintColor.g == 0 && tintColor.b == 0 && tintColor.a == 0) {
                    tintColor = (Clay_Color) { 255, 255, 255, 255 };
                }
                DrawTexturePro(
                    imageTexture,
                    (Rectangle) { 0, 0, imageTexture.width, imageTexture.height },
                    (Rectangle){boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height},
                    (Vector2) {},
                    0,
                    CLAY_COLOR_TO_RAYLIB_COLOR(tintColor));
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                BeginScissorMode((int)roundf(boundingBox.x), (int)roundf(boundingBox.y), (int)roundf(boundingBox.width), (int)roundf(boundingBox.height));
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                EndScissorMode();
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleRenderData *config = &renderCommand->renderData.rectangle;
                if (config->cornerRadius.topLeft > boundingBox.width || config->cornerRadius.topLeft > boundingBox.height) {
                    float radius = min(boundingBox.width, boundingBox.height) / 2.0f;
                    DrawRectangleRounded((Rectangle) { boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height }, radius, 8, CLAY_COLOR_TO_RAYLIB_COLOR(config->backgroundColor));
                } else if (config->cornerRadius.topLeft > 0) {
                    float radius = (config->cornerRadius.topLeft * 2) / (float)((boundingBox.width > boundingBox.height) ? boundingBox.height : boundingBox.width);
                    DrawRectangleRounded((Rectangle) { boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height }, radius, 8, CLAY_COLOR_TO_RAYLIB_COLOR(config->backgroundColor));
                } else {
                    DrawRectangle(boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height, CLAY_COLOR_TO_RAYLIB_COLOR(config->backgroundColor));
                }
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                Clay_BorderRenderData *config = &renderCommand->renderData.border;
                float topRight, topLeft, bottomRight, bottomLeft;

                if (config->cornerRadius.topLeft > boundingBox.width || config->cornerRadius.topLeft > boundingBox.height) {
                    topLeft = min(boundingBox.width, boundingBox.height) / 2.0f;
                } else {
                    topLeft = config->cornerRadius.topLeft;
                }

                if (config->cornerRadius.topRight > boundingBox.width || config->cornerRadius.topRight > boundingBox.height) {
                    topRight = min(boundingBox.width, boundingBox.height) / 2.0f;
                } else {
                    topRight = config->cornerRadius.topRight;
                }

                if (config->cornerRadius.bottomLeft > boundingBox.width || config->cornerRadius.bottomLeft > boundingBox.height) {
                    bottomLeft = min(boundingBox.width, boundingBox.height) / 2.0f;
                } else {
                    bottomLeft = config->cornerRadius.bottomLeft;
                }

                if (config->cornerRadius.bottomRight > boundingBox.width || config->cornerRadius.bottomRight > boundingBox.height) {
                    bottomRight = min(boundingBox.width, boundingBox.height) / 2.0f;
                } else {
                    bottomRight = config->cornerRadius.bottomRight;
                }

                // Left border
                if (config->width.left > 0) {
                    DrawRectangle((int)roundf(boundingBox.x), (int)roundf(boundingBox.y + topLeft), (int)config->width.left, (int)roundf(boundingBox.height - topLeft - bottomLeft), CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
                }
                // Right border
                if (config->width.right > 0) {
                    DrawRectangle((int)roundf(boundingBox.x + boundingBox.width - config->width.right), (int)roundf(boundingBox.y + topRight), (int)config->width.right, (int)roundf(boundingBox.height - topRight - bottomRight), CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
                }
                // Top border
                if (config->width.top > 0) {
                    DrawRectangle((int)roundf(boundingBox.x + topLeft), (int)roundf(boundingBox.y), (int)roundf(boundingBox.width - topLeft - topRight), (int)config->width.top, CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
                }
                // Bottom border
                if (config->width.bottom > 0) {
                    DrawRectangle((int)roundf(boundingBox.x + bottomLeft), (int)roundf(boundingBox.y + boundingBox.height - config->width.bottom), (int)roundf(boundingBox.width - bottomLeft - bottomRight), (int)config->width.bottom, CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
                }

                int segments = config->width.top * 3.0;

                if (config->cornerRadius.topLeft > 0) {
                    DrawRing((Vector2) { roundf(boundingBox.x + topLeft), roundf(boundingBox.y + topLeft) }, roundf(topLeft - config->width.top), topLeft, 180, 270, segments, CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
                }
                if (config->cornerRadius.topRight > 0) {
                    DrawRing((Vector2) { roundf(boundingBox.x + boundingBox.width - topRight), roundf(boundingBox.y + topRight) }, roundf(topRight - config->width.top), topRight, 270, 360, segments, CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
                }
                if (config->cornerRadius.bottomLeft > 0) {
                    DrawRing((Vector2) { roundf(boundingBox.x + bottomLeft), roundf(boundingBox.y + boundingBox.height - bottomLeft) }, roundf(bottomLeft - config->width.bottom), bottomLeft, 90, 180, segments, CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
                }
                if (config->cornerRadius.bottomRight > 0) {
                    DrawRing((Vector2) { roundf(boundingBox.x + boundingBox.width - bottomRight), roundf(boundingBox.y + boundingBox.height - bottomRight) }, roundf(bottomRight - config->width.bottom), bottomRight, 0.1, 90, segments, CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
                }
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
                Clay_CustomRenderData *config = &renderCommand->renderData.custom;
                CustomLayoutElement *customElement = (CustomLayoutElement *)config->customData;
                if (!customElement) continue;
                
                switch (customElement->type) {
                    case CUSTOM_LAYOUT_ELEMENT_TYPE_3D_MODEL: {
                        Clay_BoundingBox rootBox = renderCommands.internalArray[0].boundingBox;
                        float scaleValue = CLAY__MIN(CLAY__MIN(1, 768 / rootBox.height) * CLAY__MAX(1, rootBox.width / 1024), 1.5f);
                        Ray positionRay = GetScreenToWorldPointWithZDistance((Vector2) { renderCommand->boundingBox.x + renderCommand->boundingBox.width / 2, renderCommand->boundingBox.y + (renderCommand->boundingBox.height / 2) + 20 }, Raylib_camera, (int)roundf(rootBox.width), (int)roundf(rootBox.height), 140);
                        BeginMode3D(Raylib_camera);
                            DrawModel(customElement->customData.model.model, positionRay.position, customElement->customData.model.scale * scaleValue, WHITE);        // Draw 3d model with texture
                        EndMode3D();
                        break;
                    }
                    case CUSTOM_LAYOUT_ELEMENT_TYPE_BACKGROUND: {
                        CustomLayoutElement_Background *data = &customElement->customData.background;
                        
                        float time = GetTime();
                        SetShaderValue(data->shader, GetShaderLocation(data->shader, "time"), &time, SHADER_UNIFORM_FLOAT);

                        Vector2 resolution = {GetScreenWidth(), GetScreenHeight()};
                        SetShaderValue(data->shader, GetShaderLocation(data->shader, "screenResolution"), &resolution, SHADER_UNIFORM_VEC2);
                        Vector2 elementResolution = {boundingBox.width, boundingBox.height};
                        SetShaderValue(data->shader, GetShaderLocation(data->shader, "elementResolution"), &elementResolution, SHADER_UNIFORM_VEC2);
                        Vector2 elementPosition = {boundingBox.x, boundingBox.y};
                        SetShaderValue(data->shader, GetShaderLocation(data->shader, "elementPosition"), &elementPosition, SHADER_UNIFORM_VEC2);

                        float color1[] = {data->color1.r / 255.0f, data->color1.g / 255.0f, data->color1.b / 255.0f, data->color1.a / 255.0f};
                        SetShaderValue(data->shader, GetShaderLocation(data->shader, "color1"), color1, SHADER_UNIFORM_VEC4);
                        float color2[] = {data->color2.r / 255.0f, data->color2.g / 255.0f, data->color2.b / 255.0f, data->color2.a / 255.0f};
                        SetShaderValue(data->shader, GetShaderLocation(data->shader, "color2"), color2, SHADER_UNIFORM_VEC4);

                        BeginShaderMode(data->shader);

                        if (config->cornerRadius.topLeft > 0) {
                            float radius = (config->cornerRadius.topLeft * 2) / (float)((boundingBox.width > boundingBox.height) ? boundingBox.height : boundingBox.width);
                            DrawRectangleRounded((Rectangle) { boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height }, radius, 8, WHITE);
                        } else {
                            DrawRectangle(boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height, WHITE);
                        }
                        
                        EndShaderMode();

                        break;
                    }
                    default: break;
                }
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_RAYLIB: {
                Clay_RaylibElementConfig *config = &renderCommand->renderData.raylib;
                switch (config->fn) {
                case CLAY_RAYLIB_FUNCTION_DRAW_RECTANGLE: {
                    DrawRectangleV(config->point, config->size, config->color);
                } break;
                case CLAY_RAYLIB_FUNCTION_DRAW_RECTANGLE_LINES: {
                    Rectangle rect = {
                        config->point.x, config->point.y,
                        config->size.x, config->size.y,
                    };
                    DrawRectangleLinesEx(rect, config->thickness, config->color);
                } break;
                case CLAY_RAYLIB_FUNCTION_DRAW_CIRCLE: {
                    DrawCircleV(config->point, config->radius, config->color);
                } break;
                case CLAY_RAYLIB_FUNCTION_DRAW_CIRCLE_LINES: {
                    DrawCircleLinesV(config->point, config->radius, config->color);
                } break;
                case CLAY_RAYLIB_FUNCTION_DRAW_TRIANGLE: {
                    DrawTriangle(config->v1, config->v2, config->v3, config->color);
                } break;
                case CLAY_RAYLIB_FUNCTION_DRAW_TRIANGLE_LINES: {
                    DrawTriangleLines(config->v1, config->v2, config->v3, config->color);
                } break;
                case CLAY_RAYLIB_FUNCTION_DRAW_TEXT: {
                    DrawTextEx(config->font, config->text, config->point, config->fontSize, config->spacing, config->color);
                } break;
                case CLAY_RAYLIB_FUNCTION_DRAW_TEXTURE: {
                    DrawTextureV(config->texture, config->point, config->color);
                } break;
                case CLAY_RAYLIB_FUNCTION_DRAW_TEXTURE_PRO: {
                    DrawTexturePro(config->texture, config->dst, config->src, config->point, config->rotation, config->color);
                } break;
                case CLAY_RAYLIB_FUNCTION_BEGIN_SHADER_MODE: {
                    BeginShaderMode(config->shader);
                } break;
                case CLAY_RAYLIB_FUNCTION_END_SHADER_MODE: {
                    EndShaderMode();
                } break;
                case CLAY_RAYLIB_FUNCTION_SET_SHADER_VALUE: {
                    SetShaderValue(config->shader, config->shaderLocIdx, config->shaderValue, config->uniformType);
                } break;
                case CLAY_RAYLIB_FUNCTION_SET_SHAPES_TEXTURE: {
                    Rectangle rect = {
                        config->point.x, config->point.y,
                        config->size.x, config->size.y,
                    };
                    SetShapesTexture(config->texture, rect);
                } break;
                case CLAY_RAYLIB_FUNCTION_DRAW_SPLINE_CATMULL_ROM: {
                    // Rectangle rect = {
                    //     config->point.x, config->point.y,
                    //     config->size.x, config->size.y,
                    // };
                    // SetShapesTexture(config->texture, rect);
                    DrawSplineCatmullRom(config->points, config->pointsCount, config->thickness, config->color);
                } break;
                default: {
                    printf("Error: unhandled render command.");
                    exit(1);
                } break;
                }
            } break;
            case CLAY_RENDER_COMMAND_TYPE_NONE: break;
            default: {
                printf("Error: unhandled render command.");
                exit(1);
            }
        }
    }
}
