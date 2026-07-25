#include "audio/audio.h"

#include <math.h>
#include <stdlib.h>

#include "raylib.h"

#include "util/assets.h"
#include "util/mathx.h"

// One audio device, one music stream, one nav sound — module-global like the
// rest of the shell. `s_ready` gates every call so a failed device (headless CI,
// no output) turns the whole module into a no-op instead of crashing.
static bool s_ready;
static Music s_music;
static bool s_music_loaded;
static Sound s_nav;
static bool s_nav_loaded;
static Sound s_select;
static bool s_select_loaded;

static float s_music_volume = 0.5f;
static float s_sfx_volume = 0.7f;
static bool s_muted;

// Muting zeroes the gain applied to raylib without disturbing the stored
// volumes, so unmuting restores the prior levels.
static float music_gain(void) { return s_muted ? 0.0f : s_music_volume; }
static float sfx_gain(void) { return s_muted ? 0.0f : s_sfx_volume; }

// Push the current gains to raylib. Split out so every volume/mute change (and
// init) applies them from one place instead of repeating the SetSoundVolume /
// SetMusicVolume sequence.
static void apply_sfx_gain(void)
{
    if (!s_ready) return;
    if (s_nav_loaded) SetSoundVolume(s_nav, sfx_gain());
    if (s_select_loaded) SetSoundVolume(s_select, sfx_gain());
}
static void apply_music_gain(void)
{
    if (s_ready && s_music_loaded) SetMusicVolume(s_music, music_gain());
}

// Synthesize a short damped-sine "click" so UI sounds always have audible
// feedback even before any asset is supplied. `freq` sets the pitch (a low tone
// reads as a thud, a higher one as a confirm) and `decay` how fast it fades.
static Sound synth_click(float freq, float decay)
{
    const int rate = 44100;
    const int frames = rate * 12 / 100; // 0.12 s
    short *samples = (short *)malloc((size_t)frames * sizeof(short));
    if (!samples) {
        TraceLog(LOG_WARNING, "AUDIO: click synth allocation failed — sound will be silent");
        return (Sound){ 0 }; // frameCount 0; callers gate playback on the loaded flag
    }

    for (int i = 0; i < frames; i++) {
        float t = (float)i / (float)rate;
        float env = expf(-t * decay);
        float body = sinf(2.0f * PI * freq * t);
        samples[i] = (short)(body * env * 30000.0f);
    }

    Wave wave = {
        .frameCount = (unsigned int)frames,
        .sampleRate = (unsigned int)rate,
        .sampleSize = 16,
        .channels = 1,
        .data = samples,
    };
    Sound snd = LoadSoundFromWave(wave); // copies samples into an audio buffer
    free(samples);
    return snd;
}

// Load a UI sound from `file` if it resolves, else synthesize one with (freq,
// decay) so the effect is never silent. `label` names the sound in logs.
static Sound load_ui_sound(const char *file, float freq, float decay, const char *label)
{
    char path[512];
    if (file[0] != '\0' && asset_resolve(file, path, sizeof(path))) {
        Sound snd = LoadSound(path);
        if (snd.frameCount > 0) {
            TraceLog(LOG_INFO, "AUDIO: %s sound '%s'", label, path);
            return snd;
        }
        UnloadSound(snd); // release the failed (empty) handle before falling back
        TraceLog(LOG_WARNING, "AUDIO: %s sound '%s' failed — using synthesized fallback",
                 label, path);
        return synth_click(freq, decay);
    }
    TraceLog(LOG_INFO, "AUDIO: %s sound synthesized (no asset)", label);
    return synth_click(freq, decay);
}

void audio_init(const AudioConfig *cfg)
{
    s_music_volume = clamp01(cfg->music_volume);
    s_sfx_volume = clamp01(cfg->sfx_volume);
    s_muted = cfg->muted;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        TraceLog(LOG_WARNING, "AUDIO: no audio device available — running silent");
        s_ready = false;
        return;
    }
    s_ready = true;

    char path[512];

    // Background music is optional: play it if a file resolves, otherwise stay
    // silent. A resolved-but-unloadable file (frameCount 0) is a real error.
    if (cfg->music_file[0] != '\0' && asset_resolve(cfg->music_file, path, sizeof(path))) {
        s_music = LoadMusicStream(path);
        if (s_music.frameCount > 0) {
            s_music_loaded = true;
            s_music.looping = true;
            SetMusicVolume(s_music, music_gain());
            PlayMusicStream(s_music);
            TraceLog(LOG_INFO, "AUDIO: playing music '%s'", path);
        } else {
            TraceLog(LOG_WARNING, "AUDIO: failed to load music '%s'", path);
        }
    } else {
        TraceLog(LOG_INFO, "AUDIO: no music file found — background music disabled");
    }

    // UI sounds: focus-move is a low thud, activate a higher confirm. Both load
    // from asset if present, otherwise synthesize so the UI is never silent. The
    // loaded flag tracks frameCount so a failed load/synth (e.g. OOM) leaves an
    // empty handle that playback simply skips rather than passing to PlaySound.
    s_nav = load_ui_sound(cfg->nav_sfx_file, 90.0f, 30.0f, "focus");
    s_nav_loaded = s_nav.frameCount > 0;

    s_select = load_ui_sound(cfg->select_sfx_file, 220.0f, 26.0f, "select");
    s_select_loaded = s_select.frameCount > 0;

    apply_sfx_gain();
}

void audio_shutdown(void)
{
    if (!s_ready) {
        return;
    }
    // Let a just-triggered confirm sound finish before we tear the device down,
    // so quitting from a menu (Exit plays select, then the app shuts down the
    // same frame) isn't silent. Bounded so a stuck sound can't hang shutdown.
    if (s_select_loaded) {
        for (int i = 0; i < 30 && IsSoundPlaying(s_select); i++) {
            WaitTime(0.01);
        }
    }
    if (s_music_loaded) {
        StopMusicStream(s_music);
        UnloadMusicStream(s_music);
        s_music_loaded = false;
    }
    if (s_nav_loaded) {
        UnloadSound(s_nav);
        s_nav_loaded = false;
    }
    if (s_select_loaded) {
        UnloadSound(s_select);
        s_select_loaded = false;
    }
    CloseAudioDevice();
    s_ready = false;
}

void audio_update(void)
{
    if (s_ready && s_music_loaded) {
        UpdateMusicStream(s_music); // refill the streaming buffers each frame
    }
}

void audio_play_nav(void)
{
    if (s_ready && s_nav_loaded) {
        PlaySound(s_nav);
    }
}

void audio_play_select(void)
{
    if (s_ready && s_select_loaded) {
        PlaySound(s_select);
    }
}

void audio_set_music_volume(float volume)
{
    s_music_volume = clamp01(volume);
    apply_music_gain();
}

void audio_set_sfx_volume(float volume)
{
    s_sfx_volume = clamp01(volume);
    apply_sfx_gain();
}

void audio_set_muted(bool muted)
{
    s_muted = muted;
    apply_music_gain();
    apply_sfx_gain();
    // Muting should be instant: silence any sfx already in flight (e.g. the
    // confirm click from the very button press that toggled mute) rather than
    // letting it ring out at the pre-mute gain.
    if (muted && s_ready) {
        if (s_nav_loaded) StopSound(s_nav);
        if (s_select_loaded) StopSound(s_select);
    }
}

float audio_music_volume(void) { return s_music_volume; }
float audio_sfx_volume(void) { return s_sfx_volume; }
bool audio_muted(void) { return s_muted; }
