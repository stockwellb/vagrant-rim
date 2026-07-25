# vagrant-rim

A space scavenger game. See [GAME_PLAY.md](GAME_PLAY.md) for the design vision — play mechanics and HUD concepts.

## Tech stack

- **C11** — core engine
- **Raylib** — rendering / input / audio
- **raygui** — immediate-mode UI (menus, buttons)
- **Lua** — config and save files (gameplay scripting planned)
- **xmake** — build system

## Building

```sh
xmake        # build (installs dependencies on first run)
xmake run    # build and launch the game
```

## Layout

```
src/
  main.c        entry point / game loop
  game/         top-level lifecycle + screen state machine (init/update/draw/shutdown)
  config/       loads assets/config.lua into typed C structs
  save/         save-file format and slot management (Lua save files)
  screen/       menu & overlay screens (loading, pause, slot picker, confirm-quit)
  ui/           raygui single-header implementation unit
  util/         shared helpers (Lua table field readers)

assets/
  config.lua    window, UI layout, content, and debug settings (edit without recompiling)
  styles/       raygui theme (.rgs) — colors and widget text size
  fonts/        UI fonts
```

Gameplay subsystems (ship, world, economy, ...) will be added as sibling
modules under `src/` as the project grows.
