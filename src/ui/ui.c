#include "ui/ui.h"

#include "raygui.h"

#include "ui/gamepad_native.h"

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

// --- Menu navigation -------------------------------------------------------

// Left-stick step thresholds: cross OUT past `on` to register a step, fall back
// inside `off` before another can fire. The gap gives hysteresis so a stick held
// near the edge doesn't stutter.
#define UI_STICK_ON 0.5f
#define UI_STICK_OFF 0.35f

// True if any connected gamepad pressed `button` this frame. Scans the first few
// pads so it works regardless of which slot the OS assigned the controller.
static bool pad_pressed(int button)
{
    // Native layer first: on macOS it carries the real input (raylib's own
    // gamepad reads come back empty there); elsewhere it's a no-op stub.
    if (gamepad_native_button_pressed(button)) {
        return true;
    }
    for (int id = 0; id < 4; id++) {
        if (IsGamepadAvailable(id) && IsGamepadButtonPressed(id, button)) {
            return true;
        }
    }
    return false;
}

// Clamp to the [-1, 1] range an axis value is expected to occupy.
static float clamp_unit(float v)
{
    if (v < -1.0f) return -1.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// Whichever of the two axes is pushed farther, keeping its sign, so a single
// stick drives both vertical and horizontal menus.
static float dominant_axis(float x, float y)
{
    return ((x < 0 ? -x : x) >= (y < 0 ? -y : y)) ? x : y;
}

// Left-stick displacement as a single signed value: negative = up/left (previous
// item), positive = down/right (next item). Sums each axis across the native
// (macOS) pad and every raylib pad — so opposing inputs cancel instead of fighting
// over "most displaced" — then clamps and takes the dominant axis.
static float pad_step_axis(void)
{
    float x = gamepad_native_axis(GAMEPAD_AXIS_LEFT_X);
    float y = gamepad_native_axis(GAMEPAD_AXIS_LEFT_Y);
    for (int id = 0; id < 4; id++) {
        if (!IsGamepadAvailable(id)) {
            continue;
        }
        x += GetGamepadAxisMovement(id, GAMEPAD_AXIS_LEFT_X);
        y += GetGamepadAxisMovement(id, GAMEPAD_AXIS_LEFT_Y);
    }
    return dominant_axis(clamp_unit(x), clamp_unit(y));
}

// Keyboard edge with OS key-repeat, so holding a direction key keeps stepping.
static bool key_step(int key)
{
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

// "Previous" and "next" rather than strictly up/down: left/up both step back and
// right/down both step forward, so the same list works vertically or horizontally.
static bool nav_prev_pressed(void)
{
    return key_step(KEY_UP) || key_step(KEY_W) || key_step(KEY_LEFT) || key_step(KEY_A) ||
           pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_UP) ||
           pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_LEFT);
}

static bool nav_next_pressed(void)
{
    return key_step(KEY_DOWN) || key_step(KEY_S) || key_step(KEY_RIGHT) ||
           key_step(KEY_D) || pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_DOWN) ||
           pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
}

static bool nav_activate_pressed(void)
{
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
           pad_pressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN); // Xbox A / PS Cross
}

bool ui_nav_menu_pressed(void)
{
    return IsKeyPressed(KEY_ESCAPE) || pad_pressed(GAMEPAD_BUTTON_MIDDLE_RIGHT); // Start
}

bool ui_nav_cancel_pressed(void)
{
    return IsKeyPressed(KEY_ESCAPE) || pad_pressed(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT); // B
}

void ui_input_poll(void)
{
    gamepad_native_update();
}

void ui_menu_nav_reset(MenuNav *nav)
{
    nav->focus = 0;
    nav->count = 0;
    nav->pending_dir = 0;
    nav->activate = false;
    nav->cancel = false;
    nav->stick_latched = false;
    for (int i = 0; i < UI_MENU_MAX_ITEMS; i++) {
        nav->enabled[i] = true;
    }
}

void ui_menu_nav_begin(MenuNav *nav, int count)
{
    nav->count = (count > UI_MENU_MAX_ITEMS) ? UI_MENU_MAX_ITEMS : count;

    int dir = 0;
    if (nav_prev_pressed()) {
        dir = -1;
    } else if (nav_next_pressed()) {
        dir = 1;
    } else {
        // Analog stick: one step per push, edge-triggered with hysteresis.
        float s = pad_step_axis();
        if (!nav->stick_latched) {
            if (s < -UI_STICK_ON) {
                dir = -1;
                nav->stick_latched = true;
            } else if (s > UI_STICK_ON) {
                dir = 1;
                nav->stick_latched = true;
            }
        } else if (s > -UI_STICK_OFF && s < UI_STICK_OFF) {
            nav->stick_latched = false;
        }
    }
    nav->pending_dir = dir;
    nav->activate = nav_activate_pressed();
    nav->cancel = ui_nav_cancel_pressed();

    // Buttons refill this during draw; default true so an item never drawn stays
    // navigable rather than silently vanishing from traversal.
    for (int i = 0; i < UI_MENU_MAX_ITEMS; i++) {
        nav->enabled[i] = true;
    }
}

bool ui_menu_button(MenuNav *nav, int index, Rectangle bounds, const char *text,
                    bool enabled)
{
    if (index >= 0 && index < UI_MENU_MAX_ITEMS) {
        nav->enabled[index] = enabled;
    }

    // A *moving* mouse over an enabled button claims focus, so the keyboard/pad
    // highlight follows the pointer and there is only ever one focused item. The
    // "moving" test matters: a mouse resting on a button must not re-claim focus
    // every frame, or it would fight keyboard/gamepad navigation.
    Vector2 md = GetMouseDelta();
    if (enabled && (md.x != 0.0f || md.y != 0.0f) &&
        CheckCollisionPointRec(GetMousePosition(), bounds)) {
        nav->focus = index;
    }

    bool focused = enabled && (index == nav->focus);

    if (!enabled) {
        GuiSetState(STATE_DISABLED);
    } else if (focused) {
        GuiSetState(STATE_FOCUSED); // raygui draws this as the hover look
    } else {
        GuiSetState(STATE_NORMAL);
    }
    bool clicked = GuiButton(bounds, text);
    GuiSetState(STATE_NORMAL);

    return clicked || (focused && nav->activate);
}

void ui_menu_nav_end(MenuNav *nav)
{
    int count = nav->count;
    if (count <= 0) {
        return;
    }

    if (nav->pending_dir != 0) {
        int i = nav->focus;
        for (int step = 0; step < count; step++) {
            i += nav->pending_dir;
            if (i < 0) {
                i = count - 1;
            } else if (i >= count) {
                i = 0;
            }
            if (nav->enabled[i]) {
                nav->focus = i;
                break;
            }
        }
        nav->pending_dir = 0;
    }

    // Focus may sit on a disabled or out-of-range item after a screen's enabled
    // set changes (Continue with no save, empty load slots). Snap to the first
    // enabled item so the highlight is always on something selectable.
    if (nav->focus < 0 || nav->focus >= count || !nav->enabled[nav->focus]) {
        for (int i = 0; i < count; i++) {
            if (nav->enabled[i]) {
                nav->focus = i;
                break;
            }
        }
    }
}
