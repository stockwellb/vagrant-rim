#ifndef VAGRANT_RIM_GAMEPAD_NATIVE_H
#define VAGRANT_RIM_GAMEPAD_NATIVE_H

#include <stdbool.h>

// Native controller input, used to work around raylib/GLFW not delivering
// gamepad button/axis values on macOS: there Apple's GameController framework
// (the XboxGamepad DriverKit driver + gamecontrollerd) owns the pad and routes
// it away from the IOKit HID path GLFW reads, so raylib reports the pad as
// "available" but every button/axis reads zero (raylib #3651). On macOS this is
// backed by GameController.framework; on every other platform it is a no-op stub
// and raylib's own gamepad functions do the work.
//
// Button/axis identifiers are raylib's GamepadButton / GamepadAxis enum values,
// so callers share one vocabulary regardless of which backend answers.

// Sample current controller state once per frame, before any of the query
// functions below. Also computes button edges for gamepad_native_button_pressed.
void gamepad_native_update(void);

// True if any connected controller pressed `button` (raylib GamepadButton value)
// this frame — edge-triggered, mirroring raylib's IsGamepadButtonPressed.
bool gamepad_native_button_pressed(int button);

// Current value of `axis` (raylib GamepadAxis value), -1..1, in raylib's sign
// convention (LEFT_Y positive = down), taking the most-displaced controller.
float gamepad_native_axis(int axis);

#endif // VAGRANT_RIM_GAMEPAD_NATIVE_H
