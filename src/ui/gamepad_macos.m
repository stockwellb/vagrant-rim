#include "ui/gamepad_native.h"

#import <GameController/GameController.h>

#include "raylib.h" // GamepadButton / GamepadAxis enum values

// macOS gamepad input via Apple's GameController framework. On modern macOS the
// system's XboxGamepad DriverKit driver claims Xbox (and PlayStation) pads and
// exposes them only through this framework, so GLFW's IOKit HID path — which
// raylib relies on — never sees button or axis values (raylib #3651). We read
// the same framework the OS itself uses, then hand results to ui.c through the
// gamepad_native_* API so the existing MenuNav layer drives menus unchanged.

#define NATIVE_BUTTON_COUNT 18 // raylib GamepadButton values run 0..17

static bool s_pressed[NATIVE_BUTTON_COUNT]; // edge fired this frame (any pad)
static float s_axis[6];                     // merged per-axis value, raylib convention

// Previous frame's held-button bitmask, per controller, so edges are detected
// per pad instead of on a merged level (where one pad holding a button would
// swallow another pad's press of it). Weak keys let the framework drop entries
// automatically as controllers disconnect.
static NSMapTable<GCController *, NSNumber *> *s_prevButtons;

static float clamp_unit(float v)
{
    if (v < -1.0f) return -1.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// Is `rlButton` (a raylib GamepadButton value) currently held on this pad?
// buttonOptions/buttonHome/thumbstick buttons are nil on some controllers and OS
// versions; we rely on Objective-C nil-messaging (a message to nil yields NO for
// the BOOL property) rather than explicit guards, so the switch stays flat.
static bool button_down(GCExtendedGamepad *g, int rlButton)
{
    switch (rlButton) {
        case GAMEPAD_BUTTON_LEFT_FACE_UP:    return g.dpad.up.pressed;
        case GAMEPAD_BUTTON_LEFT_FACE_RIGHT: return g.dpad.right.pressed;
        case GAMEPAD_BUTTON_LEFT_FACE_DOWN:  return g.dpad.down.pressed;
        case GAMEPAD_BUTTON_LEFT_FACE_LEFT:  return g.dpad.left.pressed;
        case GAMEPAD_BUTTON_RIGHT_FACE_UP:    return g.buttonY.pressed;
        case GAMEPAD_BUTTON_RIGHT_FACE_RIGHT: return g.buttonB.pressed;
        case GAMEPAD_BUTTON_RIGHT_FACE_DOWN:  return g.buttonA.pressed;
        case GAMEPAD_BUTTON_RIGHT_FACE_LEFT:  return g.buttonX.pressed;
        case GAMEPAD_BUTTON_LEFT_TRIGGER_1:  return g.leftShoulder.pressed;
        case GAMEPAD_BUTTON_LEFT_TRIGGER_2:  return g.leftTrigger.pressed;
        case GAMEPAD_BUTTON_RIGHT_TRIGGER_1: return g.rightShoulder.pressed;
        case GAMEPAD_BUTTON_RIGHT_TRIGGER_2: return g.rightTrigger.pressed;
        case GAMEPAD_BUTTON_MIDDLE_LEFT:  return g.buttonOptions.pressed; // View
        case GAMEPAD_BUTTON_MIDDLE:       return g.buttonHome.pressed;    // Guide
        case GAMEPAD_BUTTON_MIDDLE_RIGHT: return g.buttonMenu.pressed;    // Start/Menu
        case GAMEPAD_BUTTON_LEFT_THUMB:   return g.leftThumbstickButton.pressed;
        case GAMEPAD_BUTTON_RIGHT_THUMB:  return g.rightThumbstickButton.pressed;
        default: return false;
    }
}

// Value of `rlAxis` on this pad in raylib's convention. Two conversions from the
// GameController framework: (1) raylib reports stick Y positive when pushed down
// while GC reports positive when pushed up, so the vertical sticks are negated;
// (2) raylib trigger axes rest at -1 and reach +1, while GC triggers are 0..1,
// so triggers are remapped to [-1, 1].
static float axis_value(GCExtendedGamepad *g, int rlAxis)
{
    switch (rlAxis) {
        case GAMEPAD_AXIS_LEFT_X:  return g.leftThumbstick.xAxis.value;
        case GAMEPAD_AXIS_LEFT_Y:  return -g.leftThumbstick.yAxis.value;
        case GAMEPAD_AXIS_RIGHT_X: return g.rightThumbstick.xAxis.value;
        case GAMEPAD_AXIS_RIGHT_Y: return -g.rightThumbstick.yAxis.value;
        case GAMEPAD_AXIS_LEFT_TRIGGER:  return g.leftTrigger.value * 2.0f - 1.0f;
        case GAMEPAD_AXIS_RIGHT_TRIGGER: return g.rightTrigger.value * 2.0f - 1.0f;
        default: return 0.0f;
    }
}

void gamepad_native_update(void)
{
    if (s_prevButtons == nil) {
        s_prevButtons = [NSMapTable weakToStrongObjectsMapTable];
    }

    unsigned int mergedEdges = 0;
    float stick[4] = { 0.0f };            // summed across pads, then clamped
    float trigger[2] = { -1.0f, -1.0f };  // most-pressed wins; -1 is raylib rest

    // Drain per-frame temporaries (the controllers array, boxed bitmasks) here
    // rather than leaning on the run loop's pool.
    @autoreleasepool {
        for (GCController *c in [GCController controllers]) {
            GCExtendedGamepad *g = c.extendedGamepad;
            if (g == nil) {
                continue;
            }

            // Per-controller edge: bits set now that were clear last frame on
            // THIS pad. OR-ing the edges (not the levels) means a button held on
            // one pad never masks the same button's press on another.
            unsigned int cur = 0;
            for (int b = 0; b < NATIVE_BUTTON_COUNT; b++) {
                if (button_down(g, b)) {
                    cur |= (1u << b);
                }
            }
            NSNumber *prev = [s_prevButtons objectForKey:c];
            mergedEdges |= cur & ~(prev ? prev.unsignedIntValue : 0u);
            [s_prevButtons setObject:@(cur) forKey:c];

            // Sticks sum across pads so opposing inputs cancel rather than
            // letting near-equal magnitudes flip the merged sign frame to frame.
            // Triggers take the most-pressed pad.
            for (int a = 0; a < 4; a++) {
                stick[a] += axis_value(g, a);
            }
            for (int t = 0; t < 2; t++) {
                float v = axis_value(g, GAMEPAD_AXIS_LEFT_TRIGGER + t);
                if (v > trigger[t]) {
                    trigger[t] = v;
                }
            }
        }
    }

    for (int b = 0; b < NATIVE_BUTTON_COUNT; b++) {
        s_pressed[b] = (mergedEdges >> b) & 1u;
    }
    for (int a = 0; a < 4; a++) {
        s_axis[a] = clamp_unit(stick[a]);
    }
    s_axis[GAMEPAD_AXIS_LEFT_TRIGGER] = trigger[0];
    s_axis[GAMEPAD_AXIS_RIGHT_TRIGGER] = trigger[1];
}

bool gamepad_native_button_pressed(int button)
{
    if (button < 0 || button >= NATIVE_BUTTON_COUNT) {
        return false;
    }
    return s_pressed[button];
}

float gamepad_native_axis(int axis)
{
    if (axis < 0 || axis >= 6) {
        return 0.0f;
    }
    return s_axis[axis];
}
