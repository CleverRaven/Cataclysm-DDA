#if defined(SDL_SOUND)

#include "cached_options.h"
#include "cata_catch.h"
#include "sdlsound.h"
#include "sound_backend.h"
#include "sounds.h"

namespace
{
bool predicate_true()
{
    return true;
}
bool predicate_false()
{
    return false;
}

static bool flag = false;
bool predicate_flag()
{
    return flag;
}
} // namespace

TEST_CASE( "sound_backend_slow_time_predicate_install_and_clear", "[sound_backend]" )
{
    sound_backend::set_slow_time_predicate( nullptr );
    CHECK_FALSE( sound_backend::slow_time_predicate_active() );

    sound_backend::set_slow_time_predicate( &predicate_true );
    CHECK( sound_backend::slow_time_predicate_active() );

    sound_backend::set_slow_time_predicate( &predicate_false );
    CHECK_FALSE( sound_backend::slow_time_predicate_active() );

    sound_backend::set_slow_time_predicate( nullptr );
    CHECK_FALSE( sound_backend::slow_time_predicate_active() );
}

TEST_CASE( "sound_backend_slow_time_predicate_reflects_caller_state", "[sound_backend]" )
{
    sound_backend::set_slow_time_predicate( &predicate_flag );

    flag = true;
    CHECK( sound_backend::slow_time_predicate_active() );

    flag = false;
    CHECK_FALSE( sound_backend::slow_time_predicate_active() );

    flag = true;
    CHECK( sound_backend::slow_time_predicate_active() );

    sound_backend::set_slow_time_predicate( nullptr );
}

TEST_CASE( "sound_backend_poll_is_safe_without_init", "[sound_backend]" )
{
    // poll() is called every frame from refresh_display(); it must be safe
    // to call regardless of whether backend::init has ever run.
    sound_backend::poll();
    sound_backend::poll();
    SUCCEED();
}

#if defined(USE_SDL3)
TEST_CASE( "sound_backend_slow_time_drives_reserved_track_frequency_ratio",
           "[sound_backend][slow_time]" )
{
    // Memory-mixer init so the test does not touch an audio device.
    REQUIRE( sound_backend::init( 44100, 2, 1024,
                                  sound_backend::init_options{ /*memory_only=*/true } ) );

    flag = false;
    sound_backend::set_slow_time_predicate( &predicate_flag );

    // poll() must leave the ratio at unity when the predicate is false.
    sound_backend::poll();
    CHECK( sound_backend::testing::reserved_track_frequency_ratio(
               sfx::channel::daytime_outdoors_env ) == Approx( 1.0f ) );

    // Flipping the predicate and polling must scale every reserved
    // SFX track down to the slow-time factor.
    flag = true;
    sound_backend::poll();
    const float slowed = sound_backend::testing::reserved_track_frequency_ratio(
                             sfx::channel::daytime_outdoors_env );
    CHECK( slowed < 1.0f );
    CHECK( slowed > 0.0f );

    // Flipping back restores unity on the next poll.
    flag = false;
    sound_backend::poll();
    CHECK( sound_backend::testing::reserved_track_frequency_ratio(
               sfx::channel::daytime_outdoors_env ) == Approx( 1.0f ) );

    sound_backend::set_slow_time_predicate( nullptr );
    sound_backend::shutdown();
}

TEST_CASE( "sound_backend_slow_time_composes_with_play_opts_pitch",
           "[sound_backend][slow_time]" )
{
    REQUIRE( sound_backend::init( 44100, 2, 1024,
                                  sound_backend::init_options{ /*memory_only=*/true } ) );

    flag = false;
    sound_backend::set_slow_time_predicate( &predicate_flag );

    // Play a pitched sound on a reserved slot. Backend must retain the
    // caller's pitch multiplier across slow-time toggles rather than
    // overwriting it to slow-factor-only. play_opts::pitch follows the
    // do_pitch_shift convention (smaller = higher perceived pitch);
    // MIX_SetTrackFrequencyRatio is native (larger = higher pitch), so
    // the backend inverts at the boundary.
    sound_backend::sfx_audio *a = sound_backend::testing::make_synthetic_sfx_audio();
    REQUIRE( a != nullptr );
    const sfx::channel slot = sfx::channel::daytime_outdoors_env;
    sound_backend::play_opts opts;
    opts.pitch = 0.8f;
    sound_backend::play_reserved( a, slot, opts );

    sound_backend::poll();
    CHECK( sound_backend::testing::reserved_track_frequency_ratio( slot ) ==
           Approx( 1.0f / 0.8f ) );

    flag = true;
    sound_backend::poll();
    const float reserved_ratio = sound_backend::testing::reserved_track_frequency_ratio( slot );
    CHECK( reserved_ratio < 1.0f / 0.8f );
    CHECK( reserved_ratio > 0.0f );
    CHECK( reserved_ratio == Approx( ( 1.0f / 0.8f ) * 0.25f ).margin( 0.001f ) );

    flag = false;
    sound_backend::poll();
    CHECK( sound_backend::testing::reserved_track_frequency_ratio( slot ) ==
           Approx( 1.0f / 0.8f ) );

    // One-shot path: pitched play must also retain its multiplier under
    // slow-time.
    sound_backend::play_opts oneshot_opts;
    oneshot_opts.pitch = 0.5f;
    sound_backend::play_oneshot( a, oneshot_opts );

    sound_backend::poll();
    CHECK( sound_backend::testing::last_oneshot_frequency_ratio() ==
           Approx( 1.0f / 0.5f ) );

    flag = true;
    sound_backend::poll();
    const float oneshot_ratio = sound_backend::testing::last_oneshot_frequency_ratio();
    CHECK( oneshot_ratio == Approx( ( 1.0f / 0.5f ) * 0.25f ).margin( 0.001f ) );

    // New play started while slow-time already active must start
    // slowed, not wait for the next poll.
    flag = true;
    sound_backend::play_opts spawned_opts;
    spawned_opts.pitch = 1.0f;
    sound_backend::play_oneshot( a, spawned_opts );
    CHECK( sound_backend::testing::last_oneshot_frequency_ratio() ==
           Approx( 0.25f ).margin( 0.001f ) );

    sound_backend::free_sfx( a );
    sound_backend::set_slow_time_predicate( nullptr );
    sound_backend::shutdown();
}
#endif // USE_SDL3

#if defined(USE_SDL3)
TEST_CASE( "sfx_wrapper_routes_is_channel_playing_through_backend",
           "[sound_backend][integration]" )
{
    // RAII guard: unblock the test_mode / sound_enabled / sound_init_success
    // early-returns in sfx:: and sdlsound wrappers for the duration
    // of the test, then restore.
    const bool prev_test_mode = test_mode;
    const bool prev_sound_enabled = sounds::sound_enabled;
    const bool prev_init_success = sound_init_success;
    test_mode = false;
    sounds::sound_enabled = true;
    sound_init_success = true;

    REQUIRE( sound_backend::init( 44100, 2, 1024,
                                  sound_backend::init_options{ /*memory_only=*/true } ) );

    const sfx::channel slot = sfx::channel::daytime_outdoors_env;

    // No play yet: the sfx:: wrapper should route to backend and
    // report not playing.
    CHECK_FALSE( sfx::is_channel_playing( slot ) );

    // Start a synthetic play through the backend so the reserved
    // track enters the playing state; the sfx:: wrapper should
    // observe the same state via the backend.
    sound_backend::sfx_audio *a = sound_backend::testing::make_synthetic_sfx_audio();
    REQUIRE( a != nullptr );
    sound_backend::play_reserved( a, slot, sound_backend::play_opts{} );
    CHECK( sfx::is_channel_playing( slot ) );

    // fade_audio_channel with the sentinel routes through backend
    // without crashing; exercises the channel::any path explicitly.
    sfx::fade_audio_channel( sfx::channel::any, 50 );

    sound_backend::free_sfx( a );
    sound_backend::shutdown();

    test_mode = prev_test_mode;
    sounds::sound_enabled = prev_sound_enabled;
    sound_init_success = prev_init_success;
}
#endif // USE_SDL3

#endif // SDL_SOUND
