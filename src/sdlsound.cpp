#if defined(SDL_SOUND)

#include "sdlsound.h"

#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <exception>
#include <memory>
#include <ostream>
#include <utility>

#if defined(_MSC_VER) && defined(USE_VCPKG)
#    include <SDL2/SDL_mixer.h>
#else
#    include <SDL_mixer.h>
#endif

#include "cached_options.h"
#include "debug.h"
#include "init.h"
#include "json.h"
#include "loading_ui.h"
#include "messages.h"
#include "music.h"
#include "options.h"
#include "path_info.h"
#include "rng.h"
#include "sdl_wrappers.h"
#include "sounds.h"
#include "units.h"

#define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

struct sfx_args {
    std::string id;
    std::string variant;
    std::string season;
    std::optional<bool> indoors;
    std::optional<bool> night;

    bool operator<( const sfx_args &rhs ) const {
        int r_ind = rhs.indoors.value_or( -1 );
        int r_nit = rhs.night.value_or( -1 );
        int l_ind = indoors.value_or( -1 );
        int l_nit = night.value_or( -1 );
        return std::tie( id, variant, season, l_ind, l_nit ) <
               std::tie( rhs.id, rhs.variant, rhs.season, r_ind, r_nit );
    }
};
struct sound_effect_resource {
    std::string path;
    struct deleter {
        // Operator overloaded to leverage deletion API.
        void operator()( Mix_Chunk *const c ) const {
            Mix_FreeChunk( c );
        }
    };
    std::unique_ptr<Mix_Chunk, deleter> chunk;
};

static int add_sfx_path( const std::string &path );

struct sound_effect {
    const int volume = 0;
    const int resource_id = 0;

    sound_effect() = default;
    sound_effect( int volume, const std::string &path )
        : volume( volume ), resource_id( add_sfx_path( path ) ) {}
};

// Sound effects are primarily keyed by id
// They support a variety of optional 'variations', such as:
// - arbitrary variant string
// - season
// - indoors/outdoors
// - nighttime/daytime
// Each of the variations is optional if unspecified. Certain lookup
// functions attempt to find a best matching sound effect and fall back
// to default values if a variant is not found. This can be modelled as
// a multi level lookup, in effect.
// Variants always fall back to their default value, never an opposing value.
// So if a nighttime sfx is requested, a daytime sfx cannot fulfill it.
namespace
{

enum class sfx_season : uint8_t {
    NONE = 0,
    SPRING,
    SUMMER,
    AUTUMN,
    WINTER,
    COUNT,
};

sfx_season season_from_string( const std::string &str )
{
    if( str.empty() ) {
        return sfx_season::NONE;
    }
    if( str == "spring" ) {
        return sfx_season::SPRING;
    }
    if( str == "summer" ) {
        return sfx_season::SUMMER;
    }
    if( str == "autumn" ) {
        return sfx_season::AUTUMN;
    }
    if( str == "winter" ) {
        return sfx_season::WINTER;
    }
    throw std::invalid_argument( std::string( "sfx specified unknown season " ) + str );
}

enum class sfx_in_or_out : uint8_t {
    EITHER = 0,
    OUTDOORS,
    INDOORS,
    COUNT,
};

// This is encoded as an optional bool in json, so we cheat a little and accept -1 for 'not set'
sfx_in_or_out in_or_out_from_int( int value )
{
    int adjusted = value + 1;
    if( adjusted >= static_cast<int>( sfx_in_or_out::COUNT ) || adjusted < 0 ) {
        throw std::invalid_argument( std::string( "sfx specified unknown inside/outside value " ) +
                                     std::to_string( value ) );
    }
    return static_cast<sfx_in_or_out>( adjusted );
}

enum class sfx_time_of_day : uint8_t {
    ANY = 0,
    DAYTIME,
    NIGHTTIME,
    COUNT,
};

// This is encoded as an optional bool in json, so we cheat a little and accept -1 for 'not set'
sfx_time_of_day tod_from_int( int value )
{
    int adjusted = value + 1;
    if( adjusted >= static_cast<int>( sfx_time_of_day::COUNT ) || adjusted < 0 ) {
        throw std::invalid_argument( std::string( "sfx specified unknown day/night value " ) +
                                     std::to_string( value ) );
    }
    return static_cast<sfx_time_of_day>( adjusted );
}

// Fun but ugly template time.
template<typename Map, typename Key>
const std::vector<sound_effect> *find_sfx( const Map &c, Key &&k )
{
    auto it = c.find( std::forward<Key>( k ) );
    if( it == c.end() ) {
        return nullptr;
    }
    return &it->second;
}

template<typename Map, typename Key1, typename Key2, typename ...Keys>
const std::vector<sound_effect> *find_sfx( const Map &c, Key1 &&k, Key2 &&k2, Keys &&...keys )
{
    auto it = c.find( std::forward<Key1>( k ) );
    if( it == c.end() ) {
        return nullptr;
    }
    return find_sfx( it->second, std::forward<Key2>( k2 ),
                     std::forward<Keys>( keys )... );
}

template<typename Map, typename Key, typename Default>
const std::vector<sound_effect> *find_closest_sfx( const Map &c, Key &&k, Default &&d )
{
    auto it = c.find( std::forward<Key>( k ) );
    if( it == c.end() ) {
        it = c.find( std::forward<Default>( d ) );
    }
    if( it == c.end() ) {
        return nullptr;
    }
    return &it->second;
}

template<typename Map, typename Key1, typename Default1, typename Key2, typename Default2, typename ...KDs>
const std::vector<sound_effect> *find_closest_sfx( const Map &c, Key1 &&k, Default1 &&d1, Key2 &&k2,
        Default2 &&d2, KDs &&...kds )
{
    auto it = c.find( std::forward<Key1>( k ) );
    if( it == c.end() ) {
        it = c.find( std::forward<Default1>( d1 ) );
    }
    if( it == c.end() ) {
        return nullptr;
    }
    return find_closest_sfx( it->second, std::forward<Key2>( k2 ),
                             std::forward<Default2>( d2 ),
                             std::forward<KDs>( kds )... );

}

template<typename Map, typename Key>
std::vector<sound_effect> &emplace_sfx( Map &c, Key &&k )
{
    return c[std::forward<Key>( k )];
}

template<typename Map, typename Key1, typename Key2, typename ...Keys>
std::vector<sound_effect> &emplace_sfx( Map &c, Key1 &&k, Key2 &&k2, Keys &&...keys )
{
    auto &nested_container = c[std::forward<Key1>( k )];
    return emplace_sfx( nested_container, std::forward<Key2>( k2 ),
                        std::forward<Keys>( keys )... );
}

int bool_or( const std::optional<bool> &opt, int defl )
{
    return opt.has_value() ? opt.value() : defl;
}

} // namespace

struct sfx_map {
        void clear() {
            effects.clear();
        }

        std::vector<sound_effect> &operator[]( const sfx_args &key ) {
            return emplace_sfx( effects, key.id, key.variant, season_from_string( key.season ),
                                in_or_out_from_int( bool_or( key.indoors, -1 ) ), tod_from_int( bool_or( key.night, -1 ) ) );
        }

        const std::vector<sound_effect> *find( const sfx_args &key ) const {
            return find_sfx( effects, key.id, key.variant, season_from_string( key.season ),
                             in_or_out_from_int( bool_or( key.indoors, -1 ) ), tod_from_int( bool_or( key.night, -1 ) ) );
        }

        std::vector<sound_effect> *end() const {
            return nullptr;
        }

        const std::vector<sound_effect> *find( const std::string &id, const std::string &variant,
                                               const std::string &season, const std::optional<bool> &is_indoors,
                                               const std::optional<bool> &is_night ) const {
            return find_closest_sfx( effects, id, "", variant, "default", season_from_string( season ),
                                     sfx_season::NONE, in_or_out_from_int( bool_or( is_indoors, -1 ) ), sfx_in_or_out::EITHER,
                                     tod_from_int( bool_or( is_night, -1 ) ), sfx_time_of_day::ANY );
        }

    private:
        std::map<std::string, std::map<std::string, std::map<sfx_season, std::map<sfx_in_or_out, std::map<sfx_time_of_day, std::vector<sound_effect>>>>>>
        effects;

};

struct sfx_resources_t {
    std::vector<sound_effect_resource> resource;
    sfx_map sound_effects;
};

struct music_playlist {
    // list of filenames relative to the soundpack location
    struct entry {
        std::string file;
        int volume;
    };
    std::vector<entry> entries;
    bool shuffle;

    music_playlist() : shuffle( false ) {
    }
};
/** The music we're currently playing. */
static Mix_Music *current_music = nullptr;
static int current_music_track_volume = 0;
static std::string current_playlist;
static size_t current_playlist_at = 0;
static size_t absolute_playlist_at = 0;
static std::vector<std::size_t> playlist_indexes;
static bool sound_init_success = false;
static std::map<std::string, music_playlist> playlists;
static cata_path current_soundpack_path;

static std::unordered_map<std::string, int> unique_paths;
static sfx_resources_t sfx_resources;
static std::vector<sfx_args> sfx_preload;

bool sounds::sound_enabled = false;

static bool check_sound( const int volume = 1 )
{
    return sound_init_success && sounds::sound_enabled && volume > 0;
}

/**
 * Attempt to initialize an audio device.  Returns false if initialization fails.
 */
bool init_sound()
{
    int audio_rate = 44100;
    Uint16 audio_format = AUDIO_S16;
    int audio_channels = 2;
    int audio_buffers = 2048;

    // We should only need to init once
    if( !sound_init_success ) {
        // Mix_OpenAudio returns non-zero if something went wrong trying to open the device
        if( !Mix_OpenAudioDevice( audio_rate, audio_format, audio_channels, audio_buffers, nullptr,
                                  SDL_AUDIO_ALLOW_FREQUENCY_CHANGE ) ) {
            Mix_AllocateChannels( 128 );
            Mix_ReserveChannels( static_cast<int>( sfx::channel::MAX_CHANNEL ) );

            // For the sound effects system.
            Mix_GroupChannels( static_cast<int>( sfx::channel::daytime_outdoors_env ),
                               static_cast<int>( sfx::channel::nighttime_outdoors_env ),
                               static_cast<int>( sfx::group::time_of_day ) );
            Mix_GroupChannels( static_cast<int>( sfx::channel::underground_env ),
                               static_cast<int>( sfx::channel::outdoor_blizzard ),
                               static_cast<int>( sfx::group::weather ) );
            Mix_GroupChannels( static_cast<int>( sfx::channel::danger_extreme_theme ),
                               static_cast<int>( sfx::channel::danger_low_theme ),
                               static_cast<int>( sfx::group::context_themes ) );
            Mix_GroupChannels( static_cast<int>( sfx::channel::stamina_75 ),
                               static_cast<int>( sfx::channel::stamina_35 ),
                               static_cast<int>( sfx::group::fatigue ) );

            sound_init_success = true;
        } else {
            dbg( D_ERROR ) << "Failed to open audio mixer, sound won't work: " << Mix_GetError();
        }
    }

    return sound_init_success;
}
void shutdown_sound()
{
    // De-allocate all loaded sound.
    sfx_resources.resource.clear();
    sfx_resources.sound_effects.clear();

    playlists.clear();
    Mix_CloseAudio();
}

static void musicFinished();

static void play_music_file( const std::string &filename, int volume )
{
    if( test_mode ) {
        return;
    }

    if( !check_sound( volume ) ) {
        return;
    }

    const std::string path = ( current_soundpack_path / filename ).get_unrelative_path().u8string();
    current_music = Mix_LoadMUS( path.c_str() );
    if( current_music == nullptr ) {
        dbg( D_ERROR ) << "Failed to load audio file " << path << ": " << Mix_GetError();
        return;
    }
    Mix_VolumeMusic( volume * get_option<int>( "MUSIC_VOLUME" ) / 100 );
    if( Mix_PlayMusic( current_music, 0 ) != 0 ) {
        dbg( D_ERROR ) << "Starting playlist " << path << " failed: " << Mix_GetError();
        return;
    }
    Mix_HookMusicFinished( musicFinished );
}

/** Callback called when we finish playing music. */
void musicFinished()
{
    if( test_mode ) {
        return;
    }

    Mix_HaltMusic();
    Mix_FreeMusic( current_music );
    current_music = nullptr;

    std::string new_playlist = music::get_music_id_string();

    if( current_playlist != new_playlist ) {
        play_music( new_playlist );
        return;
    }

    const auto iter = playlists.find( current_playlist );
    if( iter == playlists.end() ) {
        return;
    }
    const music_playlist &list = iter->second;
    if( list.entries.empty() ) {
        return;
    }

    // Load the next file to play.
    absolute_playlist_at++;

    // Wrap around if we reached the end of the playlist.
    if( absolute_playlist_at >= list.entries.size() ) {
        absolute_playlist_at = 0;
    }

    current_playlist_at = playlist_indexes.at( absolute_playlist_at );

    const music_playlist::entry &next = list.entries[current_playlist_at];
    play_music_file( next.file, next.volume );
}

void play_music( const std::string &playlist )
{
    // Don't interrupt playlist that's already playing.
    if( playlist == current_playlist ) {
        return;
    } else {
        stop_music();
    }

    const auto iter = playlists.find( playlist );
    if( iter == playlists.end() ) {
        return;
    }
    const music_playlist &list = iter->second;
    if( list.entries.empty() ) {
        return;
    }

    for( size_t i = 0; i < list.entries.size(); i++ ) {
        playlist_indexes.push_back( i );
    }
    if( list.shuffle ) {
        // Son't need to worry about the determinism check here because it only
        // affects audio, not game logic.
        // NOLINTNEXTLINE(cata-determinism)
        static cata_default_random_engine eng = cata_default_random_engine(
                std::chrono::steady_clock::now().time_since_epoch().count() );
        std::shuffle( playlist_indexes.begin(), playlist_indexes.end(), eng );
    }

    current_playlist = playlist;
    current_playlist_at = playlist_indexes.at( absolute_playlist_at );

    const music_playlist::entry &next = list.entries[current_playlist_at];
    current_music_track_volume = next.volume;
    play_music_file( next.file, next.volume );
}

void stop_music()
{
    if( test_mode ) {
        return;
    }

    Mix_FreeMusic( current_music );
    Mix_HaltMusic();
    current_music = nullptr;

    playlist_indexes.clear();
    current_playlist.clear();
    current_playlist_at = 0;
    absolute_playlist_at = 0;
}

void update_music_volume()
{
    if( test_mode ) {
        return;
    }

    Mix_VolumeMusic( current_music_track_volume * get_option<int>( "MUSIC_VOLUME" ) / 100 );

    bool sound_enabled_old = sounds::sound_enabled;
    sounds::sound_enabled = ::get_option<bool>( "SOUND_ENABLED" );

    if( !sounds::sound_enabled ) {
        stop_music();
        music::deactivate_music_id_all();
        return;
    } else if( !sound_enabled_old ) {
        play_music( music::get_music_id_string() );
    }
}

// Allocate new Mix_Chunk as a null-chunk. Results in a valid, but empty chunk
// that is created when loading of a sound effect resource fails. Does not own
// memory. Mix_FreeChunk will free the SDL_malloc'd Mix_Chunk pointer.
static Mix_Chunk *make_null_chunk()
{
    static Mix_Chunk null_chunk = { 0, nullptr, 0, 0 };
    // SDL_malloc to match up with Mix_FreeChunk's SDL_free call
    // to free the Mix_Chunk object memory
    Mix_Chunk *nchunk = static_cast<Mix_Chunk *>( SDL_malloc( sizeof( Mix_Chunk ) ) );

    // Assign as copy of null_chunk
    ( *nchunk ) = null_chunk;
    return nchunk;
}

static Mix_Chunk *load_chunk( const std::string &path )
{
    Mix_Chunk *result = Mix_LoadWAV( path.c_str() );
    if( result == nullptr ) {
        // Failing to load a sound file is not a fatal error worthy of a backtrace
        dbg( D_WARNING ) << "Failed to load sfx audio file " << path << ": " << Mix_GetError();
        result = make_null_chunk();
    }
    return result;
}

// Check to see if the resource has already been loaded
// - Loaded: Return stored pointer
// - Not Loaded: Load chunk from stored resource path
static Mix_Chunk *get_sfx_resource( int resource_id )
{
    sound_effect_resource &resource = sfx_resources.resource[ resource_id ];
    if( !resource.chunk ) {
        cata_path path = current_soundpack_path / resource.path;
        resource.chunk.reset( load_chunk( path.generic_u8string() ) );
    }
    return resource.chunk.get();
}

static int add_sfx_path( const std::string &path )
{
    auto find_result = unique_paths.find( path );
    if( find_result != unique_paths.end() ) {
        return find_result->second;
    } else {
        int result = sfx_resources.resource.size();
        sound_effect_resource new_resource;
        new_resource.path = path;
        new_resource.chunk.reset();
        sfx_resources.resource.push_back( std::move( new_resource ) );
        unique_paths[ path ] = result;
        return result;
    }
}

void sfx::load_sound_effects( const JsonObject &jsobj )
{
    if( !sound_init_success ) {
        return;
    }
    sfx_args key = {
        jsobj.get_string( "id" ),
        "", // actual variant string is filled in the variant loop
        jsobj.get_string( "season", "" ),
        std::nullopt,
        std::nullopt,
    };
    if( jsobj.has_bool( "is_indoors" ) ) {
        key.indoors = jsobj.get_bool( "is_indoors" );
    }
    if( jsobj.has_bool( "is_night" ) ) {
        key.night = jsobj.get_bool( "is_night" );
    }
    const int volume = jsobj.get_int( "volume", 100 );
    std::vector<std::string> variants;
    if( jsobj.has_array( "variant" ) ) {
        variants = jsobj.get_string_array( "variant" );
    } else if( jsobj.has_string( "variant" ) ) {
        variants = { jsobj.get_string( "variant" ) };
    } else {
        variants = { "default" };
    }
    for( const std::string &variant : variants ) {
        key.variant = variant;
        std::vector<sound_effect> &effects = sfx_resources.sound_effects[key];
        for( const std::string file : jsobj.get_array( "files" ) ) {
            effects.emplace_back( volume, file );
        }
    }
}

void sfx::load_sound_effect_preload( const JsonObject &jsobj )
{
    if( !sound_init_success ) {
        return;
    }

    for( JsonObject aobj : jsobj.get_array( "preload" ) ) {
        sfx_args preload_key = {
            aobj.get_string( "id" ),
            "", // actual variant string is filled in the variant loop
            aobj.get_string( "season", "" ),
            std::nullopt,
            std::nullopt,
        };
        if( aobj.has_bool( "is_indoors" ) ) {
            preload_key.indoors = aobj.get_bool( "is_indoors" );
        }
        if( aobj.has_bool( "is_night" ) ) {
            preload_key.night = aobj.get_bool( "is_night" );
        }
        std::vector<std::string> variants;
        if( aobj.has_array( "variant" ) ) {
            variants = aobj.get_string_array( "variant" );
        } else if( aobj.has_string( "variant" ) ) {
            variants = { aobj.get_string( "variant" ) };
        } else {
            variants = { "default" };
        }
        for( const std::string &variant : variants ) {
            preload_key.variant = variant;
            sfx_preload.push_back( preload_key );
        }
    }
}

void sfx::load_playlist( const JsonObject &jsobj )
{
    if( !sound_init_success ) {
        return;
    }

    for( JsonObject playlist : jsobj.get_array( "playlists" ) ) {
        const std::string playlist_id = playlist.get_string( "id" );
        music_playlist playlist_to_load;
        playlist_to_load.shuffle = playlist.get_bool( "shuffle", false );

        for( JsonObject entry : playlist.get_array( "files" ) ) {
            const music_playlist::entry e{ entry.get_string( "file" ),  entry.get_int( "volume" ) };
            playlist_to_load.entries.push_back( e );
        }

        playlists[playlist_id] = std::move( playlist_to_load );

        music::update_music_id_is_empty_flag( playlist_id, true );
    }
}

// Returns a random sound effect matching given id and variant, but with fallback to "default" variants.
// May still return `nullptr`
static const sound_effect *find_random_effect( const std::string &id, const std::string &variant,
        const std::string &season, const std::optional<bool> &is_indoors,
        const std::optional<bool> &is_night )
{
    const std::vector<sound_effect> *iter = sfx_resources.sound_effects.find( id, variant, season,
                                            is_indoors,
                                            is_night );
    if( !iter ) {
        return nullptr;
    }

    return &random_entry_ref( *iter );
}

bool sfx::has_variant_sound( const std::string &id, const std::string &variant,
                             const std::string &season, const std::optional<bool> &is_indoors,
                             const std::optional<bool> &is_night )
{
    return find_random_effect( id, variant, season, is_indoors, is_night ) != nullptr;
}

// Deletes the dynamically created chunk (if such a chunk had been played).
static void cleanup_when_channel_finished( int /* channel */, void *udata )
{
    Mix_Chunk *chunk = static_cast<Mix_Chunk *>( udata );
    free( chunk->abuf );
    free( chunk );
}

// empty effect, as we cannot change the size of the output buffer,
// therefore we cannot do the math from do_pitch_shift here
static void empty_effect( int /* chan */, void * /* stream */, int /* len */, void * /* udata */ )
{
}

static Mix_Chunk *do_pitch_shift( Mix_Chunk *s, float pitch )
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
        Sint16 lt = 0;
        Sint16 rt = 0;
        Sint16 lt_out = 0;
        Sint16 rt_out = 0;
        Sint64 lt_avg = 0;
        Sint64 rt_avg = 0;
        Uint32 begin = static_cast<Uint32>( static_cast<float>( i ) / pitch_real );
        Uint32 end = static_cast<Uint32>( static_cast<float>( i + 1 ) / pitch_real );

        // check for boundary case
        if( end > 0 && ( end >= ( s->alen / 4 ) ) ) {
            end = begin;
        }

        for( Uint32 j = begin; j <= end; j++ ) {
            lt = ( s->abuf[( 4 * j ) + 1] << 8 ) | ( s->abuf[( 4 * j ) + 0] );
            rt = ( s->abuf[( 4 * j ) + 3] << 8 ) | ( s->abuf[( 4 * j ) + 2] );
            lt_avg += lt;
            rt_avg += rt;
        }
        lt_out = static_cast<Sint16>( static_cast<float>( lt_avg ) / static_cast<float>
                                      ( end - begin + 1 ) );
        rt_out = static_cast<Sint16>( static_cast<float>( rt_avg ) / static_cast<float>
                                      ( end - begin + 1 ) );
        result->abuf[( 4 * i ) + 1] = static_cast<Uint8>( ( lt_out >> 8 ) & 0xFF );
        result->abuf[( 4 * i ) + 0] = static_cast<Uint8>( lt_out & 0xFF );
        result->abuf[( 4 * i ) + 3] = static_cast<Uint8>( ( rt_out >> 8 ) & 0xFF );
        result->abuf[( 4 * i ) + 2] = static_cast<Uint8>( rt_out & 0xFF );
    }
    return result;
}

void sfx::play_variant_sound( const std::string &id, const std::string &variant,
                              const std::string &season, const std::optional<bool> &is_indoors,
                              const std::optional<bool> &is_night, int volume )
{
    if( test_mode ) {
        return;
    }

    add_msg_debug( debugmode::DF_SOUND, "sound id: %s, variant: %s, volume: %d ", id, variant, volume );

    if( !check_sound( volume ) ) {
        return;
    }
    const sound_effect *eff = find_random_effect( id, variant, season, is_indoors, is_night );
    if( eff == nullptr ) {
        eff = find_random_effect( id, "default", "", std::optional<bool>(), std::optional<bool>() );
        if( eff == nullptr ) {
            return;
        }
    }
    const sound_effect &selected_sound_effect = *eff;

    Mix_Chunk *effect_to_play = get_sfx_resource( selected_sound_effect.resource_id );
    Mix_VolumeChunk( effect_to_play,
                     selected_sound_effect.volume * get_option<int>( "SOUND_EFFECT_VOLUME" ) * volume / ( 100 * 100 ) );
    bool failed = Mix_PlayChannel( static_cast<int>( channel::any ), effect_to_play, 0 ) == -1;
    if( failed ) {
        dbg( D_ERROR ) << "Failed to play sound effect: " << Mix_GetError() << " id:" << id
                       << " variant:" << variant << " season:" << season;
    }
}

void sfx::play_variant_sound( const std::string &id, const std::string &variant,
                              const std::string &season, const std::optional<bool> &is_indoors,
                              const std::optional<bool> &is_night, int volume, units::angle angle,
                              double pitch_min, double pitch_max )
{
    if( test_mode ) {
        return;
    }

    add_msg_debug( debugmode::DF_SOUND, "sound id: %s, variant: %s, volume: %d ", id, variant, volume );

    if( !check_sound( volume ) ) {
        return;
    }
    const sound_effect *eff = find_random_effect( id, variant, season, is_indoors, is_night );
    if( eff == nullptr ) {
        return;
    }
    const sound_effect &selected_sound_effect = *eff;

    Mix_Chunk *effect_to_play = get_sfx_resource( selected_sound_effect.resource_id );
    bool is_pitched = ( pitch_min > 0 ) && ( pitch_max > 0 );
    if( is_pitched ) {
        double pitch_random = rng_float( pitch_min, pitch_max );
        effect_to_play = do_pitch_shift( effect_to_play, static_cast<float>( pitch_random ) );
    }
    Mix_VolumeChunk( effect_to_play,
                     selected_sound_effect.volume * get_option<int>( "SOUND_EFFECT_VOLUME" ) * volume / ( 100 * 100 ) );
    int channel = Mix_PlayChannel( static_cast<int>( sfx::channel::any ), effect_to_play, 0 );
    bool failed = channel == -1;
    if( !failed && is_pitched ) {
        if( Mix_RegisterEffect( channel, empty_effect, cleanup_when_channel_finished,
                                effect_to_play ) == 0 ) {
            // To prevent use after free, stop the playback right now.
            failed = true;
            dbg( D_WARNING ) << "Mix_RegisterEffect failed: " << Mix_GetError();
            Mix_HaltChannel( channel );
        }
    }
    if( !failed ) {
        if( Mix_SetPosition( channel, static_cast<Sint16>( to_degrees( angle ) ), 1 ) == 0 ) {
            // Not critical
            dbg( D_INFO ) << "Mix_SetPosition failed: " << Mix_GetError();
        }
    }
    if( failed ) {
        dbg( D_ERROR ) << "Failed to play sound effect: " << Mix_GetError() << " id:" << id
                       << " variant:" << variant << " season:" << season;
        if( is_pitched ) {
            cleanup_when_channel_finished( channel, effect_to_play );
        }
    }
}

void sfx::play_ambient_variant_sound( const std::string &id, const std::string &variant,
                                      const std::string &season, const std::optional<bool> &is_indoors,
                                      const std::optional<bool> &is_night, int volume,
                                      channel channel, int fade_in_duration, double pitch, int loops )
{
    if( test_mode ) {
        return;
    }
    if( !check_sound( volume ) ) {
        return;
    }
    if( is_channel_playing( channel ) ) {
        return;
    }
    const sound_effect *eff = find_random_effect( id, variant, season, is_indoors, is_night );
    if( eff == nullptr ) {
        return;
    }
    const sound_effect &selected_sound_effect = *eff;

    Mix_Chunk *effect_to_play = get_sfx_resource( selected_sound_effect.resource_id );
    bool is_pitched = pitch > 0;
    if( is_pitched ) {
        effect_to_play = do_pitch_shift( effect_to_play, static_cast<float>( pitch ) );
    }
    Mix_VolumeChunk( effect_to_play,
                     selected_sound_effect.volume * get_option<int>( "AMBIENT_SOUND_VOLUME" ) * volume / ( 100 * 100 ) );
    bool failed = false;
    int ch = static_cast<int>( channel );
    if( fade_in_duration ) {
        failed = ( Mix_FadeInChannel( ch, effect_to_play, loops, fade_in_duration ) == -1 );
    } else {
        failed = ( Mix_PlayChannel( ch, effect_to_play, loops ) == -1 );
    }
    if( !failed && is_pitched ) {
        if( Mix_RegisterEffect( ch, empty_effect, cleanup_when_channel_finished, effect_to_play ) == 0 ) {
            // To prevent use after free, stop the playback right now.
            failed = true;
            dbg( D_WARNING ) << "Mix_RegisterEffect failed: " << Mix_GetError();
            Mix_HaltChannel( ch );
        }
    }
    if( failed ) {
        dbg( D_ERROR ) << "Failed to play sound effect: " << Mix_GetError() << " id:" << id
                       << " variant:" << variant << " season:" << season;
        if( is_pitched ) {
            cleanup_when_channel_finished( ch, effect_to_play );
        }
    }
}

void load_soundset()
{
    const cata_path default_path = PATH_INFO::defaultsounddir();
    const std::string default_soundpack = "basic";
    std::string current_soundpack = get_option<std::string>( "SOUNDPACKS" );
    cata_path soundpack_path;

    // Get current soundpack and it's directory path.
    if( current_soundpack.empty() ) {
        dbg( D_ERROR ) << "Soundpack not set in options or empty.";
        soundpack_path = default_path;
        current_soundpack = default_soundpack;
    } else {
        dbg( D_INFO ) << "Current soundpack is: " << current_soundpack;
        soundpack_path = SOUNDPACKS[current_soundpack];
    }

    if( soundpack_path.empty() ) {
        dbg( D_ERROR ) << "Soundpack with name " << current_soundpack << " can't be found or empty string";
        soundpack_path = default_path;
        current_soundpack = default_soundpack;
    } else {
        dbg( D_INFO ) << '"' << current_soundpack << '"' << " soundpack: found path: " << soundpack_path;
    }

    current_soundpack_path = soundpack_path;
    try {
        loading_ui ui( false );
        DynamicDataLoader::get_instance().load_data_from_path( soundpack_path, "core", ui );
    } catch( const std::exception &err ) {
        debugmsg( "failed to load sounds: %s", err.what() );
    }

    // Preload sound effects
    for( const sfx_args &preload : sfx_preload ) {
        const std::vector<sound_effect> *find_result = sfx_resources.sound_effects.find( preload );
        if( find_result != sfx_resources.sound_effects.end() ) {
            for( const sound_effect &sfx : *find_result ) {
                get_sfx_resource( sfx.resource_id );
            }
        }
    }

    // Memory of unique_paths no longer required, swap with locally scoped unordered_map
    // to force deallocation of resources.
    {
        unique_paths.clear();
        std::unordered_map<std::string, int> t_swap;
        unique_paths.swap( t_swap );
    }
    // Memory of sfx_preload no longer required, swap with locally scoped vector
    // to force deallocation of resources.
    {
        sfx_preload.clear();
        std::vector<sfx_args> t_swap;
        sfx_preload.swap( t_swap );
    }
}

#endif
