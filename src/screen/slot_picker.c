#include "screen/slot_picker.h"

#include "raygui.h"
#include "raylib.h"

int slot_picker_draw(int screen_width, int screen_height, SlotPickerMode mode,
                     const SlotInfo *slots, int count) {
  (void)screen_height; // layout is top-anchored; height unused for now
  Font font = GuiGetFont();
  Color text_color = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
  float spacing = (float)GuiGetStyle(DEFAULT, TEXT_SPACING);

  // --- Title -----------------------------------------------------------
  const char *title = (mode == SLOT_PICKER_LOAD) ? "LOAD GAME" : "NEW GAME";
  int title_size = 48;
  Vector2 tsize = MeasureTextEx(font, title, (float)title_size, spacing);
  DrawTextEx(font, title, (Vector2){ (screen_width - tsize.x) / 2.0f, 60.0f },
             (float)title_size, spacing, text_color);

  // --- Slot rows -------------------------------------------------------
  const float row_w = 700.0f;
  const float row_h = 48.0f;
  const float gap = 10.0f;
  float x = (screen_width - row_w) / 2.0f;
  float y = 150.0f;

  // Left-align row text so the slot number + summary read as a list.
  int prev_align = GuiGetStyle(DEFAULT, TEXT_ALIGNMENT);
  int prev_pad = GuiGetStyle(DEFAULT, TEXT_PADDING);
  GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
  GuiSetStyle(DEFAULT, TEXT_PADDING, 14);

  int chosen = SLOT_PICKER_NONE;

  for (int i = 0; i < count; i++) {
    const char *label = TextFormat("SLOT %d   %s", i + 1, slots[i].label);

    // In load mode, empty slots can't be selected.
    bool selectable = (mode == SLOT_PICKER_LOAD) ? slots[i].used : true;
    if (!selectable) {
      GuiSetState(STATE_DISABLED);
    }
    if (GuiButton((Rectangle){ x, y, row_w, row_h }, label)) {
      chosen = i;
    }
    GuiSetState(STATE_NORMAL);

    y += row_h + gap;
  }

  GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, prev_align);
  GuiSetStyle(DEFAULT, TEXT_PADDING, prev_pad);

  // --- Back ------------------------------------------------------------
  float back_w = 200.0f;
  if (GuiButton((Rectangle){ (screen_width - back_w) / 2.0f, y + gap, back_w, row_h }, "BACK")) {
    chosen = SLOT_PICKER_BACK;
  }

  return chosen;
}
