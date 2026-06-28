#pragma once
#ifndef CATA_SRC_SOUND_BACKEND_H
#define CATA_SRC_SOUND_BACKEND_H

#if defined(SDL_SOUND)

#include <array>
#include <string>

#include "sounds.h"

namespace sound_backend
{

struct init_options {
    bool memory_only = false;
};

bool init( int frequency, int out_channels, int chunk_size,
           const init_options &opts );
void shutdown();

struct sfx_audio;
sfx_audio *load_sfx( const std::string &path );
void       free_sfx( sfx_audio * );

struct music_source;
music_source *load_music( const std::string &path );
void          free_music( music_source * );

struct play_opts {
    int   loops       = 0;
    int   fade_in_ms  = 0;
    int   volume      = 128;
    int   angle_deg   = 0;
    int   distance    = 1;
    bool  positional  = false;
    float pitch       = 1.0f;
};

void play_reserved( sfx_audio *a, sfx::channel slot, const play_opts &opts );
void play_oneshot( sfx_audio *a, const play_opts &opts );

void tag_reserved_groups( const std::array<sfx::group,
                          static_cast<size_t>( sfx::channel::MAX_CHANNEL )> &assignments );

void stop_reserved( sfx::channel slot, int fade_out_ms );
bool is_reserved_playing( sfx::channel slot );
bool is_reserved_fading( sfx::channel slot );
int  set_reserved_volume( sfx::channel slot, int vol );
int  get_reserved_volume( sfx::channel slot );
void set_reserved_position( sfx::channel slot, int angle_deg, int distance );

using bool_predicate = bool ( * )();
void set_slow_time_predicate( bool_predicate fn );

// Lets slow-time DSP code outside the backend query the installed
// predicate without re-exporting the function pointer.
bool slow_time_predicate_active();

namespace testing
{
// Reserved track's current frequency ratio. Used by slow-time
// property tests to verify predicate-to-ratio wiring without coupling
// tests to backend internals. SDL2 backend returns 1.0 (no native
// ratio concept).
float reserved_track_frequency_ratio( sfx::channel slot );

// Current frequency ratio of the most-recently-started one-shot
// track. SDL2 backend returns 1.0. Returns 0.0 if no one-shot is
// currently tracked.
float last_oneshot_frequency_ratio();

// Produce a minimal valid sfx_audio (a single silent stereo frame)
// that play_reserved and play_oneshot will accept. Used by tests
// that need to exercise the play path without loading a real file.
// SDL2 backend returns nullptr.
sfx_audio *make_synthetic_sfx_audio();
} // namespace testing

void stop_all_sfx( int fade_out_ms );
void fade_group( sfx::group g, int ms );

void poll();

using music_finished_cb = void ( * )();
void set_music_finished_cb( music_finished_cb );
bool play_music( music_source *m, int loops, int fade_in_ms );
void stop_music( int fade_out_ms );
void set_music_volume( int vol );
bool is_music_playing();

} // namespace sound_backend

#endif // SDL_SOUND

#endif // CATA_SRC_SOUND_BACKEND_H
