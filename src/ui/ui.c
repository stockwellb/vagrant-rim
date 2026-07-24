#include "ui/ui.h"

#include "raygui.h"

Font ui_font(void)
{
    return GuiGetFont();
}

float ui_spacing(void)
{
    return (float)GuiGetStyle(DEFAULT, TEXT_SPACING);
}

Color ui_style_color(int property)
{
    return GetColor(GuiGetStyle(DEFAULT, property));
}

void ui_text_centered(const char *text, float left, float width, float y, float size,
                      Color color)
{
    Font font = ui_font();
    float spacing = ui_spacing();
    Vector2 m = MeasureTextEx(font, text, size, spacing);
    DrawTextEx(font, text, (Vector2){ left + (width - m.x) / 2.0f, y }, size, spacing, color);
}
