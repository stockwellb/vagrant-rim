#include "screen/confirm_quit.h"

#include "raygui.h"
#include "raylib.h"

ConfirmQuitAction confirm_quit_draw(int screen_width, int screen_height,
                                    const ConfirmQuitConfig *cfg,
                                    const ButtonConfig *button) {
  // Dim the frozen scene behind the modal.
  DrawRectangle(0, 0, screen_width, screen_height,
                (Color){ 0, 0, 0, (unsigned char)cfg->scrim_alpha });

  Font font = GuiGetFont();
  float spacing = (float)GuiGetStyle(DEFAULT, TEXT_SPACING);
  Color txt = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
  Color fill = GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL));
  Color border = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL));

  // --- Box -------------------------------------------------------------
  const float bw = (float)cfg->box_width;
  const float bh = (float)cfg->box_height;
  Rectangle box = { (screen_width - bw) / 2.0f, (screen_height - bh) / 2.0f, bw, bh };
  DrawRectangleRec(box, fill);
  DrawRectangleLinesEx(box, 2.0f, border);

  // --- Title / message -------------------------------------------------
  Vector2 ts = MeasureTextEx(font, cfg->title_text, (float)cfg->title_size, spacing);
  DrawTextEx(font, cfg->title_text,
             (Vector2){ box.x + (bw - ts.x) / 2.0f, box.y + 28.0f },
             (float)cfg->title_size, spacing, txt);

  Vector2 ms = MeasureTextEx(font, cfg->message_text, (float)cfg->message_size, spacing);
  DrawTextEx(font, cfg->message_text,
             (Vector2){ box.x + (bw - ms.x) / 2.0f, box.y + 92.0f },
             (float)cfg->message_size, spacing, txt);

  // --- Buttons ---------------------------------------------------------
  const float btn_w = 200.0f;
  const float btn_h = (float)button->height;
  const float gap = 24.0f;
  float total = btn_w * 2.0f + gap;
  float bx = box.x + (bw - total) / 2.0f;
  float by = box.y + bh - btn_h - 28.0f;

  ConfirmQuitAction action = CONFIRM_QUIT_NONE;
  if (GuiButton((Rectangle){ bx, by, btn_w, btn_h }, cfg->confirm_text)) {
    action = CONFIRM_QUIT_SAVE;
  }
  if (GuiButton((Rectangle){ bx + btn_w + gap, by, btn_w, btn_h }, cfg->cancel_text)) {
    action = CONFIRM_QUIT_DISCARD;
  }
  return action;
}
