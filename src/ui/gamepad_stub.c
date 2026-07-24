#include "ui/gamepad_native.h"

// Non-macOS builds: raylib/GLFW reads Xbox controllers correctly, so the native
// layer does nothing and ui.c falls back to raylib's gamepad functions. The
// macOS implementation lives in gamepad_macos.m and replaces this file there
// (see xmake.lua).

void gamepad_native_update(void) {}

bool gamepad_native_button_pressed(int button)
{
    (void)button;
    return false;
}

float gamepad_native_axis(int axis)
{
    (void)axis;
    return 0.0f;
}
