#include "ui/ui.h"

#include <math.h>   // fabsf
#include <stddef.h> // NULL

#include "raygui.h"

#include "audio/audio.h"
#include "ui/gamepad_native.h"
#include "util/mathx.h"

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

// Analog auto-repeat for slider adjustment (VERTICAL menus): fire once when a
// direction is first held, wait DELAY, then emit a step every RATE while held.
// Tuned so a full 0..1 sweep at 5% steps takes ~1.6s and the per-step tick reads
// as a deliberate cadence rather than a machine-gun.
#define UI_ADJUST_DELAY 0.35f
#define UI_ADJUST_RATE 0.08f

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

// One left-stick axis (raylib convention), summed across the native (macOS) pad
// and every raylib pad so opposing inputs cancel rather than fight, then clamped.
static float pad_axis(int axis)
{
    float v = gamepad_native_axis(axis);
    for (int id = 0; id < 4; id++) {
        if (IsGamepadAvailable(id)) {
            v += GetGamepadAxisMovement(id, axis);
        }
    }
    return clamp_unit(v);
}

// One discrete focus step from an analog axis, edge-triggered with hysteresis:
// cross out past ON to fire once, fall back inside OFF before it can fire again.
// `latch` carries the between-frames state. `active` is whether this axis won the
// frame's X-vs-Y arbitration: an inactive axis never fires or newly latches (so a
// diagonal push can't consume the losing axis's next real step) but is still
// allowed to relax once it recenters.
static int stick_focus_step(float v, bool *latch, bool active)
{
    if (active && !*latch) {
        if (v < -UI_STICK_ON) { *latch = true; return -1; }
        if (v > UI_STICK_ON) { *latch = true; return 1; }
    }
    if (*latch && v > -UI_STICK_OFF && v < UI_STICK_OFF) {
        *latch = false;
    }
    return 0;
}

// One slider-adjust step this frame from a held direction, auto-repeating like a
// held key: immediate on the first frame held, then after DELAY, then every RATE.
// `dir` is the currently-held direction (-1/0/+1) from the keyboard or the
// arbitrated stick X; driving keyboard and stick through one timer keeps the
// adjust cadence (and its per-step tick) uniform instead of racing OS key-repeat.
static int adjust_repeat_step(int dir, bool *held, float *timer, float dt)
{
    if (dir == 0) {
        *held = false;
        *timer = 0.0f;
        return 0;
    }
    if (!*held) {
        *held = true;
        *timer = UI_ADJUST_DELAY;
        return dir;
    }
    *timer -= dt;
    if (*timer <= 0.0f) {
        *timer = UI_ADJUST_RATE;
        return dir;
    }
    return 0;
}

// Keyboard edge with OS key-repeat, so holding a direction key keeps stepping.
static bool key_step(int key)
{
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

// Directional intents kept separate on each axis: vertical drives focus, and
// horizontal drives value adjustment (sliders). Keyboard, WASD, and D-pad each.
static bool up_pressed(void)
{
    return key_step(KEY_UP) || key_step(KEY_W) || pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_UP);
}
static bool down_pressed(void)
{
    return key_step(KEY_DOWN) || key_step(KEY_S) || pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_DOWN);
}
static bool left_pressed(void)
{
    return key_step(KEY_LEFT) || key_step(KEY_A) || pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_LEFT);
}
static bool right_pressed(void)
{
    return key_step(KEY_RIGHT) || key_step(KEY_D) || pad_pressed(GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
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
    nav->adjust = 0;
    nav->activate = false;
    nav->cancel = false;
    nav->adjust_consumed = false;
    nav->stick_latched = false;
    nav->hstick_latched = false;
    nav->adjust_held = false;
    nav->adjust_timer = 0.0f;
    for (int i = 0; i < UI_MENU_MAX_ITEMS; i++) {
        nav->enabled[i] = true;
    }
}

// Which direction the keyboard/D-pad is requesting for slider adjustment (held,
// not edge — the adjust auto-repeat timer decides when to actually step).
static int adjust_key_dir(void)
{
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) return -1;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) return 1;
    return 0;
}

void ui_menu_nav_begin(MenuNav *nav, int count)
{
    ui_menu_nav_begin_mode(nav, count, MENU_NAV_BIDIRECTIONAL);
}

void ui_menu_nav_begin_mode(MenuNav *nav, int count, MenuNavMode mode)
{
    nav->count = (count > UI_MENU_MAX_ITEMS) ? UI_MENU_MAX_ITEMS : count;

    // Read each left-stick axis once, then arbitrate: only the more-displaced
    // axis acts this frame (ties go to Y / focus). Without this a diagonal push
    // would drive both axes at once — e.g. moving focus while also nudging the
    // focused slider. Sampling here also means the 4-pad scan in pad_axis runs
    // once per axis, not once per step helper.
    float ax = pad_axis(GAMEPAD_AXIS_LEFT_X);
    float ay = pad_axis(GAMEPAD_AXIS_LEFT_Y);
    bool y_active = fabsf(ay) >= fabsf(ax);
    bool x_active = !y_active;

    // Vertical focus step: up/down keys/D-pad, or the arbitrated stick Y.
    int vstep = up_pressed() ? -1 : down_pressed() ? 1 : 0;
    if (vstep == 0) {
        vstep = stick_focus_step(ay, &nav->stick_latched, y_active);
    }

    if (mode == MENU_NAV_VERTICAL) {
        // Up/down move focus; left/right are reported as an adjustment (sliders)
        // instead of moving focus. Adjust auto-repeats (keyboard or the arbitrated
        // stick X) so a held direction keeps stepping at a controlled cadence.
        nav->pending_dir = vstep;
        int adir = adjust_key_dir();
        if (adir == 0 && x_active) {
            adir = (ax <= -UI_STICK_ON) ? -1 : (ax >= UI_STICK_ON) ? 1 : 0;
        }
        nav->adjust = adjust_repeat_step(adir, &nav->adjust_held, &nav->adjust_timer,
                                         GetFrameTime());
    } else {
        // Any of the four directions moves focus; vertical wins when both fire.
        int hstep = left_pressed() ? -1 : right_pressed() ? 1 : 0;
        if (hstep == 0) {
            hstep = stick_focus_step(ax, &nav->hstick_latched, x_active);
        }
        nav->pending_dir = (vstep != 0) ? vstep : hstep;
        nav->adjust = 0;
    }

    nav->adjust_consumed = false;
    nav->activate = nav_activate_pressed();
    nav->cancel = ui_nav_cancel_pressed();

    // Buttons refill this during draw; default true so an item never drawn stays
    // navigable rather than silently vanishing from traversal.
    for (int i = 0; i < UI_MENU_MAX_ITEMS; i++) {
        nav->enabled[i] = true;
    }
}

// A *moving* mouse over `bounds` claims focus for item `index`, so the
// keyboard/pad highlight follows the pointer and there is only ever one focused
// item. The "moving" test matters: a mouse resting on an item must not re-claim
// focus every frame, or it would fight keyboard/gamepad navigation. Plays the
// focus tick when the pointer actually moves the highlight.
static void hover_claims_focus(MenuNav *nav, int index, Rectangle bounds, bool enabled)
{
    Vector2 md = GetMouseDelta();
    if (enabled && (md.x != 0.0f || md.y != 0.0f) &&
        CheckCollisionPointRec(GetMousePosition(), bounds) && nav->focus != index) {
        nav->focus = index;
        audio_play_nav();
    }
}

bool ui_menu_button(MenuNav *nav, int index, Rectangle bounds, const char *text,
                    bool enabled)
{
    if (index >= 0 && index < UI_MENU_MAX_ITEMS) {
        nav->enabled[index] = enabled;
    }

    hover_claims_focus(nav, index, bounds, enabled);

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

    bool activated = clicked || (focused && nav->activate);
    if (activated) {
        audio_play_select(); // confirm sound when an item is chosen
    }
    return activated;
}

float ui_menu_slider(MenuNav *nav, int index, Rectangle bounds, const char *label,
                     float value, float step)
{
    if (index >= 0 && index < UI_MENU_MAX_ITEMS) {
        nav->enabled[index] = true;
    }

    // Row layout: label on the left, a percentage readout on the right, the
    // slider bar filling the space between.
    const float label_w = bounds.width * 0.30f;
    const float pct_w = 56.0f;
    Rectangle bar = { bounds.x + label_w, bounds.y, bounds.width - label_w - pct_w,
                      bounds.height };

    // A moving mouse over the row claims focus, matching ui_menu_button so pointer
    // and keyboard never disagree.
    hover_claims_focus(nav, index, bounds, true);

    bool focused = (index == nav->focus);

    // Keyboard/gamepad adjust when focused. nav->adjust is already rate-limited by
    // the adjust auto-repeat, so ticking per step is a controlled cadence rather
    // than the OS key-repeat machine-gun; and the SFX slider previews its own new
    // level. Claim the adjust so nav_end knows this row (not a button) used it.
    if (focused && nav->adjust != 0) {
        nav->adjust_consumed = true;
        float adjusted = value + (float)nav->adjust * step;
        adjusted = clamp01(adjusted);
        if (adjusted != value) {
            value = adjusted;
            audio_play_nav(); // one tick per real step; silent at the rails
        }
    }

    Color label_color =
        focused ? ui_style_color(TEXT_COLOR_FOCUSED) : ui_style_color(TEXT_COLOR_NORMAL);
    const float text_size = 20.0f;
    float text_y = bounds.y + (bounds.height - text_size) / 2.0f;
    DrawTextEx(ui_font(), label, (Vector2){ bounds.x, text_y }, text_size, ui_spacing(),
               label_color);

    if (focused) {
        GuiSetState(STATE_FOCUSED); // raygui draws the bar highlighted, like a focused button
    }
    GuiSliderBar(bar, NULL, NULL, &value, 0.0f, 1.0f); // mouse drag updates value in place
    GuiSetState(STATE_NORMAL);

    const char *pct = TextFormat("%d%%", (int)(value * 100.0f + 0.5f));
    DrawTextEx(ui_font(), pct, (Vector2){ bar.x + bar.width + 10.0f, text_y }, text_size,
               ui_spacing(), label_color);

    return value;
}

void ui_menu_nav_end(MenuNav *nav)
{
    int count = nav->count;
    if (count <= 0) {
        return;
    }

    // In a VERTICAL menu left/right adjust the focused slider. If no slider
    // consumed this frame's adjust — the focused row is a plain button — fall back
    // to treating left/right as a focus move so navigation is never dead on a
    // button row (left = previous, right = next).
    if (nav->pending_dir == 0 && nav->adjust != 0 && !nav->adjust_consumed) {
        nav->pending_dir = nav->adjust;
    }
    nav->adjust = 0;

    if (nav->pending_dir != 0) {
        int start = nav->focus;
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
        if (nav->focus != start) {
            audio_play_nav(); // thud when keyboard/gamepad moves focus
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
