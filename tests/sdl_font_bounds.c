#include "game.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "Font bounds check failed at line %d\n", __LINE__); \
        return 1; \
    } \
} while (0)

static int CheckSubtitleBounds(void)
{
    unsigned short fontStorage[0x1a0];
    unsigned char *font = (unsigned char *)fontStorage;
    unsigned char storage[32 + 288 * 50 + 32];
    unsigned char *pixels = storage + 32;
    unsigned short rows[50];
    Viewport viewport = {0};
    TextContext context = {0};
    unsigned int i;

    memset(fontStorage, 0, sizeof(fontStorage));
    font[0] = 11;
    font[2] = 7;
    font[3] = 0xff;
    memset(font + 0x304, 7, 11);
    for (i = 0; i < 256; i++) {
        font[4 + i] = 1;
        font[0x104 + i] = 4;
        font[0x204 + i] = 3;
        g_abPaletteTranslation_00496338[i] = (unsigned char)i;
    }
    /* Extra mapped rows let the pre-fix renderer fail by canary corruption
     * deterministically, even in a build without sanitizers. */
    for (i = 0; i < 50; i++)
        rows[i] = (unsigned short)(288 * i);
    viewport.pixels = pixels;
    viewport.rowOffsets = rows;
    viewport.right = 287;
    viewport.bottom = 36;
    context.viewport = &viewport;
    context.font = font;
    context.colour = 7;
    context.backgroundColour = 0xff;
    SetTextContext(&context);

    memset(storage, 0x5a, sizeof(storage));
    DrawTextString("A\nB\nC\nD");
    for (i = 0; i < 32; i++)
        CHECK(storage[i] == 0x5a);
    for (i = 288 * 37; i < sizeof(storage) - 32; i++)
        CHECK(pixels[i] == 0x5a);
    CHECK(context.cursorX == 1 && context.cursorY == 33);

    memset(storage, 0x5a, sizeof(storage));
    viewport.bottom = 49;
    context.cursorX = context.cursorY = 0;
    DrawTextString("A\nB\nC\nD");
    for (i = 0; i < 44; i++)
        CHECK(pixels[i * 288] == 7);
    for (i = 288 * 50; i < sizeof(storage) - 32; i++)
        CHECK(pixels[i] == 0x5a);

    /* Exercise the same allocation entry point used by opcode 0x7f. */
    g_nSpacePaletteFadeMode_004901e8 = 0x13;
    g_stCutsceneTextViewport_005d2d90.top = 150;
    g_stCutsceneTextViewport_005d2d90.bottom = 199;
    g_stCutsceneTextBackingViewport_005d2db0.right = 287;
    g_stCutsceneTextBackingViewport_005d2db0.bottom = 36;
    AllocateCutsceneViewportOrFatal(
        &g_stCutsceneTextBackingViewport_005d2db0, -1, 0);
    CHECK(g_stCutsceneTextBackingViewport_005d2db0.bottom == 49);
    context.viewport = &g_stCutsceneTextBackingViewport_005d2db0;
    context.cursorX = context.cursorY = 0;
    DrawTextString("A\nB\nC\nD");
    CHECK(context.viewport->pixels[43 * 288] == 7);
    free_viewport(context.viewport);
    context.viewport->pixels = 0;
    SetTextContext(0);
    return 0;
}

static int CheckClippedGlyphs(void)
{
    unsigned short fontStorage[0x190] = {0};
    unsigned char *font = (unsigned char *)fontStorage;
    unsigned char storage[32];
    unsigned char *pixels = storage + 8;
    unsigned short rows[3] = {0, 4, 8};
    Viewport viewport = {0};
    TextContext context = {0};
    int x;
    int y;
    int row;
    int column;
    int i;

    font[0] = 3;
    font[2] = 7;
    font[3] = 0xff;
    font[4 + 'A'] = 3;
    font[0x104 + 'A'] = 4;
    font[0x204 + 'A'] = 3;
    memset(font + 0x304, 7, 9);
    viewport.pixels = pixels;
    viewport.rowOffsets = rows;
    viewport.left = 1;
    viewport.top = 1;
    viewport.right = 2;
    viewport.bottom = 2;
    context.viewport = &viewport;
    context.font = font;
    context.backgroundColour = 0xff;
    /* Every edge and fully invisible glyphs, with and without translation. */
    for (context.colour = 7; context.colour <= 8; context.colour++) {
        for (y = -3; y <= 4; y++) {
            for (x = -3; x <= 4; x++) {
                memset(storage, 0x5a, sizeof(storage));
                context.cursorX = (short)x;
                DrawFontGlyph('A', &context, 3, 3, y);
                CHECK(context.cursorX == x + 3);
                for (i = 0; i < sizeof(storage); i++) {
                    row = (i - 8) / 4;
                    column = (i - 8) % 4;
                    if (i >= 8 && row >= 1 && row <= 2 &&
                        column >= 1 && column <= 2 &&
                        row >= y && row < y + 3 &&
                        column >= x && column < x + 3)
                        CHECK(storage[i] == context.colour);
                    else
                        CHECK(storage[i] == 0x5a);
                }
                CHECK(g_abPaletteTranslation_00496338[7] == 7);
            }
        }
    }
    /* Invisible rows must never touch the row-offset table. */
    viewport.rowOffsets = 0;
    context.cursorX = 1;
    DrawFontGlyph('A', &context, 3, 3, -4);
    context.cursorX = 1;
    DrawFontGlyph('A', &context, 3, 3, 3);
    context.cursorX = -4;
    DrawFontGlyph('A', &context, 3, 3, 1);

    /* Preserve the special first-row origin and transparent/background ink. */
    viewport.left = viewport.top = viewport.bottom = 0;
    rows[0] = 0x8000;
    viewport.rowOffsets = rows;
    context.cursorX = 0;
    context.colour = 7;
    font[0x305] = 0xff;
    font[0x306] = 6;
    memset(storage, 0x5a, sizeof(storage));
    DrawFontGlyph('A', &context, 1, 3, 0);
    CHECK(pixels[0] == 7 && pixels[1] == 0x5a && pixels[2] == 6);
    context.cursorX = 0;
    context.colour = 9;
    DrawFontGlyph('A', &context, 1, 3, 0);
    CHECK(pixels[0] == 9 && pixels[1] == 0x5a && pixels[2] == 6);
    CHECK(context.cursorX == 3);
    context.cursorX = 0;
    context.backgroundColour = 5;
    DrawFontGlyph('A', &context, 1, 3, 0);
    CHECK(pixels[1] == 5);
    CHECK(g_abPaletteTranslation_00496338[7] == 7 &&
          g_abPaletteTranslation_00496338[0xff] == 0xff);
    context.cursorX = 0;
    pixels[2] = 0x5a;
    DrawFontGlyph('A', &context, 1, 2, 0);
    CHECK(context.cursorX == 3 && pixels[2] == 0x5a);
    return 0;
}

int main(void)
{
    return CheckSubtitleBounds() || CheckClippedGlyphs();
}
