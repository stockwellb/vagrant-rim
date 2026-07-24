#include "screen/pause_menu.h"

#include "raygui.h"
#include "raylib.h"

PauseMenuAction pause_menu_draw(int screen_width, int screen_height,
                                const PauseMenuConfig *menu, const ButtonConfig *button,
                                bool recently_saved) {
  // Dim the frozen play scene behind the menu.
  DrawRectangle(0, 0, screen_width, screen_height,
                (Color){ 0, 0, 0, (unsigned char)menu->scrim_alpha });

  Font font = GuiGetFont();
  Color text_color = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
  float spacing = (float)GuiGetStyle(DEFAULT, TEXT_SPACING);

  // --- Title -----------------------------------------------------------
  Vector2 tsize = MeasureTextEx(font, menu->title_text, (float)menu->title_size, spacing);
  DrawTextEx(font, menu->title_text,
             (Vector2){ (screen_width - tsize.x) / 2.0f, screen_height / 4.0f },
             (float)menu->title_size, spacing, text_color);

  // --- Buttons ---------------------------------------------------------
  const float bw = (float)button->width;
  const float bh = (float)button->height;
  const float gap = (float)button->gap;
  float x = (screen_width - bw) / 2.0f;
  float y = screen_height / 2.0f;

  PauseMenuAction action = PAUSE_NONE;

  if (GuiButton((Rectangle){ x, y, bw, bh }, menu->resume_text)) {
    action = PAUSE_RESUME;
  }

  y += bh + gap;
  if (GuiButton((Rectangle){ x, y, bw, bh }, menu->save_text)) {
    action = PAUSE_SAVE;
  }
  if (recently_saved) {
    DrawTextEx(font, menu->saved_notice, (Vector2){ x + bw + 16.0f, y + (bh - 20.0f) / 2.0f },
               20.0f, spacing, text_color);
  }

  y += bh + gap;
  if (GuiButton((Rectangle){ x, y, bw, bh }, menu->quit_text)) {
    action = PAUSE_QUIT;
  }

  return action;
}
