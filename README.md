# vagrant-rim

A space scavenger game. See [GOALS.md](GOALS.md) for the design vision.

## Tech stack

- **C11** — core engine
- **Raylib** — rendering / input / audio
- **Lua** — config & scripting (planned)
- **xmake** — build system

## Building

```sh
xmake        # build (installs raylib on first run)
xmake run    # build and launch the game
```

## Layout

```
src/
  main.c        entry point / game loop
  game/         top-level game lifecycle (init/update/draw/shutdown)
```

Subsystems (ship, world, economy, ...) will be added as sibling modules
under `src/` as the project grows.
