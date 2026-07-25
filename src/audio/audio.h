#ifndef VR_AUDIO_AUDIO_H
#define VR_AUDIO_AUDIO_H

#include <stdbool.h>

#include "config/config.h" // AudioConfig

// Game audio: one looping background-music stream plus short UI sound effects,
// wrapping raylib's audio device. All state is module-global (one game, one
// device), mirroring how the rest of the shell is wired. Every entry point is a
// no-op when the audio device failed to open (e.g. headless), so callers never
// need to guard.

// Open the audio device and load music + the nav sound from `cfg`. Music starts
// playing immediately (looping). Missing/empty music plays nothing; a missing
// nav sound is replaced by a synthesized thud. Safe to call once at startup.
void audio_init(const AudioConfig *cfg);

// Release sounds/music and close the audio device. Safe to call once at shutdown.
void audio_shutdown(void);

// Pump the music stream. Call once per frame.
void audio_update(void);

// Play the menu focus-move "thud". Called on every focus change.
void audio_play_nav(void);

// Play the menu activate/confirm sound. Called when an item is selected.
void audio_play_select(void);

// --- Mixer controls (for the settings screen) --------------------------------
// Volumes are 0..1 linear gain. Muting preserves the stored volumes so the
// prior levels return on unmute.
void audio_set_music_volume(float volume);
void audio_set_sfx_volume(float volume);
void audio_set_muted(bool muted);

float audio_music_volume(void);
float audio_sfx_volume(void);
bool audio_muted(void);

#endif // VR_AUDIO_AUDIO_H
