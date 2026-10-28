#include "game.h"

/* The native build selects this renderer without changing the retail body. */
void DrawFontGlyph(char character, TextContext *context, int height,
                   int width, int y)
{
    Viewport *viewport;
    const unsigned char *source;
    unsigned char *destination;
    unsigned char fontColour;
    unsigned char fontBackground;
    unsigned char pixel;
    int translate;
    int firstColumn;
    int lastColumn;
    int row;
    int column;

    viewport = context->viewport;
    fontColour = context->font[2];
    fontBackground = context->font[3];
    translate = fontColour != context->colour ||
                fontBackground != context->backgroundColour;
    source = context->font + context->font[0x104 + (unsigned char)character] +
             (context->font[0x204 + (unsigned char)character] << 8);
    firstColumn = viewport->left - context->cursorX;
    if (firstColumn < 0)
        firstColumn = 0;
    lastColumn = viewport->right - context->cursorX + 1;
    if (lastColumn > width)
        lastColumn = width;
    g_abPaletteTranslation_00496338[fontColour] = context->colour;
    g_abPaletteTranslation_00496338[fontBackground] = context->backgroundColour;
    for (row = 0; row < height; row++) {
        if (y + row < 0 || y + row < viewport->top ||
            y + row > viewport->bottom || firstColumn >= lastColumn)
            continue;
        /* Clip before reading the row table or forming a destination. Keep
         * the rasteriser's special first-row origin convention. */
        if (y + row == viewport->top &&
            (viewport->rowOffsets[y + row] & 0x8000) != 0)
            destination = viewport->pixels + (context->cursorX + firstColumn);
        else
            destination = viewport->pixels + viewport->rowOffsets[y + row] +
                          (context->cursorX + firstColumn);
        for (column = firstColumn; column < lastColumn; column++) {
            pixel = source[row * width + column];
            if (translate)
                pixel = g_abPaletteTranslation_00496338[pixel];
            if (pixel != 0xff)
                destination[column - firstColumn] = pixel;
        }
    }
    context->cursorX += context->font[4 + (unsigned char)character];
    g_abPaletteTranslation_00496338[fontColour] = fontColour;
    g_abPaletteTranslation_00496338[fontBackground] = fontBackground;
}
