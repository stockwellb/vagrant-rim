// Unit tests for the audio mixer STATE (audio/audio) — the volume/mute logic the
// settings screen drives, exercised without an audio device. The setters store
// clamped volumes and a mute flag in module globals; the getters read them back.
// None of this needs InitAudioDevice: every device-touching call inside the
// setters is gated on an "audio ready" flag that stays false when no device was
// opened, so the stored state is all that moves here. We deliberately do NOT call
// audio_init (which would open a real device / synthesize sounds) — only the pure
// mixer state is under test.
#include "unity.h"

#include "audio/audio.h"

void setUp(void) {}
void tearDown(void) {}

// --- Volume set/get round-trip ----------------------------------------------

static void test_music_volume_round_trips(void)
{
    audio_set_music_volume(0.3f);
    TEST_ASSERT_EQUAL_FLOAT(0.3f, audio_music_volume());
}

static void test_sfx_volume_round_trips(void)
{
    audio_set_sfx_volume(0.8f);
    TEST_ASSERT_EQUAL_FLOAT(0.8f, audio_sfx_volume());
}

// --- Clamping to [0, 1] -----------------------------------------------------

static void test_music_volume_clamps_high(void)
{
    audio_set_music_volume(1.5f);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, audio_music_volume());
}

static void test_music_volume_clamps_low(void)
{
    audio_set_music_volume(-0.2f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, audio_music_volume());
}

static void test_sfx_volume_clamps_high(void)
{
    audio_set_sfx_volume(3.0f);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, audio_sfx_volume());
}

static void test_sfx_volume_clamps_low(void)
{
    audio_set_sfx_volume(-1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, audio_sfx_volume());
}

// --- Mute preserves the stored volumes --------------------------------------

static void test_mute_preserves_volumes_and_restores_on_unmute(void)
{
    audio_set_music_volume(0.4f);
    audio_set_sfx_volume(0.6f);

    audio_set_muted(true);
    TEST_ASSERT_TRUE(audio_muted());
    // Muting zeroes the applied gain but must not disturb the stored volumes.
    TEST_ASSERT_EQUAL_FLOAT(0.4f, audio_music_volume());
    TEST_ASSERT_EQUAL_FLOAT(0.6f, audio_sfx_volume());

    audio_set_muted(false);
    TEST_ASSERT_FALSE(audio_muted());
    TEST_ASSERT_EQUAL_FLOAT(0.4f, audio_music_volume());
    TEST_ASSERT_EQUAL_FLOAT(0.6f, audio_sfx_volume());
}

static void test_volume_can_change_while_muted(void)
{
    audio_set_muted(true);
    audio_set_music_volume(0.25f); // adjusting the slider while muted still stores
    TEST_ASSERT_EQUAL_FLOAT(0.25f, audio_music_volume());
    TEST_ASSERT_TRUE(audio_muted()); // still muted
    audio_set_muted(false);          // and the new level is what returns on unmute
    TEST_ASSERT_EQUAL_FLOAT(0.25f, audio_music_volume());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_music_volume_round_trips);
    RUN_TEST(test_sfx_volume_round_trips);
    RUN_TEST(test_music_volume_clamps_high);
    RUN_TEST(test_music_volume_clamps_low);
    RUN_TEST(test_sfx_volume_clamps_high);
    RUN_TEST(test_sfx_volume_clamps_low);
    RUN_TEST(test_mute_preserves_volumes_and_restores_on_unmute);
    RUN_TEST(test_volume_can_change_while_muted);
    return UNITY_END();
}
