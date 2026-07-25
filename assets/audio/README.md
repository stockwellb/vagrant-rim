# Audio assets

Drop game audio here. Paths are set in `assets/config.lua` under the `audio`
table and resolve against the asset search paths, so the defaults expect files
in this folder.

| File         | Purpose                          | If missing                        |
|--------------|----------------------------------|-----------------------------------|
| `music.ogg`  | Looping background music         | Game runs with no music           |
| `focus.ogg`  | Menu focus-move sound            | A thud is synthesized in code     |
| `select.ogg` | Menu activate/confirm sound      | A confirm tone is synthesized     |

Supported formats (raylib): `.ogg`, `.mp3`, `.wav`, `.flac`, `.xm`, `.mod`.
`.ogg` is a good default for music (small, loops cleanly); `.wav` for short SFX.

Volumes and mute also live in the `audio` table and are adjustable at runtime
through `src/audio/audio.h` (the forthcoming Settings screen drives these).
