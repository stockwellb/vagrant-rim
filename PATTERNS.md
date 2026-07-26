# PATTERNS.md — How this codebase "does OOP" in C

This is a guide for an experienced Java/C# developer who knows C syntax but hasn't
lived in a language without classes, garbage collection, generics, or exceptions.

C gives you three tools that matter here: **structs** (data), **functions**
(behavior), and the **translation unit** (a `.c` file — the unit of compilation and
of privacy). Everything you're used to reaching for — classes, interfaces,
constructors, `private`, singletons, dependency injection, exceptions,
`Optional<T>`, `List<T>` — has to be *reconstructed* out of those three tools plus
some discipline. This document is the decoder ring: for each pattern, what the C
code does, the OOP construct it stands in for, and *why* it's done this way.

Read the code alongside it. Every pattern below is pulled from files under `src/`.

---

## The mental model in one paragraph

There are no objects. There are **modules**. A module is a `.h`/`.c` pair living in
its own folder (`src/audio/`, `src/save/`, `src/config/`). The `.h` is the public
interface (what other modules may call); the `.c` is the implementation. Functions
are **named with the module as a prefix** (`audio_init`, `save_load_slot`,
`config_load`) because C has one flat global namespace — the prefix *is* the
namespace. A function that operates on a struct takes a **pointer to that struct as
its first argument** — that pointer is your `this`. That's the whole paradigm.
Once you internalize "folder = class/package, prefix = namespace, first pointer arg
= `this`," the rest is detail.

---

## 1. Module = class/package (header + implementation, prefixed functions)

**Look at:** `src/audio/audio.h` + `src/audio/audio.c`, `src/save/save.h` + `save.c`.

The `.h` declares the public surface; the `.c` defines it. Callers `#include` the
header and never see the implementation. Compare a C# class:

```csharp
// C#                                   // C
public class Audio {                    // audio.h  — the "public" members
    public void Init(AudioConfig c);    //   void audio_init(const AudioConfig *cfg);
    public void Shutdown();             //   void audio_shutdown(void);
    public void PlayNav();              //   void audio_play_nav(void);
    private static Music _music;        // audio.c  — the "private" members
}                                       //   static Music s_music;
```

- **The `.h` is the `public` section.** If a declaration isn't in the header, no
  other file can call it.
- **Every function is prefixed** (`audio_*`). There is no `namespace` keyword in C;
  `audio_play_nav()` is just a globally-visible symbol whose *name* keeps it from
  colliding with `save_play_nav` or whatever. Treat the prefix as mandatory.
- **`const` on a pointer parameter is your read-only contract.**
  `audio_init(const AudioConfig *cfg)` promises not to mutate the config —
  the analogue of passing an immutable/`readonly` reference.

Why like this? The compiler processes each `.c` in isolation. The header is the
*only* thing it knows about another module. Keeping headers minimal is what keeps
build times and coupling down — it's the physical enforcement of "program to an
interface."

---

## 2. The struct + first-pointer-arg is your object (`this` is explicit)

**Look at:** `Game` in `src/game/game.h`, and every `game_*` function in `game.c`.

```c
typedef struct Game { ... } Game;      // the fields

void game_init(Game *game);            // "constructor"
void game_update(Game *game);          // a method
void game_draw(Game *game);            // a method
bool game_should_close(const Game *g); // a read-only method (note const)
void game_shutdown(Game *game);        // "destructor"
```

`main.c` is the whole story:

```c
Game game;                       // stack-allocated; no `new`
game_init(&game);                // pass its address as the explicit `this`
while (!game_should_close(&game)) {
    game_update(&game);
    game_draw(&game);
}
game_shutdown(&game);
```

Key differences from Java/C#:

- **`this` is a parameter, not a keyword.** `game->screen` inside `game.c` is what
  you'd write as `this.screen`. The `->` operator is "dereference then access field"
  — it's `.` through a pointer.
- **`Game game;` allocates on the stack.** No heap, no `new`, no GC. When `main`
  returns, it's gone. Large, long-lived aggregate roots like `Game` are commonly
  stack-allocated in `main` and threaded everywhere by pointer.
- **`const Game *g`** means "a method that doesn't mutate state" — like a C# method
  with no side effects, but the compiler actually enforces it.

This "struct + functions taking `T*`" is the single most important pattern. `MenuNav`
(`ui.h`), `Config`, `GameSave` all follow it.

---

## 3. Lifecycle is manual and paired (no constructors, no GC, no `using`)

**Look at:** `game_init`/`game_shutdown` (`game.c`), `audio_init`/`audio_shutdown`
(`audio.c`), and the font handling in `game.c`.

There is no constructor that runs automatically and no destructor/finalizer. You
call an `_init` explicitly and you **must** call the matching `_shutdown`. Every
acquired resource has a hand-written release:

```c
// game_shutdown (game.c) — mirror image of what init acquired
audio_shutdown();                       // pairs with audio_init
if (game->ui_font_loaded) {
    UnloadFont(game->ui_font);          // pairs with LoadFontEx
}
CloseWindow();                          // pairs with InitWindow
```

Note the **ownership flag** `ui_font_loaded` (`game.h:39`, set in `load_ui_font`):

```c
// LoadFontEx returns the *built-in* font on failure. Don't adopt (or later
// unload) something we don't own.
if (f.texture.id == 0 || f.texture.id == GetFontDefault().texture.id) { ...return; }
...
game->ui_font = f;
game->ui_font_loaded = true;            // remember we own it, so shutdown frees it
```

This is the manual version of what RAII / `IDisposable` / try-with-resources do for
free. The discipline: **for every `Load*`/`Init*`/`fopen`/`malloc`, find its release
and pair them, and track ownership with a bool when acquisition can fail.** The
codebase is careful about this — see `load_ui_sound` in `audio.c`, which
`UnloadSound`s a failed handle before falling back rather than leaking it.

---

## 4. Errors are return values, not exceptions (`bool` + out-parameter)

**Look at:** `config_load` (`config.h:151`, `config.c`), `save_load`/`save_write`
(`save.c`), `atomic_write` (`atomic_file.c`).

C has no exceptions. A function that can fail returns a **status** and delivers its
real result through a **pointer argument you passed in**:

```c
bool config_load(Config *config, const char *path, char *err, int err_size);
//   ^ success/fail   ^ result written here      ^ error message written here
```

Caller pattern (from `game.c`):

```c
char err[256];
if (config_load(&game->config, path, err, sizeof(err))) {
    TraceLog(LOG_INFO, "CONFIG: loaded '%s'", path);
} else {
    TraceLog(LOG_WARNING, "CONFIG: failed: %s — using defaults", err);
    config_set_defaults(&game->config);
}
```

Map to what you know:

- `bool` return replaces `try/catch`. There is no stack unwinding; **you check the
  return value at every call site.**
- The **out-parameter** replaces a multi-value return or a `(bool ok, T value)`
  tuple. C functions return exactly one value, so "give me success *and* data" is
  spelled "return the success, write the data through a pointer."
- **The failure contract is explicit and consistent:** on failure the output is
  *left untouched*. See `save_load` (`save.c:65`) — it fills a local `GameSave tmp`
  and only copies it to `*save` on full success, so a half-parsed file never
  corrupts the caller's data. That "commit at the end" style is how you get
  transactional behavior without exceptions.

---

## 5. The caller owns the memory (no hidden allocation, `Optional` is a convention)

**Look at:** `save_slot_path`, `asset_resolve` (`assets.c`), any `snprintf` into a
fixed buffer.

In C# a method returns a `string` and the GC cleans up. In C, **returning a
heap-allocated string means handing the caller a cleanup obligation** — so the
codebase avoids it almost entirely. Instead, the *caller* provides the buffer:

```c
void asset_resolve(const char *rel, char *out, int out_size);  // signature idea
...
char path[512];                          // caller's stack buffer
if (asset_resolve("audio/music.ogg", path, sizeof(path))) { ... }
```

The `char *out, int out_size` pair is a mini contract: "here's my buffer, here's how
big it is, don't overrun it." The callee uses `snprintf(out, out_size, ...)`, which
truncates rather than overflowing. You'll see `sizeof(buf)` passed as the size
*everywhere* — that's not paranoia, it's the only defense against buffer overruns,
which are memory-corruption bugs, not exceptions.

There is essentially **no `malloc` in the shell.** The one transient allocation
(`synth_click` in `audio.c`, for a sound buffer) `free`s it in the same function.
Everything else lives in structs that are either stack-allocated or embedded in
`Game`. This is deliberate: no allocation means no leaks, no fragmentation, and no
allocation-failure paths to test.

**The C `Optional<T>`:** there isn't one. The conventions are (a) `bool` return +
out-param ("present?"), and (b) a **sentinel value** — see the next pattern.

---

## 6. Enums as sentinels, state, and "commands" (the shell's backbone)

**Look at:** `ScreenId` (`game.h`), the `*Action` enums (every `src/screen/*.h`),
and `SLOT_PICKER_NONE`/`SLOT_PICKER_BACK` (`slot_picker.h`).

Three distinct uses of enums/ints, all worth recognizing:

**(a) State machine.** `ScreenId` is the current screen; `game_draw` is one big
`switch (game->screen)`. This is a classic finite state machine — the same thing
you'd model with a State pattern and polymorphic `Screen` subclasses in C#, done
with an enum and a switch. Given five screens, the switch is clearer and cheaper
than five classes and a vtable.

**(b) Command / intent enum.** This is the nicest architectural idea in the shell,
so slow down here. A screen's `_draw` function **does not mutate game state.** It
draws itself and returns an *action* describing what the player asked for:

```c
typedef enum PauseMenuAction {
    PAUSE_NONE = 0,   // nothing this frame  (note: 0, see below)
    PAUSE_RESUME, PAUSE_SAVE, PAUSE_SETTINGS, PAUSE_QUIT,
} PauseMenuAction;
```

The caller (`game.c`) turns that intent into state changes in a `handle_*` function:

```c
PauseMenuAction action = pause_menu_draw(...);   // screen reports intent
handle_pause_action(game, action);               // game applies it
```

This cleanly separates **rendering/input** (the screen module, which knows nothing
about `Game`) from **state transitions** (the `handle_*` functions in `game.c`,
which know nothing about pixels). It's the Command pattern, and it's *why the flow
tests exist* — see §12. Notice the screen modules depend on `config.h` and `ui.h`
but **not** on `game.h`; the dependency only points one way.

**(c) Sentinels.** `PAUSE_NONE = 0` is "no action this frame." Making the
do-nothing case `0` is idiomatic: a zeroed struct (`memset(&g, 0, ...)`, or a
`{0}` initializer) naturally starts in the neutral state. Where an `int` is
overloaded to carry either a real index *or* a special result, negative sentinels
are used: `slot_picker_draw` returns `>= 0` for a chosen slot, or `SLOT_PICKER_NONE`
(`-1`) / `SLOT_PICKER_BACK` (`-2`). That's C's version of returning
`int?`/`Optional<int>` plus an enum, packed into one `int`.

---

## 7. Module-global state = the singleton (via `static` at file scope)

**Look at:** the top of `audio.c` and `gamepad_macos.m`.

```c
static bool  s_ready;         // file-scope + static  ==  private static field
static Music s_music;         // of an implicit singleton "class" (this .c file)
static float s_music_volume = 0.5f;
```

`static` at file scope means **internal linkage**: the symbol exists only within
this translation unit. No other `.c` can name `s_music`. Combined with the public
`audio_*` functions, `audio.c` *is* a singleton — one audio device, one music
stream, reachable only through its methods. This is exactly a C# `static class`, or
a singleton whose instance is hidden.

Conventions to notice:
- **`s_` prefix** flags "module-static state" at a glance (like an `_` field prefix
  in C#).
- The whole module **guards on `s_ready`** so that if the device failed to open,
  every entry point is a safe no-op. That's the Null Object pattern (§10) applied to
  a singleton: callers never check "is audio up?" — they just call.

Trade-off (same as any singleton): global state isn't easily instantiated twice or
mocked. It's chosen here because there genuinely is exactly one audio device for one
game. `Game` state, by contrast, is a passed-around struct precisely so it *is*
testable (§12). Recognize the deliberate split.

---

## 8. `static` functions = private methods

**Look at:** `load_config`, `save_game`, `draw_playing` in `game.c`; `pad_pressed`,
`stick_focus_step` in `ui.c`.

A `static` function (as opposed to a `static` *variable*) is one with internal
linkage — callable only within its `.c`. That's your `private` method. If it's not
in the `.h`, it's private, full stop.

Note the overloaded keyword, a classic C gotcha for newcomers:
- `static` on a **local variable** → persists across calls (like a C# method-local
  that keeps its value; rarely used here).
- `static` on a **file-scope variable** → module-private global (§7).
- `static` on a **function** → module-private function (this section).

Same word, three meanings, disambiguated by *where* it appears.

---

## 9. Compile-time polymorphism: one interface, per-platform implementation

**Look at:** `gamepad_native.h` (the interface), `gamepad_stub.c` and
`gamepad_macos.m` (two implementations), and the swap in `xmake.lua:31-37`.

This is the pattern an OOP dev will most want a name for. `gamepad_native.h`
declares three functions:

```c
void  gamepad_native_update(void);
bool  gamepad_native_button_pressed(int button);
float gamepad_native_axis(int axis);
```

Two files implement that identical interface:
- `gamepad_stub.c` — every function is a no-op (non-macOS: raylib reads the pad).
- `gamepad_macos.m` — a real implementation over Apple's GameController framework
  (macOS: raylib/GLFW can't read Xbox pads there — see the file's header comment).

**The build picks one:**

```lua
if is_plat("macosx") then
    add_files("src/ui/gamepad_macos.m")
    remove_files("src/ui/gamepad_stub.c")   -- swap the implementation
    ...
end
```

`ui.c` calls `gamepad_native_*` and neither knows nor cares which body it links
against. That's the **Strategy pattern / dependency injection resolved at link time**
instead of through a runtime interface + vtable. No `interface`, no virtual dispatch,
no per-call indirection — the linker binds the one true implementation. C does
"program to an interface, not an implementation" by making the interface a header
and letting the build system choose the `.c`.

(The runtime-dispatch version — a struct of function pointers acting as a vtable —
also exists in C, but isn't needed here since the choice is fixed at build time.)

---

## 10. Graceful degradation = the Null Object pattern

**Look at:** the `s_ready` no-op guards in `audio.c`; the synthesized-sound fallback
(`synth_click`, `load_ui_sound`); the built-in-font fallback in `game.c`; the
no-op `gamepad_stub.c`.

Instead of returning null and making callers check, a subsystem that can't do the
real thing **substitutes a harmless stand-in that satisfies the same interface**:

- No audio device? `audio_play_nav()` still exists and just does nothing.
- No nav sound asset? `synth_click` generates one so the UI is never silent.
- No custom font? Keep raygui's built-in and carry on.
- Not on macOS? `gamepad_stub.c` answers "no input" to every native query.

The payoff is that **call sites have no branches**. `ui.c` calls `audio_play_nav()`
unconditionally; `game.c` draws with `GuiGetFont()` whether or not a custom font
loaded. In OOP you'd inject a `NullAudio : IAudio`. Here the "null object" is either
a set of guarded no-ops behind the same functions, or a swapped-in stub `.c` (§9).

---

## 11. Fixed-size arrays + a count (there is no `List<T>`)

**Look at:** `slot_infos[SAVE_MAX_SLOTS]` + `slot_info_count` (`game.h`);
`enabled[UI_MENU_MAX_ITEMS]` (`ui.h`); `#define SAVE_MAX_SLOTS 12` (`save.h`).

C's standard library has no growable list, map, or generic container. The idiom is a
**fixed-capacity array paired with an integer count** of how many slots are live:

```c
SlotInfo slot_infos[SAVE_MAX_SLOTS];  // capacity (a compile-time constant)
int      slot_info_count;             // how many are actually valid
```

You iterate `for (i = 0; i < slot_info_count; i++)`. The `#define` upper bound lets
the array be embedded directly in `Game` (no allocation, known size) and lets the
compiler size everything at build time. The cost is a hard ceiling — hence
`config.save.slots` is **clamped** to `[1, SAVE_MAX_SLOTS]` (`config.c:130`) so
configuration can never index past the array. Whenever you see a `MAX` `#define`,
look for the clamp that enforces it; they come in pairs.

This is a real philosophical shift from Java/C#: prefer **bounded, preallocated**
data over unbounded dynamic growth. It trades flexibility for zero allocation and
predictable memory — the right trade for a game loop running 60 times a second.

---

## 12. Designing for testability: the link seam and the "friend" header

**Look at:** `xmake.lua` (the `vagrant-core` static lib), `game_internal.h`, and
`test/test_game_flow.c`.

Two problems: (1) `main.c` has *the* `main()`, but a test binary needs its own; and
(2) the interesting logic (`handle_pause_action`, etc.) is `static`-ish inside
`game.c` and needs a window to run through the normal path. Both are solved with
build-and-header tricks that stand in for C#'s `InternalsVisibleTo` and interface
seams:

- **Core-as-a-library seam.** All of `src/` *except* `main.c` is compiled into a
  static lib `vagrant-core` (`xmake.lua:17-24`). The game binary is `main.c` + that
  lib; each test binary is `test_x.c` + the *same* lib. Tests exercise the identical
  code the game ships. This is the "extract everything but `Main` so it's linkable"
  move.

- **The `_internal.h` "friend" header.** `handle_pause_action` and the other
  transition handlers are the pure state-machine logic (§6b) — no window, no
  drawing. They're declared in `game_internal.h`, whose comment states plainly it is
  *not* a public API: it exists so `test_game_flow.c` can call the handlers directly
  on a stack `Game`. That's C's version of package-private / `internal` visibility —
  a header you only include from `game.c` and the tests.

- **The tests read like unit tests.** Because the handlers are pure
  `(Game*, action) -> mutated Game*`, a test just builds a `Game` with `memset(&g,
  0, ...)`, calls a handler, and asserts on `g.screen`. No window is ever opened —
  the whole flow suite runs headless in CI. This payoff is *the reason* §6b keeps
  rendering and state-transition separate. The architecture and the test strategy
  are the same decision.

---

## 13. Boundary validation: clamp untrusted input where it enters

**Look at:** `clamp_config` (`config.c:109`), the `clamp*` helpers (`mathx.c`),
`clamp01` calls throughout `audio.c`.

Config and save files are **untrusted input** — a user can hand-edit `config.lua` or
a save. Rather than sprinkle validation at every use, the code validates **once, at
the boundary**, right after parsing:

```c
config->window.width      = clampi(config->window.width, 320, 7680);
config->save.slots        = clampi(config->save.slots, 1, SAVE_MAX_SLOTS);
config->audio.music_volume= clampf(config->audio.music_volume, 0.0f, 1.0f);
```

The comments say why each bound exists (font-atlas size, array safety, a scrim alpha
that would wrap if it exceeded 255). This is "validate at the edge, trust
internally" — the same instinct as sanitizing at an API boundary — but here it's
also **memory safety**: an unclamped `slots` would index past a fixed array (§11),
which in C is undefined behavior, not an exception. The tiny `mathx` module exists so
this clamping vocabulary (`clampi`/`clampf`/`clamp01`/`clamp_unit`) is written once
and shared.

---

## 14. Overlay/merge configuration: "leave `*out` untouched if absent"

**Look at:** `lua_util.h`/`lua_util.c` (the field readers) and how `config.c`,
`save.c`, `settings.c` all use them.

Loading is layered. You **seed a struct with defaults, then overlay** whatever the
file actually specifies, so a partial or missing file degrades gracefully:

```c
// The readers leave *out unchanged when the key is missing or the wrong type:
void lua_read_int_field(lua_State *L, const char *key, int *out);
```

`config_load` calls `config_set_defaults` first, then reads fields over the top.
`load_settings` in `game.c` seeds a `Settings` from the config values, then
`settings_load` overlays any persisted preferences — so the precedence
"built-in defaults < config.lua < saved settings" falls out naturally without a
single `if (present)` at the call sites. This shared reader idiom (five tiny
functions) is deliberately factored into `util/lua_util` so config, save, and
settings loaders don't each re-implement the "read a table field, tolerate absence"
dance.

Reusing the Lua interpreter as the *file format* for config, saves, and settings is
itself a pattern worth noting: the data files are `return { ... }` tables, and both
save and settings loaders open Lua **with no standard library** (`save.c:70`,
`settings.c`) so a shared/hand-edited file can't reach `os`/`io` — data, not code.

---

## 15. Robustness idiom: atomic write (temp file + rename)

**Look at:** `atomic_file.c`, used by `save.c` and `settings.c`.

Writing directly over a save file means a crash mid-write leaves a truncated,
corrupt save. The fix is a well-known systems idiom: **write to `path.tmp`, flush and
close it, then `rename` it over the target.** `rename` is atomic on POSIX, so the
file the game reads is *always* either the complete old version or the complete new
one — never a half-written mix. On any error the temp file is removed and the
original is left intact. Factored into one `atomic_write(path, contents)` so every
persistence path gets the guarantee for free. This has no OOP-specific analogue —
it's the kind of low-level durability concern that a runtime/ORM usually hides from
you, surfaced here because C hides nothing.

---

## 16. A note on the render loop: immediate mode (not retained widgets)

**Look at:** any `src/screen/*.c`, and `ui.c`'s `MenuNav`.

Coming from Swing/WPF/WinForms you expect **retained mode**: you construct `Button`
objects once, add them to a tree, wire event handlers, and the framework remembers
them. raygui is **immediate mode**: there are no widget objects. *Every frame* you
call `GuiButton(bounds, "SAVE")`, which draws the button *and* returns whether it was
clicked this frame:

```c
if (ui_menu_button(nav, 1, rect, "SAVE", true)) { action = PAUSE_SAVE; }
```

Nothing persists between frames except your own plain data. There's no button
instance to hold state, so the little bit of state a menu *does* need — which item is
focused for keyboard/gamepad — is kept explicitly in a `MenuNav` struct that the
screen threads through its `ui_menu_button` calls (`ui.h` documents the
begin/button.../end sequence). If a concept from retained-mode UI seems "missing,"
that's why: in immediate mode you *are* the retained state, and you keep it in a
struct like everything else.

---

## Cheat sheet: OOP construct → how this codebase spells it

| You'd reach for (Java/C#)        | Here it's…                                                        |
|----------------------------------|------------------------------------------------------------------|
| Class / package                  | A `.h`/`.c` pair in its own folder; functions prefixed `module_` |
| `public` members                 | Declared in the `.h`                                              |
| `private` methods                | `static` functions in the `.c`                                   |
| `private static` fields          | `static` file-scope variables (`s_` prefix)                      |
| `this`                           | First parameter, `Module *self`; fields via `->`                 |
| Constructor / destructor         | Hand-written `module_init` / `module_shutdown`, paired           |
| `readonly` / immutable param     | `const T *`                                                      |
| `try/catch` / exceptions         | `bool` return; check it at every call site                       |
| Multiple return / `(ok, value)`  | `bool` return + out-parameter pointer                            |
| `Optional<T>` / `T?`             | `bool`+out-param, or a negative/`0` sentinel                     |
| Returning a `string`             | Caller passes `char *out, int out_size`; callee `snprintf`s      |
| `List<T>` / `T[]` that grows     | Fixed array `T x[MAX]` + `int count`, with a clamp on `MAX`      |
| Singleton / `static class`       | A `.c` full of `static` state behind public functions            |
| Interface + DI                   | A header + per-platform `.c`, chosen in `xmake.lua` (link-time)  |
| Null object / graceful fallback  | Guarded no-op functions, or a swapped-in stub `.c`               |
| State pattern (subclasses)       | An `enum` + a `switch` in the update/draw loop                   |
| Command pattern                  | Screen returns an action `enum`; a `handle_*` applies it         |
| `InternalsVisibleTo` (tests)     | A `_internal.h` "friend" header + core-as-static-lib             |
| RAII / `using` / `IDisposable`   | Manual paired release + an ownership `bool` when acquisition can fail |
| `new` / heap by default          | Stack allocation by default; `malloc` is rare and locally freed  |

---

## What to keep doing when you add code

1. **New subsystem → new folder + `.h`/`.c`, functions prefixed with its name.**
   Public API in the header; everything else `static`.
2. **Pass state by pointer; make it `const` when you only read.** Don't reach for a
   global unless there's genuinely one of the thing (like the audio device).
3. **Fallible functions return `bool` and write results through out-params, leaving
   them untouched on failure.** Callers check the return.
4. **Pair every acquire with a release**, and track ownership with a flag when the
   acquire can fail.
5. **Screens report intent (an action enum); `game.c` owns the transitions.** Keep
   drawing and state-mutation on opposite sides of that line so the logic stays
   headless-testable.
6. **Validate/clamp external input at the boundary** (config, saves), and pair every
   `MAX` with the clamp that enforces it.
7. **Prefer fixed-capacity data over dynamic growth;** allocate on the stack or
   inside `Game`.
8. **Add a test by dropping `test/test_<name>.c` and appending `"<name>"` to the
   list in `xmake.lua`.** It links the same `vagrant-core` the game does.
</content>
</invoke>
