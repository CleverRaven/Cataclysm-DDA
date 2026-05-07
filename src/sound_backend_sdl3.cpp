#if defined(SDL_SOUND) && defined(USE_SDL3)

#include "sound_backend.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

#include "debug.h"

#define dbg(x) DebugLog((x), D_SDL) << __FILE__ << ":" << __LINE__ << ": "

namespace sound_backend
{

struct sfx_audio {
    MIX_Audio *audio = nullptr;
};

struct music_source {
    MIX_Audio *audio = nullptr;
};

namespace
{
MIX_Mixer *g_mixer = nullptr;
MIX_Track *g_music_track = nullptr;
bool_predicate g_slow_time_predicate = nullptr;
music_finished_cb g_music_finished_cb = nullptr;

// SFX_TAG is applied to every SFX track so MIX_StopTag(mixer, SFX_TAG, ...)
// affects all SFX without touching music.
constexpr const char *SFX_TAG = "sfx";

constexpr float SLOW_TIME_FACTOR = 0.25f;

std::array<MIX_Track *, static_cast<size_t>( sfx::channel::MAX_CHANNEL )> g_reserved_tracks{};
std::array<sfx::group, static_cast<size_t>( sfx::channel::MAX_CHANNEL )> g_reserved_groups{};
std::array<float, static_cast<size_t>( sfx::channel::MAX_CHANNEL )> g_reserved_base_pitch{};
std::array<float, static_cast<size_t>( sfx::channel::MAX_CHANNEL )> g_reserved_base_gain{};
std::array<float, static_cast<size_t>( sfx::channel::MAX_CHANNEL )> g_reserved_distance_scale{};

// One-shot tracks are created per play. The stopped callback fires
// on the mixer's audio thread while the mixer is still iterating
// internal track state, so destroying the track from inside that
// callback is undefined behavior. Instead the callback marks the
// entry as stopped; poll() reaps (MIX_DestroyTrack + vector erase)
// on the main thread.
//
// base_pitch is the caller's per-play pitch so poll() can recompose
// pitch with the current slow-time factor without losing it.
struct oneshot_entry {
    MIX_Track *track;
    float base_pitch;
    bool stopped = false;
};
std::mutex g_oneshots_mutex;
std::vector<oneshot_entry> g_oneshots;

// Music generation token: bump on every play_music / stop_music so
// superseded stopped callbacks no-op.
std::atomic<uint32_t> g_music_generation{0};

const char *tag_for_group( sfx::group g )
{
    switch( g ) {
        case sfx::group::weather:
            return "weather";
        case sfx::group::time_of_day:
            return "time_of_day";
        case sfx::group::context_themes:
            return "context_themes";
        case sfx::group::low_stamina:
            return "low_stamina";
    }
    return nullptr;
}

MIX_Track *reserved_track( sfx::channel slot )
{
    const int idx = static_cast<int>( slot );
    if( idx < 0 || idx >= static_cast<int>( sfx::channel::MAX_CHANNEL ) ) {
        return nullptr;
    }
    return g_reserved_tracks[idx];
}

void oneshot_stopped_cb( void *userdata, MIX_Track *track )
{
    ( void )userdata;
    // Do NOT destroy the track here. SDL3_mixer invokes this callback
    // while still holding internal references; MIX_DestroyTrack from
    // inside the callback is UB. Mark the entry stopped and let poll()
    // reap on the main thread.
    std::lock_guard<std::mutex> lock( g_oneshots_mutex );
    for( oneshot_entry &e : g_oneshots ) {
        if( e.track == track ) {
            e.stopped = true;
            return;
        }
    }
}

void music_stopped_cb( void *userdata, MIX_Track * /*track*/ )
{
    const uint32_t captured = static_cast<uint32_t>( reinterpret_cast<uintptr_t>( userdata ) );
    if( captured != g_music_generation.load() ) {
        return;
    }
    if( g_music_finished_cb != nullptr ) {
        g_music_finished_cb();
    }
}

float volume_to_gain( int vol )
{
    if( vol <= 0 ) {
        return 0.0f;
    }
    if( vol >= 128 ) {
        return 1.0f;
    }
    return static_cast<float>( vol ) / 128.0f;
}

// SDL2 Mix_SetPosition distance semantics: 0 = closest (max volume),
// 255 = farthest (muted). Linear attenuation.
float distance_to_scale( int distance )
{
    if( distance <= 0 ) {
        return 1.0f;
    }
    if( distance >= 255 ) {
        return 0.0f;
    }
    return 1.0f - static_cast<float>( distance ) / 255.0f;
}

void apply_pan( MIX_Track *track, int angle_deg )
{
    // Equal-power pan law.
    const float pan_x = std::sin( static_cast<float>( angle_deg ) * 3.14159265f / 180.0f );
    MIX_StereoGains gains;
    gains.left  = std::sqrt( 0.5f * ( 1.0f - pan_x ) );
    gains.right = std::sqrt( 0.5f * ( 1.0f + pan_x ) );
    MIX_SetTrackStereo( track, &gains );
}
} // namespace

bool init( int frequency, int out_channels, int chunk_size,
           const init_options &opts )
{
    if( !MIX_Init() ) {
        dbg( D_ERROR ) << "MIX_Init failed: " << SDL_GetError();
        return false;
    }

    SDL_AudioSpec spec{};
    spec.freq = frequency;
    spec.format = SDL_AUDIO_S16;
    spec.channels = out_channels;

    if( opts.memory_only ) {
        g_mixer = MIX_CreateMixer( &spec );
    } else {
        g_mixer = MIX_CreateMixerDevice( SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec );
    }
    ( void )chunk_size;
    if( g_mixer == nullptr ) {
        dbg( D_ERROR ) << "MIX_CreateMixer failed: " << SDL_GetError();
        MIX_Quit();
        return false;
    }

    for( size_t i = 0; i < g_reserved_tracks.size(); ++i ) {
        g_reserved_tracks[i] = MIX_CreateTrack( g_mixer );
        if( g_reserved_tracks[i] != nullptr ) {
            MIX_TagTrack( g_reserved_tracks[i], SFX_TAG );
        }
        g_reserved_base_pitch[i] = 1.0f;
        g_reserved_base_gain[i] = 1.0f;
        g_reserved_distance_scale[i] = 1.0f;
    }

    g_music_track = MIX_CreateTrack( g_mixer );
    return true;
}

void shutdown()
{
    for( MIX_Track *t : g_reserved_tracks ) {
        if( t != nullptr ) {
            MIX_DestroyTrack( t );
        }
    }
    g_reserved_tracks.fill( nullptr );

    {
        std::lock_guard<std::mutex> lock( g_oneshots_mutex );
        for( const oneshot_entry &o : g_oneshots ) {
            MIX_DestroyTrack( o.track );
        }
        g_oneshots.clear();
    }

    if( g_music_track != nullptr ) {
        MIX_DestroyTrack( g_music_track );
        g_music_track = nullptr;
    }
    if( g_mixer != nullptr ) {
        MIX_DestroyMixer( g_mixer );
        g_mixer = nullptr;
    }
    MIX_Quit();
}

sfx_audio *load_sfx( const std::string &path )
{
    sfx_audio *a = new sfx_audio{};
    if( g_mixer == nullptr ) {
        return a;
    }
    a->audio = MIX_LoadAudio( g_mixer, path.c_str(), true );
    if( a->audio == nullptr ) {
        dbg( D_WARNING ) << "Failed to load sfx audio " << path << ": " << SDL_GetError();
    }
    return a;
}

void free_sfx( sfx_audio *a )
{
    if( a == nullptr ) {
        return;
    }
    if( a->audio != nullptr ) {
        MIX_DestroyAudio( a->audio );
    }
    delete a;
}

music_source *load_music( const std::string &path )
{
    if( g_mixer == nullptr ) {
        return nullptr;
    }
    MIX_Audio *audio = MIX_LoadAudio( g_mixer, path.c_str(), false );
    if( audio == nullptr ) {
        dbg( D_WARNING ) << "Failed to load music " << path << ": " << SDL_GetError();
        return nullptr;
    }
    music_source *m = new music_source{};
    m->audio = audio;
    return m;
}

void free_music( music_source *m )
{
    if( m == nullptr ) {
        return;
    }
    if( m->audio != nullptr ) {
        MIX_DestroyAudio( m->audio );
    }
    delete m;
}

namespace
{
void configure_and_play( MIX_Track *track, const play_opts &opts )
{
    const float base = volume_to_gain( opts.volume );
    const float scale = opts.positional ? distance_to_scale( opts.distance ) : 1.0f;
    MIX_SetTrackGain( track, base * scale );
    if( opts.positional ) {
        apply_pan( track, opts.angle_deg );
    }
    // Compose the caller's pitch with the current slow-time factor so
    // sounds spawned while bullet-time is active start slowed, rather
    // than waiting for the next poll. Caller's pitch follows the
    // do_pitch_shift convention (smaller = higher perceived pitch,
    // produces a shorter chunk), which is the inverse of a native
    // frequency ratio; invert here.
    const float slow = slow_time_predicate_active() ? SLOW_TIME_FACTOR : 1.0f;
    MIX_SetTrackFrequencyRatio( track, ( 1.0f / opts.pitch ) * slow );

    SDL_PropertiesID props = SDL_CreateProperties();
    if( opts.loops != 0 ) {
        SDL_SetNumberProperty( props, MIX_PROP_PLAY_LOOPS_NUMBER, opts.loops );
    }
    if( opts.fade_in_ms > 0 ) {
        SDL_SetNumberProperty( props, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, opts.fade_in_ms );
    }
    MIX_PlayTrack( track, props );
    SDL_DestroyProperties( props );
}
} // namespace

void play_reserved( sfx_audio *a, sfx::channel slot, const play_opts &opts )
{
    if( a == nullptr || a->audio == nullptr ) {
        return;
    }
    MIX_Track *track = reserved_track( slot );
    if( track == nullptr ) {
        return;
    }

    MIX_SetTrackAudio( track, a->audio );
    const size_t idx = static_cast<size_t>( slot );
    g_reserved_base_pitch[idx] = opts.pitch;
    g_reserved_base_gain[idx] = volume_to_gain( opts.volume );
    g_reserved_distance_scale[idx] = opts.positional
                                     ? distance_to_scale( opts.distance )
                                     : 1.0f;
    configure_and_play( track, opts );
}

void play_oneshot( sfx_audio *a, const play_opts &opts )
{
    if( a == nullptr || a->audio == nullptr || g_mixer == nullptr ) {
        return;
    }
    MIX_Track *track = MIX_CreateTrack( g_mixer );
    if( track == nullptr ) {
        dbg( D_WARNING ) << "MIX_CreateTrack failed for one-shot: " << SDL_GetError();
        return;
    }
    MIX_TagTrack( track, SFX_TAG );
    MIX_SetTrackAudio( track, a->audio );
    MIX_SetTrackStoppedCallback( track, oneshot_stopped_cb, nullptr );
    {
        std::lock_guard<std::mutex> lock( g_oneshots_mutex );
        g_oneshots.push_back( { track, opts.pitch } );
    }
    configure_and_play( track, opts );
}

void tag_reserved_groups( const std::array<sfx::group,
                          static_cast<size_t>( sfx::channel::MAX_CHANNEL )> &assignments )
{
    for( size_t i = 0; i < assignments.size(); ++i ) {
        g_reserved_groups[i] = assignments[i];
        MIX_Track *track = g_reserved_tracks[i];
        if( track == nullptr ) {
            continue;
        }
        const char *g_tag = tag_for_group( assignments[i] );
        if( g_tag != nullptr ) {
            MIX_TagTrack( track, g_tag );
        }
    }
}

void stop_reserved( sfx::channel slot, int fade_out_ms )
{
    MIX_Track *track = reserved_track( slot );
    if( track == nullptr ) {
        return;
    }
    if( fade_out_ms <= 0 ) {
        MIX_StopTrack( track, 0 );
    } else {
        // MIX_StopTrack is not fade-idempotent: each call re-arms the
        // fade countdown. Callers (e.g. sfx::do_ambient) invoke this
        // on every tick while the sound is still audible, so the guard
        // below lets an in-progress fade finish on its original
        // schedule.
        if( MIX_GetTrackFadeFrames( track ) != 0 ) {
            return;
        }
        MIX_StopTrack( track, MIX_TrackMSToFrames( track, fade_out_ms ) );
    }
}

bool is_reserved_playing( sfx::channel slot )
{
    MIX_Track *track = reserved_track( slot );
    return track != nullptr && MIX_TrackPlaying( track );
}

bool is_reserved_fading( sfx::channel slot )
{
    MIX_Track *track = reserved_track( slot );
    return track != nullptr && MIX_GetTrackFadeFrames( track ) != 0;
}

int set_reserved_volume( sfx::channel slot, int vol )
{
    MIX_Track *track = reserved_track( slot );
    if( track == nullptr ) {
        return -1;
    }
    if( !MIX_TrackPlaying( track ) ) {
        return -1;
    }
    if( MIX_GetTrackFadeFrames( track ) != 0 ) {
        return -1;
    }
    const size_t idx = static_cast<size_t>( slot );
    const float prev_base = g_reserved_base_gain[idx];
    g_reserved_base_gain[idx] = volume_to_gain( vol );
    MIX_SetTrackGain( track, g_reserved_base_gain[idx] * g_reserved_distance_scale[idx] );
    return static_cast<int>( prev_base * 128.0f );
}

int get_reserved_volume( sfx::channel slot )
{
    const int idx = static_cast<int>( slot );
    if( idx < 0 || idx >= static_cast<int>( sfx::channel::MAX_CHANNEL ) ) {
        return 0;
    }
    return static_cast<int>( g_reserved_base_gain[idx] * 128.0f );
}

void set_reserved_position( sfx::channel slot, int angle_deg, int distance )
{
    MIX_Track *track = reserved_track( slot );
    if( track == nullptr ) {
        return;
    }
    apply_pan( track, angle_deg );
    const size_t idx = static_cast<size_t>( slot );
    g_reserved_distance_scale[idx] = distance_to_scale( distance );
    MIX_SetTrackGain( track, g_reserved_base_gain[idx] * g_reserved_distance_scale[idx] );
}

void set_slow_time_predicate( bool_predicate fn )
{
    g_slow_time_predicate = fn;
}

bool slow_time_predicate_active()
{
    return g_slow_time_predicate != nullptr && g_slow_time_predicate();
}

namespace testing
{
float reserved_track_frequency_ratio( sfx::channel slot )
{
    MIX_Track *track = reserved_track( slot );
    if( track == nullptr ) {
        return 0.0f;
    }
    return MIX_GetTrackFrequencyRatio( track );
}
float last_oneshot_frequency_ratio()
{
    std::lock_guard<std::mutex> lock( g_oneshots_mutex );
    if( g_oneshots.empty() ) {
        return 0.0f;
    }
    return MIX_GetTrackFrequencyRatio( g_oneshots.back().track );
}
sfx_audio *make_synthetic_sfx_audio()
{
    if( g_mixer == nullptr ) {
        return nullptr;
    }
    // One frame of silent stereo float32 PCM.
    static const float silent_frame[2] = { 0.0f, 0.0f };
    SDL_AudioSpec spec{};
    spec.freq = 44100;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    MIX_Audio *audio = MIX_LoadRawAudio( g_mixer, silent_frame,
                                         sizeof( silent_frame ), &spec );
    if( audio == nullptr ) {
        return nullptr;
    }
    sfx_audio *a = new sfx_audio{};
    a->audio = audio;
    return a;
}
} // namespace testing

void stop_all_sfx( int fade_out_ms )
{
    if( g_mixer == nullptr ) {
        return;
    }
    MIX_StopTag( g_mixer, SFX_TAG, fade_out_ms );
}

void fade_group( sfx::group g, int ms )
{
    if( g_mixer == nullptr ) {
        return;
    }
    // Reserved tracks are reused across plays, so per-slot
    // MIX_StopTrack is the authoritative way to stop them even though
    // they also carry the group tag. The tag-based MIX_StopTag path is
    // unreliable on reused tracks in this backend.
    for( size_t i = 0; i < g_reserved_tracks.size(); ++i ) {
        if( g_reserved_groups[i] != g ) {
            continue;
        }
        MIX_Track *t = g_reserved_tracks[i];
        if( t == nullptr ) {
            continue;
        }
        if( ms <= 0 ) {
            MIX_StopTrack( t, 0 );
        } else {
            // Skip if already fading; re-issuing MIX_StopTrack resets
            // the fade countdown on SDL3 and would stall the fade.
            if( MIX_GetTrackFadeFrames( t ) != 0 ) {
                continue;
            }
            MIX_StopTrack( t, MIX_TrackMSToFrames( t, ms ) );
        }
    }
}

void poll()
{
    const float slow = slow_time_predicate_active() ? SLOW_TIME_FACTOR : 1.0f;
    for( size_t i = 0; i < g_reserved_tracks.size(); ++i ) {
        MIX_Track *t = g_reserved_tracks[i];
        if( t != nullptr ) {
            MIX_SetTrackFrequencyRatio( t, ( 1.0f / g_reserved_base_pitch[i] ) * slow );
        }
    }
    std::lock_guard<std::mutex> lock( g_oneshots_mutex );
    // Reap stopped one-shots: destroy tracks the stopped callback
    // flagged, then drop them from the vector. Running one-shots
    // get their frequency ratio refreshed below.
    for( auto it = g_oneshots.begin(); it != g_oneshots.end(); ) {
        if( it->stopped ) {
            MIX_DestroyTrack( it->track );
            it = g_oneshots.erase( it );
        } else {
            MIX_SetTrackFrequencyRatio( it->track, ( 1.0f / it->base_pitch ) * slow );
            ++it;
        }
    }
}

void set_music_finished_cb( music_finished_cb cb )
{
    g_music_finished_cb = cb;
}

bool play_music( music_source *m, int loops, int fade_in_ms )
{
    if( m == nullptr || m->audio == nullptr || g_music_track == nullptr ) {
        return false;
    }
    const uint32_t new_gen = ++g_music_generation;
    MIX_SetTrackAudio( g_music_track, m->audio );
    MIX_SetTrackStoppedCallback( g_music_track, music_stopped_cb,
                                 reinterpret_cast<void *>( static_cast<uintptr_t>( new_gen ) ) );

    SDL_PropertiesID props = SDL_CreateProperties();
    if( loops != 0 ) {
        SDL_SetNumberProperty( props, MIX_PROP_PLAY_LOOPS_NUMBER, loops );
    }
    if( fade_in_ms > 0 ) {
        SDL_SetNumberProperty( props, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, fade_in_ms );
    }
    const bool ok = MIX_PlayTrack( g_music_track, props );
    SDL_DestroyProperties( props );
    if( !ok ) {
        dbg( D_ERROR ) << "Failed to start music: " << SDL_GetError();
    }
    return ok;
}

void stop_music( int fade_out_ms )
{
    if( g_music_track == nullptr ) {
        return;
    }
    ++g_music_generation;
    if( fade_out_ms <= 0 ) {
        MIX_StopTrack( g_music_track, 0 );
    } else {
        MIX_StopTrack( g_music_track, MIX_TrackMSToFrames( g_music_track, fade_out_ms ) );
    }
}

void set_music_volume( int vol )
{
    if( g_music_track == nullptr ) {
        return;
    }
    MIX_SetTrackGain( g_music_track, volume_to_gain( vol ) );
}

bool is_music_playing()
{
    return g_music_track != nullptr && MIX_TrackPlaying( g_music_track );
}

} // namespace sound_backend

#endif // SDL_SOUND && USE_SDL3
