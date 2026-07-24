#ifndef VR_UI_UI_H
#define VR_UI_UI_H

#include "raylib.h"

// Small shared helpers over the active raygui theme, so every screen draws text
// from one source of truth (the loaded UI font + the .rgs style) instead of
// repeating the same GuiGetFont/GuiGetStyle boilerplate.

// The font raygui is currently drawing with (our loaded UI font, or the built-in).
Font ui_font(void);

// The style's global text spacing, as a float (for MeasureTextEx/DrawTextEx).
float ui_spacing(void);

// GetColor(GuiGetStyle(DEFAULT, property)) — e.g. ui_style_color(TEXT_COLOR_NORMAL).
Color ui_style_color(int property);

// Draw `text` at font `size`, horizontally centered within [left, left + width)
// at vertical position `y`, using the active UI font and spacing.
void ui_text_centered(const char *text, float left, float width, float y, float size,
                      Color color);

#endif // VR_UI_UI_H
