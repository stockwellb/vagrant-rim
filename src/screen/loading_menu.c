#include "screen/loading_menu.h"

#include "raygui.h"
#include "raylib.h"

LoadingMenuAction loading_menu_draw(int screen_width, int screen_height, bool has_save,
                                    const LoadingMenuConfig *menu,
                                    const ButtonConfig *button) {
  // Draw title/tagline with the same font raygui uses (our loaded UI font) so
  // the whole screen is consistent. Title uses the style's normal text color;
  // the tagline is that same color dimmed 30% — readable, but clearly secondary.
  // (The style's DISABLED color was too dim to read on the dark background.)
  Font font = GuiGetFont();
  Color title_color = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
  Color tagline_color = ColorBrightness(title_color, -0.30f);
  float spacing = (float)GuiGetStyle(DEFAULT, TEXT_SPACING);

  // --- Title -----------------------------------------------------------
  Vector2 title_size = MeasureTextEx(font, menu->title_text, (float)menu->title_size, spacing);
  float title_y = screen_height / 4.0f;
  DrawTextEx(font, menu->title_text,
             (Vector2){ (screen_width - title_size.x) / 2.0f, title_y },
             (float)menu->title_size, spacing, title_color);

  Vector2 tag_size = MeasureTextEx(font, menu->tagline_text, (float)menu->tagline_size, spacing);
  DrawTextEx(font, menu->tagline_text,
             (Vector2){ (screen_width - tag_size.x) / 2.0f, title_y + menu->title_size + 8 },
             (float)menu->tagline_size, spacing, tagline_color);

  // --- Menu buttons ----------------------------------------------------
  const float btn_w = (float)button->width;
  const float btn_h = (float)button->height;
  const float gap = (float)button->gap;
  float x = (screen_width - btn_w) / 2.0f;
  float y = screen_height / 2.0f;

  LoadingMenuAction action = LOADING_MENU_NONE;

  // Continue is only selectable when a save exists.
  if (!has_save) {
    GuiSetState(STATE_DISABLED);
  }
  if (GuiButton((Rectangle){x, y, btn_w, btn_h}, "CONTINUE")) {
    action = LOADING_MENU_CONTINUE;
  }
  GuiSetState(STATE_NORMAL);

  y += btn_h + gap;
  if (GuiButton((Rectangle){x, y, btn_w, btn_h}, "LOAD")) {
    action = LOADING_MENU_LOAD;
  }

  y += btn_h + gap;
  if (GuiButton((Rectangle){x, y, btn_w, btn_h}, "NEW GAME")) {
    action = LOADING_MENU_NEW;
  }

  y += btn_h + gap;
  if (GuiButton((Rectangle){x, y, btn_w, btn_h}, "EXIT")) {
    action = LOADING_MENU_EXIT;
  }

  return action;
}
