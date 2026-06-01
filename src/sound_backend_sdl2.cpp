#if defined(SDL_SOUND) && !defined(USE_SDL3)

#include "sound_backend.h"

#include "sdl_wrappers.h"

#if defined(_MSC_VER) && defined(USE_VCPKG)
#include <SDL2/SDL_mixer.h>
#else
#include <SDL_mixer.h>
#endif

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

#include "cata_assert.h"
#include "debug.h"
#include "sounds.h"

#define dbg(x) DebugLog((x), D_SDL) << __FILE__ << ":" << __LINE__ << ": "

namespace sound_backend
{

namespace
{
// Channels whose stop was signaled from the audio thread; reaped on
// the main thread at the top of every play_effect call. Mix_HaltChannel
// cannot be called from inside an SDL_mixer effect callback.
std::vector<sfx::channel> channels_to_end;
std::mutex channels_to_end_mutex;

constexpr float sound_speed_factor = 0.25f;

struct sound_effect_handler {
    Mix_Chunk *audio_src;
    bool active;
    bool owns_audio;
    float current_sample_index = 0;
    int loops_remaining = 0;
    bool marked_for_termination = false;

    ~sound_effect_handler() {
        if( owns_audio ) {
            free( audio_src->abuf );
            free( audio_src );
        }
    }

    static void on_finish( int /* chan */, void *udata ) {
        sound_effect_handler *handler = static_cast<sound_effect_handler *>( udata );
        cata_assert( handler != nullptr && handler->audio_src != nullptr );
        delete handler;
    }

    static void slowed_time_effect( int channel, void *stream, int len, void *udata ) {
        sound_effect_handler *handler = static_cast<sound_effect_handler *>( udata );

        using sample = int16_t;
        constexpr int bytes_per_sample = sizeof( sample ) * 2;

        const float playback_speed = slow_time_predicate_active() ? sound_speed_factor : 1;
        const int num_source_samples = handler->audio_src->alen / bytes_per_sample;

        if( handler->loops_remaining < 0 ) {
            std::memset( stream, 0, len );

            const int num_samples = len / bytes_per_sample;
            handler->current_sample_index += num_samples * playback_speed;
            if( handler->current_sample_index >= num_source_samples ) {
                handler->loops_remaining--;
            }

            if( !handler->marked_for_termination && handler->loops_remaining < -1 ) {
                if( channels_to_end_mutex.try_lock() ) {
                    handler->marked_for_termination = true;
                    channels_to_end.push_back( static_cast<sfx::channel>( channel ) );
                    channels_to_end_mutex.unlock();
                }
            }
            return;
        }

        for( int dst_index = 0; dst_index < len / bytes_per_sample &&
             handler->current_sample_index < num_source_samples; dst_index++ ) {

            int low_index = static_cast<int>( std::floor( handler->current_sample_index ) );
            int high_index = static_cast<int>( std::ceil( handler->current_sample_index ) );

            if( high_index == num_source_samples ) {
                high_index = 0;
            }
            if( low_index == num_source_samples ) {
                low_index = 0;
            }

            for( int ear_offset = 0; ear_offset < 4; ear_offset += 2 ) {
                sample low_value;
                sample high_value;

                std::memcpy( &low_value,
                             static_cast<uint8_t *>( handler->audio_src->abuf ) + ear_offset +
                             low_index * bytes_per_sample, sizeof( sample ) );
                std::memcpy( &high_value,
                             static_cast<uint8_t *>( handler->audio_src->abuf ) + ear_offset +
                             high_index * bytes_per_sample, sizeof( sample ) );

                const float interpolation_factor = handler->current_sample_index - low_index;
                const sample interpolated = static_cast<sample>(
                                                ( high_value - low_value ) * interpolation_factor + low_value );

                std::memcpy( static_cast<uint8_t *>( stream ) + dst_index * bytes_per_sample + ear_offset,
                             &interpolated, sizeof( sample ) );
            }

            handler->current_sample_index += 1.0f * playback_speed;
            if( handler->current_sample_index >= num_source_samples ) {
                handler->loops_remaining--;
                handler->current_sample_index = std::fmod( handler->current_sample_index,
                                                static_cast<float>( num_source_samples ) );

                if( handler->loops_remaining < 0 ) {
                    const int bytes_remaining = len - dst_index * bytes_per_sample;
                    std::memset( static_cast<uint8_t *>( stream ) + dst_index * bytes_per_sample, 0,
                                 bytes_remaining );
                    break;
                }
            }
        }
    }
};

// Pitch-shift helper: returns a new Mix_Chunk owning freshly malloc'd
// PCM. Caller takes ownership (via sound_effect_handler::owns_audio
// for play_effect).
Mix_Chunk *do_pitch_shift( const Mix_Chunk *s, float pitch )
{
    Uint32 s_in = s->alen / 4;
    Uint32 s_out = static_cast<Uint32>( static_cast<float>( s_in ) * pitch );
    float pitch_real = static_cast<float>( s_out ) / static_cast<float>( s_in );
    Mix_Chunk *result = static_cast<Mix_Chunk *>( malloc( sizeof( Mix_Chunk ) ) );
    result->allocated = 1;
    result->alen = s_out * 4;
    result->abuf = static_cast<Uint8 *>( malloc( result->alen * sizeof( Uint8 ) ) );
    result->volume = s->volume;
    for( Uint32 i = 0; i < s_out; i++ ) {
        Sint64 lt_avg = 0;
        Sint64 rt_avg = 0;
        Uint32 begin = static_cast<Uint32>( static_cast<float>( i ) / pitch_real );
        Uint32 end = static_cast<Uint32>( static_cast<float>( i + 1 ) / pitch_real );

        if( end > 0 && ( end >= ( s->alen / 4 ) ) ) {
            end = begin;
        }

        for( Uint32 j = begin; j <= end; j++ ) {
            const Sint16 lt = ( s->abuf[( 4 * j ) + 1] << 8 ) | ( s->abuf[( 4 * j ) + 0] );
            const Sint16 rt = ( s->abuf[( 4 * j ) + 3] << 8 ) | ( s->abuf[( 4 * j ) + 2] );
            lt_avg += lt;
            rt_avg += rt;
        }
        const Sint16 lt_out = static_cast<Sint16>( static_cast<float>( lt_avg ) /
                              static_cast<float>( end - begin + 1 ) );
        const Sint16 rt_out = static_cast<Sint16>( static_cast<float>( rt_avg ) /
                              static_cast<float>( end - begin + 1 ) );
        result->abuf[( 4 * i ) + 1] = static_cast<Uint8>( ( lt_out >> 8 ) & 0xFF );
        result->abuf[( 4 * i ) + 0] = static_cast<Uint8>( lt_out & 0xFF );
        result->abuf[( 4 * i ) + 3] = static_cast<Uint8>( ( rt_out >> 8 ) & 0xFF );
        result->abuf[( 4 * i ) + 2] = static_cast<Uint8>( rt_out & 0xFF );
    }
    return result;
}
} // namespace

namespace
{
bool play_effect( Mix_Chunk *audio_src, sfx::channel slot, int loops,
                  int volume, int fade_in_ms,
                  int angle_deg, bool positional,
                  float pitch, bool owns_audio_src )
{
    // Reap any channels the audio thread asked us to halt.
    if( channels_to_end_mutex.try_lock() ) {
        for( sfx::channel channel : channels_to_end ) {
            const int success = Mix_HaltChannel( static_cast<int>( channel ) );
            if( success != 0 ) {
                dbg( D_ERROR ) << "Mix_HaltChannel failed: " << Mix_GetError();
            }
        }
        channels_to_end.clear();
        channels_to_end_mutex.unlock();
    }

    Mix_Chunk *chunk_to_play = audio_src;
    bool owns_chunk = owns_audio_src;
    if( pitch != 1.0f ) {
        chunk_to_play = do_pitch_shift( audio_src, pitch );
        owns_chunk = true;
    }

    sound_effect_handler *handler = new sound_effect_handler();
    handler->active = true;
    handler->audio_src = chunk_to_play;
    handler->owns_audio = owns_chunk;
    handler->loops_remaining = loops == -1 ? 10000 : loops;

    Mix_VolumeChunk( chunk_to_play, volume );

    int channel;
    if( fade_in_ms > 0 ) {
        channel = Mix_FadeInChannel( static_cast<int>( slot ), chunk_to_play, -1, fade_in_ms );
    } else {
        channel = Mix_PlayChannel( static_cast<int>( slot ), chunk_to_play, -1 );
    }
    bool failed = channel == -1;
    if( !failed ) {
        const int out = Mix_RegisterEffect( channel, sound_effect_handler::slowed_time_effect,
                                            sound_effect_handler::on_finish, handler );
        if( out == 0 ) {
            failed = true;
            dbg( D_WARNING ) << "Mix_RegisterEffect failed: " << Mix_GetError();
            Mix_HaltChannel( channel );
        }
    }
    if( !failed && positional ) {
        if( Mix_SetPosition( channel, static_cast<Sint16>( angle_deg ), 1 ) == 0 ) {
            dbg( D_INFO ) << "Mix_SetPosition failed: " << Mix_GetError();
        }
    }
    if( failed ) {
        sound_effect_handler::on_finish( -1, handler );
    }

    return failed;
}
} // namespace

struct sfx_audio {
    Mix_Chunk *chunk;
};

struct music_source {
    Mix_Music *music;
};

namespace
{
Mix_Chunk *make_null_chunk()
{
    // Valid Mix_Chunk shell with no sample data. Mix_FreeChunk frees
    // only the struct (allocated=0, abuf=nullptr). Matches the
    // sdlsound.cpp fallback pattern used for missing sfx files.
    Mix_Chunk *nchunk = static_cast<Mix_Chunk *>( SDL_malloc( sizeof( Mix_Chunk ) ) );
    nchunk->allocated = 0;
    nchunk->abuf = nullptr;
    nchunk->alen = 0;
    nchunk->volume = 0;
    return nchunk;
}
} // namespace

sfx_audio *load_sfx( const std::string &path )
{
    Mix_Chunk *result = Mix_LoadWAV( path.c_str() );
    if( result == nullptr ) {
        dbg( D_WARNING ) << "Failed to load sfx audio file " << path << ": " << Mix_GetError();
        result = make_null_chunk();
    }
    return new sfx_audio{ result };
}

void free_sfx( sfx_audio *a )
{
    if( a == nullptr ) {
        return;
    }
    if( a->chunk != nullptr ) {
        Mix_FreeChunk( a->chunk );
    }
    delete a;
}

music_source *load_music( const std::string &path )
{
    Mix_Music *m = Mix_LoadMUS( path.c_str() );
    if( m == nullptr ) {
        dbg( D_WARNING ) << "Failed to load music " << path << ": " << Mix_GetError();
        return nullptr;
    }
    return new music_source{ m };
}

void free_music( music_source *m )
{
    if( m == nullptr ) {
        return;
    }
    if( m->music != nullptr ) {
        Mix_FreeMusic( m->music );
    }
    delete m;
}

void fade_group( sfx::group g, int ms )
{
    Mix_FadeOutGroup( static_cast<int>( g ), ms );
}

void stop_all_sfx( int fade_out_ms )
{
    // Mix_FadeOutChannel(-1, ...) is a bulk-across-all-channels op.
    Mix_FadeOutChannel( -1, fade_out_ms );
}

void stop_reserved( sfx::channel slot, int fade_out_ms )
{
    if( fade_out_ms <= 0 ) {
        Mix_HaltChannel( static_cast<int>( slot ) );
    } else {
        Mix_FadeOutChannel( static_cast<int>( slot ), fade_out_ms );
    }
}

bool is_reserved_playing( sfx::channel slot )
{
    return Mix_Playing( static_cast<int>( slot ) ) != 0;
}

bool is_reserved_fading( sfx::channel slot )
{
    return Mix_FadingChannel( static_cast<int>( slot ) ) != MIX_NO_FADING;
}

int set_reserved_volume( sfx::channel slot, int vol )
{
    // Refuse on non-playing or fading channels; return -1 to signal the
    // refusal so callers know the requested volume did not stick.
    const int ch = static_cast<int>( slot );
    if( !Mix_Playing( ch ) ) {
        return -1;
    }
    if( Mix_FadingChannel( ch ) != MIX_NO_FADING ) {
        return -1;
    }
    return Mix_Volume( ch, vol );
}

int get_reserved_volume( sfx::channel slot )
{
    return Mix_Volume( static_cast<int>( slot ), -1 );
}

void set_reserved_position( sfx::channel slot, int angle_deg, int distance )
{
    const int ch = static_cast<int>( slot );
    if( Mix_SetPosition( ch, static_cast<Sint16>( angle_deg ),
                         static_cast<Uint8>( distance ) ) == 0 ) {
        // Positioning failure is non-fatal; the sound still plays.
        dbg( D_INFO ) << "Mix_SetPosition failed: " << Mix_GetError();
    }
}

static bool_predicate g_slow_time_predicate = nullptr;

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
float reserved_track_frequency_ratio( sfx::channel /*slot*/ )
{
    // SDL2_mixer has no native per-track frequency ratio. Slow-time
    // DSP runs via Mix_RegisterEffect and mutates output PCM in
    // place; there is no stored ratio to query.
    return 1.0f;
}
float last_oneshot_frequency_ratio()
{
    return 1.0f;
}
sfx_audio *make_synthetic_sfx_audio()
{
    return nullptr;
}
} // namespace testing

void poll()
{
    // SDL2 needs no main-thread audio pump.
}

bool init( int frequency, int out_channels, int chunk_size,
           const init_options & /*opts*/ )
{
    if( Mix_OpenAudioDevice( frequency, AUDIO_S16, out_channels, chunk_size, nullptr,
                             SDL_AUDIO_ALLOW_FREQUENCY_CHANGE ) != 0 ) {
        dbg( D_ERROR ) << "Failed to open audio mixer: " << Mix_GetError();
        return false;
    }
    Mix_AllocateChannels( 128 );
    Mix_ReserveChannels( static_cast<int>( sfx::channel::MAX_CHANNEL ) );
    return true;
}

void shutdown()
{
    Mix_CloseAudio();
}

void tag_reserved_groups( const std::array<sfx::group,
                          static_cast<size_t>( sfx::channel::MAX_CHANNEL )> &assignments )
{
    // Collapse adjacent same-group slots into a single Mix_GroupChannels
    // call, matching the existing init-time grouping pattern.
    const int n = static_cast<int>( sfx::channel::MAX_CHANNEL );
    int run_start = 0;
    while( run_start < n ) {
        const sfx::group g = assignments[run_start];
        int run_end = run_start;
        while( run_end + 1 < n && assignments[run_end + 1] == g ) {
            ++run_end;
        }
        // static_cast<int>(g) == 0 indicates "no group"; skip those runs.
        if( static_cast<int>( g ) != 0 ) {
            Mix_GroupChannels( run_start, run_end, static_cast<int>( g ) );
        }
        run_start = run_end + 1;
    }
}

void play_reserved( sfx_audio *a, sfx::channel slot, const play_opts &opts )
{
    if( a == nullptr || a->chunk == nullptr ) {
        return;
    }
    play_effect( a->chunk, slot, opts.loops, opts.volume, opts.fade_in_ms,
                 opts.angle_deg, opts.positional, opts.pitch, false );
}

void play_oneshot( sfx_audio *a, const play_opts &opts )
{
    if( a == nullptr || a->chunk == nullptr ) {
        return;
    }
    play_effect( a->chunk, sfx::channel::any, opts.loops, opts.volume, opts.fade_in_ms,
                 opts.angle_deg, opts.positional, opts.pitch, false );
}

static music_finished_cb g_music_finished_cb = nullptr;

void set_music_finished_cb( music_finished_cb cb )
{
    g_music_finished_cb = cb;
    if( cb != nullptr ) {
        Mix_HookMusicFinished( cb );
    } else {
        Mix_HookMusicFinished( nullptr );
    }
}

bool play_music( music_source *m, int loops, int fade_in_ms )
{
    if( m == nullptr || m->music == nullptr ) {
        return false;
    }
    int rc;
    if( fade_in_ms > 0 ) {
        rc = Mix_FadeInMusic( m->music, loops, fade_in_ms );
    } else {
        rc = Mix_PlayMusic( m->music, loops );
    }
    if( rc != 0 ) {
        dbg( D_ERROR ) << "Failed to start music: " << Mix_GetError();
        return false;
    }
    return true;
}

void stop_music( int /*fade_out_ms*/ )
{
    // Caller frees any owned music_source after stop; the backend only
    // stops playback.
    Mix_HaltMusic();
}

void set_music_volume( int vol )
{
    Mix_VolumeMusic( vol );
}

bool is_music_playing()
{
    return Mix_PlayingMusic() != 0;
}

} // namespace sound_backend

#endif // SDL_SOUND && !USE_SDL3
