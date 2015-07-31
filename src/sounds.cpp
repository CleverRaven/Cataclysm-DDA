#include "sounds.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "enums.h"
#include "overmapbuffer.h"
#include "translations.h"
#include "messages.h"
#include "monster.h"
#include "line.h"
#include "npc.h"
#include "item.h"
#include "player.h"
#include "path_info.h"
#include "options.h"
#include "time.h"
#include <chrono>
#include <thread>

#define dbg(x) DebugLog((DebugLevel)(x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

weather_type previous_weather;
int prev_hostiles = 0;
int deafness_turns = 0;
int current_deafness_turns = 0;
bool audio_muted = false;
float g_sfx_volume_multiplier = 1;
auto start_sfx_timestamp = std::chrono::high_resolution_clock::now();
auto end_sfx_timestamp = std::chrono::high_resolution_clock::now();
auto sfx_time = end_sfx_timestamp - start_sfx_timestamp;

struct sound_event {
    int volume;
    std::string description;
    bool ambient;
    bool footstep;
    std::string id;
    std::string variant;
};

struct sound_thread {
    tripoint source;
    tripoint target;
    bool hit;
    bool targ_mon;
    std::string material;
};

struct centroid {
    // Values have to be floats to prevent rounding errors.
    float x;
    float y;
    float z;
    float volume;
    float weight;
};

// Static globals tracking sounds events of various kinds.
// The sound events since the last monster turn.
static std::vector<std::pair<tripoint, int>> recent_sounds;
// The sound events since the last interactive player turn. (doesn't count sleep etc)
static std::vector<std::pair<tripoint, sound_event>> sounds_since_last_turn;
// The sound events currently displayed to the player.
static std::unordered_map<tripoint, sound_event> sound_markers;
std::vector<sound_effect> sound_effects_p;

void sounds::ambient_sound( const tripoint &p, int vol, std::string description )
{
    sound( p, vol, description, true );
}

void sounds::sound( const tripoint &p, int vol, std::string description, bool ambient, const std::string& id, const std::string& variant )
{
    if( vol < 0 ) {
        // Bail out if no volume.
        debugmsg( "negative sound volume %d", vol );
        return;
    }
    recent_sounds.emplace_back( std::make_pair( p, vol ) );
    sounds_since_last_turn.emplace_back(
        std::make_pair( p, sound_event {vol, description, ambient, false, id, variant} ) );
}

void sounds::add_footstep( const tripoint &p, int volume, int, monster * )
{
    sounds_since_last_turn.emplace_back(
        std::make_pair( p, sound_event {volume, "", false, true, "", ""} ) );
}

template <typename C>
static void vector_quick_remove( std::vector<C> &source, int index )
{
    if( source.size() != 1 ) {
        // Swap the target and the last element of the vector.
        // This scrambles the vector, but makes removal O(1).
        std::iter_swap( source.begin() + index, source.end() - 1 );
    }
    source.pop_back();
}

static std::vector<centroid> cluster_sounds( std::vector<std::pair<tripoint, int>> recent_sounds )
{
    // If there are too many monsters and too many noise sources (which can be monsters, go figure),
    // applying sound events to monsters can dominate processing time for the whole game,
    // so we cluster sounds and apply the centroids of the sounds to the monster AI
    // to fight the combanatorial explosion.
    std::vector<centroid> sound_clusters;
    const int num_seed_clusters = std::max( std::min( recent_sounds.size(), ( size_t ) 10 ),
                                            ( size_t ) log( recent_sounds.size() ) );
    const size_t stopping_point = recent_sounds.size() - num_seed_clusters;
    const size_t max_map_distance = rl_dist( 0, 0, MAPSIZE * SEEX, MAPSIZE * SEEY );
    // Randomly choose cluster seeds.
    for( size_t i = recent_sounds.size(); i > stopping_point; i-- ) {
        size_t index = rng( 0, i - 1 );
        // The volume and cluster weight are the same for the first element.
        sound_clusters.push_back(
            // Assure the compiler that these int->float conversions are safe.
        {
            ( float ) recent_sounds[index].first.x, ( float ) recent_sounds[index].first.y,
            ( float ) recent_sounds[index].first.z,
            ( float ) recent_sounds[index].second, ( float ) recent_sounds[index].second
        } );
        vector_quick_remove( recent_sounds, index );
    }
    for( const auto &sound_event_pair : recent_sounds ) {
        auto found_centroid = sound_clusters.begin();
        float dist_factor = max_map_distance;
        const auto cluster_end = sound_clusters.end();
        for( auto centroid_iter = sound_clusters.begin(); centroid_iter != cluster_end;
                ++centroid_iter ) {
            // Scale the distance between the two by the max possible distance.
            tripoint centroid_pos { ( int ) centroid_iter->x, ( int ) centroid_iter->y, ( int ) centroid_iter->z };
            const int dist = rl_dist( sound_event_pair.first, centroid_pos );
            if( dist * dist < dist_factor ) {
                found_centroid = centroid_iter;
                dist_factor = dist * dist;
            }
        }
        const float volume_sum = ( float ) sound_event_pair.second + found_centroid->weight;
        // Set the centroid location to the average of the two locations, weighted by volume.
        found_centroid->x = ( float )( ( sound_event_pair.first.x * sound_event_pair.second ) +
                                       ( found_centroid->x * found_centroid->weight ) ) / volume_sum;
        found_centroid->y = ( float )( ( sound_event_pair.first.y * sound_event_pair.second ) +
                                       ( found_centroid->y * found_centroid->weight ) ) / volume_sum;
        found_centroid->z = ( float )( ( sound_event_pair.first.z * sound_event_pair.second ) +
                                       ( found_centroid->z * found_centroid->weight ) ) / volume_sum;
        // Set the centroid volume to the larger of the volumes.
        found_centroid->volume = std::max( found_centroid->volume, ( float ) sound_event_pair.second );
        // Set the centroid weight to the sum of the weights.
        found_centroid->weight = volume_sum;
    }
    return sound_clusters;
}

void sounds::process_sounds()
{
    std::vector<centroid> sound_clusters = cluster_sounds( recent_sounds );
    const int weather_vol = weather_data( g->weather ).sound_attn;
    for( const auto &this_centroid : sound_clusters ) {
        // Since monsters don't go deaf ATM we can just use the weather modified volume
        // If they later get physical effects from loud noises we'll have to change this
        // to use the unmodified volume for those effects.
        const int vol = this_centroid.volume - weather_vol;
        const tripoint source = tripoint( this_centroid.x, this_centroid.y, this_centroid.z );
        // --- Monster sound handling here ---
        // Alert all hordes
        if( vol > 20 && g->get_levz() == 0 ) {
            int sig_power = ( ( vol > 140 ) ? 140 : vol ) - 20;
            const point abs_ms = g->m.getabs( source.x, source.y );
            const point abs_sm = overmapbuffer::ms_to_sm_copy( abs_ms );
            const tripoint target( abs_sm.x, abs_sm.y, source.z );
            overmap_buffer.signal_hordes( target, sig_power );
        }
        // Alert all monsters (that can hear) to the sound.
        for( int i = 0, numz = g->num_zombies(); i < numz; i++ ) {
            monster &critter = g->zombie( i );
            // rl_dist() is faster than critter.has_flag() or critter.can_hear(), so we'll check it first.
            int dist = rl_dist( source, critter.pos3() );
            int vol_goodhearing = vol * 2 - dist;
            if( vol_goodhearing > 0 && critter.can_hear() ) {
                const bool goodhearing = critter.has_flag( MF_GOODHEARING );
                int volume = goodhearing ? vol_goodhearing : ( vol - dist );
                // Error is based on volume, louder sound = less error
                if( volume > 0 ) {
                    int max_error = 0;
                    if( volume < 2 ) {
                        max_error = 10;
                    } else if( volume < 5 ) {
                        max_error = 5;
                    } else if( volume < 10 ) {
                        max_error = 3;
                    } else if( volume < 20 ) {
                        max_error = 1;
                    }
                    int target_x = source.x + rng( -max_error, max_error );
                    int target_y = source.y + rng( -max_error, max_error );
                    // target_z will require some special check due to soil muffling sounds
                    int wander_turns = volume * ( goodhearing ? 6 : 1 );
                    critter.wander_to( tripoint( target_x, target_y, source.z ), wander_turns );
                    critter.process_trigger( MTRIG_SOUND, volume );
                }
            }
        }
    }
    recent_sounds.clear();
}

void sounds::process_sound_markers( player *p )
{
    bool is_deaf = p->is_deaf();
    const float volume_multiplier = p->hearing_ability();
    const int weather_vol = weather_data( g->weather ).sound_attn;
    for( const auto &sound_event_pair : sounds_since_last_turn ) {
        const int volume = sound_event_pair.second.volume * volume_multiplier;
        const std::string& sfx_id = sound_event_pair.second.id;
        const std::string& sfx_variant = sound_event_pair.second.variant;
        const int max_volume = std::max( volume, sound_event_pair.second.volume );  // For deafness checks
        int dist = rl_dist( p->pos3(), sound_event_pair.first );
        bool ambient = sound_event_pair.second.ambient;
        // Too far away, we didn't hear it!
        if( dist > volume ) {
            continue;
        }
        if( is_deaf ) {
            // Has to be here as well to work for stacking deafness (loud noises prolong deafness)
            if( !p->is_immune_effect( "deaf" )
                    && rng( ( max_volume - dist ) / 2, ( max_volume - dist ) ) >= 150 ) {
                // Prolong deafness, but not as much as if it was freshly applied
                int duration = std::min( 40, ( max_volume - dist - 130 ) / 8 );
                p->add_effect( "deaf", duration );
                if( !p->has_trait( "DEADENED" ) ) {
                    p->add_msg_if_player( m_bad, _( "Your eardrums suddenly ache!" ) );
                    if( p->pain < 10 ) {
                        p->mod_pain( rng( 0, 2 ) );
                    }
                }
            }
            // We're deaf, skip rest of processing.
            continue;
        }
        // Player volume meter includes all sounds from their tile and adjacent tiles
        // TODO: Add noises from vehicle player is in.
        if( dist <= 1 ) {
            p->volume = std::max( p->volume, volume );
        }
        // Check for deafness
        if( !p->is_immune_effect( "deaf" )
                && rng( ( max_volume - dist ) / 2, ( max_volume - dist ) ) >= 150 ) {
            int duration = ( max_volume - dist - 130 ) / 4;
            p->add_effect( "deaf", duration );
            sfx::do_hearing_loss_sfx( duration );
            is_deaf = true;
            continue;
        }
        // At this point we are dealing with attention (as opposed to physical effects)
        // so reduce volume by the amount of ambient noise from the weather.
        const int mod_vol = ( sound_event_pair.second.volume - weather_vol ) * volume_multiplier;
        // The noise was drowned out by the surroundings.
        if( mod_vol - dist < 0 ) {
            continue;
        }
        // See if we need to wake someone up
        if( p->has_effect( "sleep" ) ) {
            if( ( !( p->has_trait( "HEAVYSLEEPER" ) ||
                     p->has_trait( "HEAVYSLEEPER2" ) ) && dice( 2, 15 ) < mod_vol - dist ) ||
                    ( p->has_trait( "HEAVYSLEEPER" ) && dice( 3, 15 ) < mod_vol - dist ) ||
                    ( p->has_trait( "HEAVYSLEEPER2" ) && dice( 6, 15 ) < mod_vol - dist ) ) {
                //Not kidding about sleep-thru-firefight
                p->wake_up();
                add_msg( m_warning, _( "Something is making noise." ) );
            } else {
                continue;
            }
        }
        const tripoint &pos = sound_event_pair.first;
        const std::string &description = sound_event_pair.second.description;
        if( !ambient && ( pos != p->pos3() ) && !g->m.pl_sees( pos, dist ) ) {
            if( p->activity.ignore_trivial != true ) {
                std::string query;
                if( description.empty() ) {
                    query = _( "Heard a noise!" );
                } else {
                    query = string_format( _( "Heard %s!" ),
                                           sound_event_pair.second.description.c_str() );
                }
                if( g->cancel_activity_or_ignore_query( query.c_str() ) ) {
                    p->activity.ignore_trivial = true;
                    for( auto activity : p->backlog ) {
                        activity.ignore_trivial = true;
                    }
                }
            }
        }
        // Only print a description if it exists
        if( !description.empty() ) {
            // If it came from us, don't print a direction
            if( pos == p->pos3() ) {
                std::string uppercased = description;
                capitalize_letter( uppercased, 0 );
                add_msg( "%s", uppercased.c_str() );
            } else {
                // Else print a direction as well
                std::string direction = direction_name( direction_from( p->pos3(), pos ) );
                add_msg( m_warning, _( "From the %s you hear %s" ), direction.c_str(), description.c_str() );
            }
        }
        // Play the sound effect, if any.
        if( !sfx_id.empty() ) {
            // for our sfx API, 100 is "normal" volume, so scale accordingly
            int heard_volume = sfx::get_heard_volume( pos );
            play_sound_effect( sfx_id, sfx_variant, heard_volume );
            //add_msg("Playing sound effect %s, %s, %d", sfx_id.c_str(), sfx_variant.c_str(), heard_volume);
        }
        // If Z coord is different, draw even when you can see the source
        const bool diff_z = pos.z != p->posz();
        // Place footstep markers.
        if( pos == p->pos3() || p->sees( pos ) ) {
            // If we are or can see the source, don't draw a marker.
            continue;
        }
        int err_offset;
        if( mod_vol / dist < 2 ) {
            err_offset = 3;
        } else if( mod_vol / dist < 3 ) {
            err_offset = 2;
        } else {
            err_offset = 1;
        }
        // Enumerate the valid points the player *cannot* see.
        // Unless the source is on a different z-level, then any point is fine
        std::vector<tripoint> unseen_points;
        tripoint newp = pos;
        int &newx = newp.x;
        int &newy = newp.y;
        for( newx = pos.x - err_offset; newx <= pos.x + err_offset; newx++ ) {
            for( newy = pos.y - err_offset; newy <= pos.y + err_offset; newy++ ) {
                if( diff_z || !p->sees( newp ) ) {
                    unseen_points.emplace_back( newp );
                }
            }
        }
        // Then place the sound marker in a random one.
        if( !unseen_points.empty() ) {
            sound_markers.emplace(
                std::make_pair( unseen_points[rng( 0, unseen_points.size() - 1 )],
                                sound_event_pair.second ) );
        }
    }
    sounds_since_last_turn.clear();
}

void sounds::reset_sounds()
{
    recent_sounds.clear();
    sounds_since_last_turn.clear();
    sound_markers.clear();
}

void sounds::reset_markers()
{
    sound_markers.clear();
}

void sounds::draw_monster_sounds( const tripoint &offset, WINDOW *window )
{
    auto sound_clusters = cluster_sounds( recent_sounds );
    // TODO: Signal sounds on different Z-levels differently (with '^' and 'v'?)
    for( const auto &sound : recent_sounds ) {
        mvwputch( window, offset.y + sound.first.y, offset.x + sound.first.x, c_yellow, '?' );
    }
    for( const auto &sound : sound_clusters ) {
        mvwputch( window, offset.y + sound.y, offset.x + sound.x, c_red, '?' );
    }
}

std::vector<tripoint> sounds::get_footstep_markers()
{
    // Optimization, make this static and clear it in reset_markers?
    std::vector<tripoint> footsteps;
    footsteps.reserve( sound_markers.size() );
    for( const auto &mark : sound_markers ) {
        footsteps.push_back( mark.first );
    }
    return footsteps;
}

std::pair<std::vector<tripoint>, std::vector<tripoint>> sounds::get_monster_sounds()
{
    auto sound_clusters = cluster_sounds( recent_sounds );
    std::vector<tripoint> sound_locations;
    sound_locations.reserve( recent_sounds.size() );
    for( const auto &sound : recent_sounds ) {
        sound_locations.push_back( sound.first );
    }
    std::vector<tripoint> cluster_centroids;
    cluster_centroids.reserve( sound_clusters.size() );
    for( const auto &sound : sound_clusters ) {
        cluster_centroids.emplace_back( sound.x, sound.y, sound.z );
    }
    return { sound_locations, cluster_centroids };
}

std::string sounds::sound_at( const tripoint &location )
{
    auto this_sound = sound_markers.find( location );
    if( this_sound == sound_markers.end() ) {
        return std::string();
    }
    if( !this_sound->second.description.empty() ) {
        return this_sound->second.description;
    }
    return _( "a sound" );
}

//sfx::
void sfx::load_sound_effects( JsonObject &jsobj ) {
    set_group_channels( 2, 9, 1 );
    set_group_channels( 0, 1, 2 );
    set_group_channels( 11, 14, 3 );
    set_group_channels( 15, 17, 4 );
    int index = 0;
    std::string file;
    sound_effect new_sound_effect;
    new_sound_effect.id = jsobj.get_string( "id" );
    new_sound_effect.volume = jsobj.get_int( "volume" );
    new_sound_effect.variant = jsobj.get_string( "variant" );
    JsonArray jsarr = jsobj.get_array( "files" );
    while( jsarr.has_more() ) {
        new_sound_effect.files.push_back( jsarr.next_string().c_str() );
        file = new_sound_effect.files[index];
        index++;
        std::string path = ( FILENAMES[ "datadir" ] + "/sound/" + file );
        Mix_Chunk *loaded_chunk = Mix_LoadWAV( path.c_str() );
        if( !loaded_chunk ) {
            dbg( D_ERROR ) << "Failed to load audio file " << path << ": " << Mix_GetError();
        }
        new_sound_effect.chunk = loaded_chunk;
        sound_effects_p.push_back( new_sound_effect );
    }
}

int sfx::get_channel( Mix_Chunk * effect_to_play ) {
    return Mix_PlayChannel( -1, effect_to_play, 0 );
}

Mix_Chunk *do_pitch_shift( Mix_Chunk *s, float pitch ) {
    Mix_Chunk *result;
    Uint32 s_in = s->alen / 4;
    Uint32 s_out = ( Uint32 )( ( float )s_in * pitch );
    float pitch_real = ( float )s_out / ( float )s_in;
    Uint32 i, j;
    result = ( Mix_Chunk * )malloc( sizeof( Mix_Chunk ) );
    result->allocated = 1;
    result->alen = s_out * 4;
    result->abuf = ( Uint8* )malloc( result->alen * sizeof( Uint8 ) );
    result->volume = s->volume;
    for( i = 0; i < s_out; i++ ) {
        Sint16 lt;
        Sint16 rt;
        Sint16 lt_out;
        Sint16 rt_out;
        Sint64 lt_avg = 0;
        Sint64 rt_avg = 0;
        Uint32 begin = ( Uint32 )( ( float )i / pitch_real );
        Uint32 end = ( Uint32 )( ( float )( i + 1 ) / pitch_real );
        for( j = begin; j <= end; j++ ) {
            lt = ( s->abuf[( 4 * j ) + 1] << 8 ) | ( s->abuf[( 4 * j ) + 0] );
            rt = ( s->abuf[( 4 * j ) + 3] << 8 ) | ( s->abuf[( 4 * j ) + 2] );
            lt_avg += lt;
            rt_avg += rt;
        }
        lt_out = ( Sint16 )( ( float )lt_avg / ( float )( end - begin + 1 ) );
        rt_out = ( Sint16 )( ( float )rt_avg / ( float )( end - begin + 1 ) );
        result->abuf[( 4 * i ) + 1] = ( lt_out >> 8 ) & 0xFF;
        result->abuf[( 4 * i ) + 0] = lt_out & 0xFF;
        result->abuf[( 4 * i ) + 3] = ( rt_out >> 8 ) & 0xFF;
        result->abuf[( 4 * i ) + 2] = rt_out & 0xFF;
    }
    return result;
}

void sfx::play_variant_sound( std::string id, std::string variant, int volume, int angle,
                              float pitch_min, float pitch_max ) {
    if( volume == 0 ) {
        return;
    }
    std::vector<sound_effect> valid_sound_effects;
    sound_effect selected_sound_effect;
    for( auto &i : sound_effects_p ) {
        if( ( i.id == id ) && ( i.variant == variant ) ) {
            valid_sound_effects.push_back( i );
        }
    }
    if( valid_sound_effects.empty() ) {
        for( auto &i : sound_effects_p ) {
            if( ( i.id == id ) && ( i.variant == "default" ) ) {
                valid_sound_effects.push_back( i );
            }
        }
    }
    if( valid_sound_effects.empty() ) {
        return;
    }
    int index = rng( 0, valid_sound_effects.size() - 1 );
    selected_sound_effect = valid_sound_effects[index];
    //add_msg ( m_warning, _ ( "rng 1: %i" ), index );
    Mix_Chunk *effect_to_play = selected_sound_effect.chunk;
    float pitch_random = rng_float( pitch_min, pitch_max );
    Mix_Chunk *shifted_effect = do_pitch_shift( effect_to_play, pitch_random );
    Mix_VolumeChunk( shifted_effect,
                     selected_sound_effect.volume * OPTIONS["SOUND_EFFECT_VOLUME"] * volume / ( 100 * 100 ) );
    int channel = get_channel( shifted_effect );
    Mix_SetPosition( channel, angle, 1 );
}

void sfx::play_ambient_variant_sound( std::string id, std::string variant, int volume, int channel,
                                      int duration ) {
    if( volume == 0 ) {
        return;
    }
    std::vector<sound_effect> valid_sound_effects;
    sound_effect selected_sound_effect;
    for( auto &i : sound_effects_p ) {
        if( ( i.id == id ) && ( i.variant == variant ) ) {
            valid_sound_effects.push_back( i );
        }
    }
    if( valid_sound_effects.empty() ) {
        for( auto &i : sound_effects_p ) {
            if( ( i.id == id ) && ( i.variant == "default" ) ) {
                valid_sound_effects.push_back( i );
            }
        }
    }
    if( valid_sound_effects.empty() ) {
        return;
    }
    int index = rng( 0, valid_sound_effects.size() - 1 );
    selected_sound_effect = valid_sound_effects[index];
    //add_msg ( m_warning, _ ( "rng: %i" ), index );
    Mix_Chunk *effect_to_play = selected_sound_effect.chunk;
    Mix_VolumeChunk( effect_to_play,
                     selected_sound_effect.volume * OPTIONS["SOUND_EFFECT_VOLUME"] * volume / ( 100 * 100 ) );
    if( Mix_FadeInChannel( channel, effect_to_play, -1, duration ) == -1 ) {
        dbg( D_ERROR ) << "Failed to play sound effect: " << Mix_GetError();
    }
}

void sfx::fade_audio_group( int tag, int duration ) {
#ifdef SDL_SOUND
    Mix_FadeOutGroup( tag, duration );
#else
    ( void ) tag;
    ( void ) duration;
#endif
}

void sfx::fade_audio_channel( int tag, int duration ) {
#ifdef SDL_SOUND
    Mix_FadeOutChannel( tag, duration );
#else
    ( void ) tag;
    ( void ) duration;
#endif
}

void sfx::set_group_channels( int from, int to, int tag ) {
#ifdef SDL_SOUND
    Mix_GroupChannels( from, to, tag );
#else
    ( void ) from;
    ( void ) to;
    ( void ) tag;
#endif
}

int sfx::is_channel_playing( int channel ) {
#ifdef SDL_SOUND
    return Mix_Playing( channel );
#else
    ( void ) channel;
#endif
}

void sfx::stop_sound_effect_fade( int channel, int duration ) {
#ifdef SDL_SOUND
    if( Mix_FadeOutChannel( channel, duration ) == -1 ) {
        dbg( D_ERROR ) << "Failed to stop sound effect: " << Mix_GetError();
    }
#else
    ( void ) id;
    ( void ) variant;
    ( void ) volume;
    ( void ) loops;
#endif
}

void sfx::do_ambient_sfx() {
    /* Channel assignments:
    0: Daytime outdoors env
    1: Nighttime outdoors env
    2: Underground env
    3: Indoors env
    4: Indoors rain env
    5: Outdoors snow env
    6: Outdoors flurry env
    7: Outdoors thunderstorm env
    8: Outdoors rain env
    9: Outdoors drizzle env
    10: Deafness tone
    11: Danger high theme
    12: Danger medium theme
    13: Danger low theme
    14: Danger extreme theme
    15: Stamina 75%
    16: Stamina 50%
    17: Stamina 35%
    18: Idle chainsaw
    19: Chainsaw theme
    Group Assignments:
    1: SFX related to weather
    2: SFX related to time of day
    3: SFX related to context themes
    4: SFX related to Fatigue
    */
    if( g->u.in_sleep_state() && !audio_muted ) {
        fade_audio_channel( -1, 300 );
        audio_muted = true;
        return;
    } else if( g->u.in_sleep_state() && audio_muted ) {
        return;
    }
    audio_muted = false;
    // Check weather at player position
    w_point weather_at_player = g->weatherGen.get_weather( g->u.global_square_location(),
                                calendar::turn );
    g->weather = g->weatherGen.get_weather_conditions( weather_at_player );
    // Step in at night time / we are not indoors
    if( calendar::turn.is_night() && !g->is_sheltered( g->u.pos() ) &&
            is_channel_playing( 1 ) != 1 && !g->u.get_effect_int( "deaf" ) > 0 ) {
        fade_audio_group( 2, 1000 );
        play_ambient_variant_sound( "environment", "nighttime", get_heard_volume( g->u.pos() ), 1, 1000 );
        // Step in at day time / we are not indoors
    } else if( !calendar::turn.is_night() && is_channel_playing( 0 ) != 1 &&
               !g->is_sheltered( g->u.pos() ) && !g->u.get_effect_int( "deaf" ) > 0 ) {
        fade_audio_group( 2, 1000 );
        play_ambient_variant_sound( "environment", "daytime", get_heard_volume( g->u.pos() ), 0, 1000 );
    }
    // We are underground
    if( ( g->is_underground( g->u.pos() ) && is_channel_playing( 2 ) != 1 &&
            !g->u.get_effect_int( "deaf" ) > 0 ) || ( g->is_underground( g->u.pos() ) &&
                    g->weather != previous_weather && !g->u.get_effect_int( "deaf" ) > 0 ) ) {
        fade_audio_group( 1, 1000 );
        fade_audio_group( 2, 1000 );
        play_ambient_variant_sound( "environment", "underground", get_heard_volume( g->u.pos() ), 2,
                                    1000 );
        // We are indoors
    } else if( ( g->is_sheltered( g->u.pos() ) && !g->is_underground( g->u.pos() ) &&
                 is_channel_playing( 3 ) != 1 && !g->u.get_effect_int( "deaf" ) > 0 ) ||
               ( g->is_sheltered( g->u.pos() ) && !g->is_underground( g->u.pos() ) &&
                 g->weather != previous_weather && !g->u.get_effect_int( "deaf" ) > 0 ) ) {
        fade_audio_group( 1, 1000 );
        fade_audio_group( 2, 1000 );
        play_ambient_variant_sound( "environment", "indoors", get_heard_volume( g->u.pos() ), 3, 1000 );
    }
    // We are indoors and it is also raining
    if( g->weather >= WEATHER_DRIZZLE && g->weather <= WEATHER_ACID_RAIN && !g->is_underground( g->u.pos() )
            && is_channel_playing( 4 ) != 1 ) {
        play_ambient_variant_sound( "environment", "indoors_rain", get_heard_volume( g->u.pos() ), 4,
                                    1000 );
    }
    if( ( !g->is_sheltered( g->u.pos() ) && g->weather != WEATHER_CLEAR
            && is_channel_playing( 5 ) != 1  &&
            is_channel_playing( 6 ) != 1  && is_channel_playing( 7 ) != 1  && is_channel_playing( 8 ) != 1
            &&
            is_channel_playing( 9 ) != 1  && !g->u.get_effect_int( "deaf" ) > 0 )
            || ( !g->is_sheltered( g->u.pos() ) &&
                 g->weather != previous_weather  && !g->u.get_effect_int( "deaf" ) > 0 ) ) {
        fade_audio_group( 1, 1000 );
        // We are outside and there is precipitation
        switch( g->weather ) {
	case WEATHER_ACID_DRIZZLE:        
	case WEATHER_DRIZZLE:
            play_ambient_variant_sound( "environment", "WEATHER_DRIZZLE", get_heard_volume( g->u.pos() ), 9,
                                        1000 );
            break;
        case WEATHER_RAINY:
            play_ambient_variant_sound( "environment", "WEATHER_RAINY", get_heard_volume( g->u.pos() ), 8,
                                        1000 );
            break;
	case WEATHER_ACID_RAIN:
	case WEATHER_THUNDER:
        case WEATHER_LIGHTNING:
            play_ambient_variant_sound( "environment", "WEATHER_THUNDER", get_heard_volume( g->u.pos() ), 7,
                                        1000 );
            break;
        case WEATHER_FLURRIES:
            play_ambient_variant_sound( "environment", "WEATHER_FLURRIES", get_heard_volume( g->u.pos() ), 6,
                                        1000 );
            break;
	// I didn't know where to put these or even if the first two were needed. They probably shouldn't make any noise, but I was getting an error when compiling when I left them out.
	case WEATHER_NULL:
	case NUM_WEATHER_TYPES:        
	case WEATHER_CLEAR:
	case WEATHER_SUNNY:
	case WEATHER_CLOUDY:
        case WEATHER_SNOWSTORM:
        case WEATHER_SNOW:
            play_ambient_variant_sound( "environment", "WEATHER_SNOW", get_heard_volume( g->u.pos() ), 5,
                                        1000 );
            break;
        }
    }
    // Keep track of weather to compare for next iteration
    previous_weather = g->weather;
}

int sfx::get_heard_volume( const tripoint source ) {
    int distance = rl_dist( g->u.pos3(), source );
    // fract = -100 / 24
    const float fract = -4.166666;
    int heard_volume = fract * distance - 1 + 100;
    if( heard_volume <= 0 ) {
        heard_volume = 0;
    }
    heard_volume *= g_sfx_volume_multiplier;
    return ( heard_volume );
}

int sfx::get_heard_angle( const tripoint source ) {
    int angle = g->m.coord_to_angle( g->u.posx(), g->u.posy(), source.x, source.y ) + 90;
    //add_msg(m_warning, "angle: %i", angle);
    return ( angle );
}

void sfx::generate_gun_soundfx( const tripoint source ) {

    end_sfx_timestamp = std::chrono::high_resolution_clock::now();
    sfx_time = end_sfx_timestamp - start_sfx_timestamp;
    if( std::chrono::duration_cast<std::chrono::milliseconds> ( sfx_time ).count() < 80 ) {
        return;
    }
    int heard_volume = get_heard_volume( source );
    if( heard_volume <= 30 ) {
        heard_volume = 30;
    }
    int angle = get_heard_angle( source );
    int distance = rl_dist( g->u.pos3(), source );
    if( source == g->u.pos3() ) {
        itype_id weapon_id = g->u.weapon.typeId();
        std::string weapon_type = g->u.weapon.gun_skill();
        std::string selected_sound = "fire_gun";
        if( g->u.weapon.has_gunmod( "suppressor" ) == 1
                || g->u.weapon.has_gunmod( "homemade suppressor" ) == 1 ) {
            selected_sound = "fire_gun";
            weapon_id = "weapon_fire_suppressed";
        }
        play_variant_sound( selected_sound, weapon_id, heard_volume, 0, 0.8, 1.2 );
        start_sfx_timestamp = std::chrono::high_resolution_clock::now();
        return;
    }
    if( g->npc_at( source ) != -1 ) {
        npc *npc_source = g->active_npc[ g->npc_at( source ) ];
        if( distance <= 17 ) {
            itype_id weapon_id = npc_source->weapon.typeId();
            play_variant_sound( "fire_gun", weapon_id, heard_volume, angle, 0.8, 1.2 );
            start_sfx_timestamp = std::chrono::high_resolution_clock::now();
            return;
        } else {
            std::string weapon_type = npc_source->weapon.gun_skill();
            play_variant_sound( "fire_gun_distant", weapon_type, heard_volume, angle, 0.8, 1.2 );
            start_sfx_timestamp = std::chrono::high_resolution_clock::now();
            return;
        }
    }
    int mon_pos = g->mon_at( source );
    if( mon_pos != -1 ) {
        monster *monster = g->monster_at( source );
        std::string monster_id = monster->type->id;
        if( distance <= 18 ) {
            if( monster_id == "mon_turret" || monster_id == "mon_secubot" ) {
                play_variant_sound( "fire_gun", "hk_mp5", heard_volume, angle, 0.8, 1.2 );
                start_sfx_timestamp = std::chrono::high_resolution_clock::now();
                return;
            } else if( monster_id == "mon_turret_rifle" || monster_id == "mon_chickenbot"
                       || monster_id == "mon_tankbot" ) {
                play_variant_sound( "fire_gun", "m4a1", heard_volume, angle, 0.8, 1.2 );
                start_sfx_timestamp = std::chrono::high_resolution_clock::now();
                return;
            } else if( monster_id == "mon_turret_bmg" ) {
                play_variant_sound( "fire_gun", "m2browning", heard_volume, angle, 0.8, 1.2 );
                start_sfx_timestamp = std::chrono::high_resolution_clock::now();
                return;
            } else if( monster_id == "mon_laserturret" ) {
                play_variant_sound( "fire_gun", "laser_rifle", heard_volume, angle, 0.8, 1.2 );
                start_sfx_timestamp = std::chrono::high_resolution_clock::now();
                return;
            }
        } else {
            if( monster_id == "mon_turret" || monster_id == "mon_secubot" ) {
                play_variant_sound( "fire_gun_distant", "smg", heard_volume, angle, 0.8, 1.2 );
                start_sfx_timestamp = std::chrono::high_resolution_clock::now();
                return;
            } else if( monster_id == "mon_turret_rifle" || monster_id == "mon_chickenbot"
                       || monster_id == "mon_tankbot" ) {
                play_variant_sound( "fire_gun_distant", "rifle", heard_volume, angle, 0.8, 1.2 );
                start_sfx_timestamp = std::chrono::high_resolution_clock::now();
                return;
            } else if( monster_id == "mon_turret_bmg" ) {
                play_variant_sound( "fire_gun_distant", "rifle", heard_volume, angle, 0.8, 1.2 );
                start_sfx_timestamp = std::chrono::high_resolution_clock::now();
                return;
            } else if( monster_id == "mon_laserturret" ) {
                play_variant_sound( "fire_gun_distant", "laser", heard_volume, angle, 0.8, 1.2 );
                start_sfx_timestamp = std::chrono::high_resolution_clock::now();
                return;
            }
        }
    }
}

void sfx::generate_melee_soundfx( tripoint source, tripoint target, bool hit, bool targ_mon,
                                  std::string material ) {
    sound_thread * out = new sound_thread();
    out->source = source;
    out->target = target;
    out->hit = hit;
    out->targ_mon = targ_mon;
    out->material = material;
    //pthread_t thread1;
    //pthread_create( &thread1, NULL, generate_melee_soundfx_thread, out );
    return;
}

void *sfx::generate_melee_soundfx_thread( void * out ) {
    std::srand( time( NULL ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( rng( 1, 2 ) ) );
    sound_thread *in = ( sound_thread* ) out;
    bool hit = in->hit;
    bool targ_mon = in->targ_mon;
    tripoint source = in->source;
    tripoint target = in->target;
    std::string material = in->material;
    std::string variant_used;
    int npc_index = g->npc_at( source );
    if( npc_index == -1 ) {
        std::string weapon_skill = g->u.weapon.weap_skill();
        int weapon_volume = g->u.weapon.volume();
        if( weapon_skill == "bashing" && weapon_volume <= 8 ) {
            variant_used = "small_bash";
            play_variant_sound( "melee_swing", "small_bash", get_heard_volume( source ), 0.8, 1.2 );
        } else if( weapon_skill == "bashing" && weapon_volume >= 9 ) {
            variant_used = "big_bash";
            play_variant_sound( "melee_swing", "big_bash", get_heard_volume( source ), 0.8, 1.2 );
        } else if( ( weapon_skill == "cutting" || weapon_skill == "stabbing" ) && weapon_volume <= 6 ) {
            variant_used = "small_cutting";
            play_variant_sound( "melee_swing", "small_cutting", get_heard_volume( source ), 0.8, 1.2 );
        } else if( ( weapon_skill == "cutting" || weapon_skill == "stabbing" ) && weapon_volume >= 7 ) {
            variant_used = "big_cutting";
            play_variant_sound( "melee_swing", "big_cutting", get_heard_volume( source ), 0.8, 1.2 );
        } else {
            variant_used = "default";
            play_variant_sound( "melee_swing", "default", get_heard_volume( source ), 0.8, 1.2 );
        }
        if( hit ) {
            if( targ_mon ) {
                if( material == "steel" ) {
                    std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 12,
                                                 weapon_volume * 16 ) ) );
                    play_variant_sound( "melee_hit_metal", variant_used, get_heard_volume( source ),
                                        get_heard_angle( target ), 0.8, 1.2 );
                } else {
                    std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 12,
                                                 weapon_volume * 16 ) ) );
                    play_variant_sound( "melee_hit_flesh", variant_used, get_heard_volume( source ),
                                        get_heard_angle( target ), 0.8, 1.2 );
                }
            } else {
                std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 9,
                                             weapon_volume * 12 ) ) );
                play_variant_sound( "melee_hit_flesh", variant_used, get_heard_volume( source ),
                                    get_heard_angle( target ), 0.8, 1.2 );
            }
        }
    } else {
        npc *p = g->active_npc[npc_index];
        std::string weapon_skill = p->weapon.weap_skill();
        int weapon_volume = p->weapon.volume();
        if( weapon_skill == "bashing" && weapon_volume <= 8 ) {
            variant_used = "small_bash";
            play_variant_sound( "melee_swing", "small_bash", get_heard_volume( source ) - 30,
                                get_heard_angle( source ), 0.8, 1.2 );
        } else if( weapon_skill == "bashing" && weapon_volume >= 9 ) {
            variant_used = "big_bash";
            play_variant_sound( "melee_swing", "big_bash", get_heard_volume( source ) - 30,
                                get_heard_angle( source ), 0.8, 1.2 );
        } else if( ( weapon_skill == "cutting" || weapon_skill == "stabbing" ) && weapon_volume <= 6 ) {
            variant_used = "small_cutting";
            play_variant_sound( "melee_swing", "small_cutting", get_heard_volume( source ) - 30,
                                get_heard_angle( source ), 0.8, 1.2 );
        } else if( ( weapon_skill == "cutting" || weapon_skill == "stabbing" ) && weapon_volume >= 7 ) {
            variant_used = "big_cutting";
            play_variant_sound( "melee_swing", "big_cutting", get_heard_volume( source ) - 30,
                                get_heard_angle( source ), 0.8, 1.2 );
        } else {
            variant_used = "default";
            play_variant_sound( "melee_swing", "default", get_heard_volume( source ) - 30,
                                get_heard_angle( source ), 0.8, 1.2 );
        }
        if( hit ) {
            if( targ_mon ) {
                if( material == "steel" ) {
                    std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 12,
                                                 weapon_volume * 16 ) ) );
                    play_variant_sound( "melee_hit_metal", variant_used, get_heard_volume( source ) - 20,
                                        get_heard_angle( target ), 0.8, 1.2 );
                } else {
                    std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 12,
                                                 weapon_volume * 16 ) ) );
                    play_variant_sound( "melee_hit_flesh", variant_used, get_heard_volume( source ) - 20,
                                        get_heard_angle( target ), 0.8, 1.2 );
                }
            } else {
                std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 9,
                                             weapon_volume * 12 ) ) );
                play_variant_sound( "melee_hit_flesh", variant_used, get_heard_volume( source ) - 20,
                                    get_heard_angle( target ), 0.8, 1.2 );
            }
        }
    }
    return 0;
}

void sfx::do_projectile_hit_sfx( const Creature *target ) {
    std::string selected_sound;
    int heard_volume;
    if( !target->is_npc() && !target->is_player() ) {
        const monster *mon = dynamic_cast<const monster *>( target );
        heard_volume = get_heard_volume( target->pos3() );
        int angle = get_heard_angle( mon->pos3() );
        const auto material = mon->get_material();
        static std::set<mat_type> const fleshy = {
            mat_type( "flesh" ),
            mat_type( "hflesh" ),
            mat_type( "iflesh" ),
            mat_type( "veggy" ),
            mat_type( "bone" ),
            mat_type( "protoplasmic" ),
        };
        if( fleshy.count( material ) > 0 || mon->has_flag( MF_VERMIN ) ) {
            play_variant_sound( "bullet_hit", "hit_flesh", heard_volume, angle, 0.8, 1.2 );
            return;
        } else if( mon->get_material() == "stone" ) {
            play_variant_sound( "bullet_hit", "hit_wall", heard_volume, angle, 0.8, 1.2 );
            return;
        } else if( mon->get_material() == "steel" ) {
            play_variant_sound( "bullet_hit", "hit_metal", heard_volume, angle, 0.8, 1.2 );
            return;
        } else {
            play_variant_sound( "bullet_hit", "hit_flesh", heard_volume, angle, 0.8, 1.2 );
            return;
        }
    }
    heard_volume = sfx::get_heard_volume( target->pos() );
    int angle = get_heard_angle( target->pos3() );
    play_variant_sound( "bullet_hit", "hit_flesh", heard_volume, angle, 0.8, 1.2 );
}

void sfx::do_player_death_hurt_sfx( bool gender, bool death ) {
    int heard_volume = get_heard_volume( g->u.pos() );
    if( !gender && !death ) {
        play_variant_sound( "deal_damage", "hurt_f", heard_volume );
    } else if( gender && !death ) {
        play_variant_sound( "deal_damage", "hurt_m", heard_volume );
    } else if( !gender && death ) {
        play_variant_sound( "clean_up_at_end", "death_f", heard_volume );
    } else if( gender && death ) {
        play_variant_sound( "clean_up_at_end", "death_m", heard_volume );
    }
}

void sfx::do_danger_music() {
    if( g->u.in_sleep_state() && !audio_muted ) {
        fade_audio_channel( -1, 100 );
        audio_muted = true;
        return;
    } else if( ( g->u.in_sleep_state() && audio_muted ) || is_channel_playing( 19 ) == 1 ) {
        fade_audio_group( 3, 1000 );
        return;
    }
    audio_muted = false;
    int hostiles = 0;
    for( auto &critter : g->u.get_visible_creatures( 40 ) ) {
        if( g->u.attitude_to( *critter ) == Creature::A_HOSTILE ) {
            hostiles++;
        }
    }
    if( hostiles == prev_hostiles ) {
        return;
    }
    if( hostiles <= 4 ) {
        fade_audio_group( 3, 1000 );
        prev_hostiles = hostiles;
        return;
    } else if( hostiles >= 5 && hostiles <= 9 && !is_channel_playing( 13 ) ) {
        fade_audio_group( 3, 1000 );
        play_ambient_variant_sound( "danger_low", "default", 100, 13, 1000 );
        prev_hostiles = hostiles;
        return;
    } else if( hostiles >= 10 && hostiles <= 14 && !is_channel_playing( 12 ) ) {
        fade_audio_group( 3, 1000 );
        play_ambient_variant_sound( "danger_medium", "default", 100, 12, 1000 );
        prev_hostiles = hostiles;
        return;
    } else if( hostiles >= 15 && hostiles <= 19 && !is_channel_playing( 11 ) ) {
        fade_audio_group( 3, 1000 );
        play_ambient_variant_sound( "danger_high", "default", 100, 11, 1000 );
        prev_hostiles = hostiles;
        return;
    } else if( hostiles >= 20 && !is_channel_playing( 14 ) ) {
        fade_audio_group( 3, 1000 );
        play_ambient_variant_sound( "danger_extreme", "default", 100, 14, 1000 );
        prev_hostiles = hostiles;
        return;
    }
    prev_hostiles = hostiles;
}

void sfx::do_fatigue_sfx() {
    /*15: Stamina 75%
    16: Stamina 50%
    17: Stamina 25%*/
    if( g->u.stamina >=  g->u.get_stamina_max() * .75 ) {
        fade_audio_group( 4, 2000 );
        return;
    } else if( g->u.stamina <=  g->u.get_stamina_max() * .74
               && g->u.stamina >=  g->u.get_stamina_max() * .5 &&
               g->u.male && !is_channel_playing( 15 ) ) {
        fade_audio_group( 4, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_m_low", 100, 15, 1000 );
        return;
    } else if( g->u.stamina <=  g->u.get_stamina_max() * .49
               && g->u.stamina >=  g->u.get_stamina_max() * .25 &&
               g->u.male && !is_channel_playing( 16 ) ) {
        fade_audio_group( 4, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_m_med", 100, 16, 1000 );
        return;
    } else if( g->u.stamina <=  g->u.get_stamina_max() * .24 && g->u.stamina >=  0 &&
               g->u.male && !is_channel_playing( 17 ) ) {
        fade_audio_group( 4, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_m_high", 100, 17, 1000 );
        return;
    } else if( g->u.stamina <=  g->u.get_stamina_max() * .74
               && g->u.stamina >=  g->u.get_stamina_max() * .5 &&
               !g->u.male && !is_channel_playing( 15 ) ) {
        fade_audio_group( 4, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_f_low", 100, 15, 1000 );
        return;
    } else if( g->u.stamina <=  g->u.get_stamina_max() * .49
               && g->u.stamina >=  g->u.get_stamina_max() * .25 &&
               !g->u.male && !is_channel_playing( 16 ) ) {
        fade_audio_group( 4, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_f_med", 100, 16, 1000 );
        return;
    } else if( g->u.stamina <=  g->u.get_stamina_max() * .24 && g->u.stamina >=  0 &&
               !g->u.male && !is_channel_playing( 17 ) ) {
        fade_audio_group( 4, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_f_high", 100, 17, 1000 );
        return;
    }
}

void sfx::do_hearing_loss_sfx( int turns ) {
    if( deafness_turns == 0 ) {
        deafness_turns = turns;
        g_sfx_volume_multiplier = .1;
        fade_audio_group( 1, 50 );
        fade_audio_group( 2, 50 );
        play_variant_sound( "environment", "deafness_shock", 100 );
        play_variant_sound( "environment", "deafness_tone_start", 100 );
        if( deafness_turns <= 35 ) {
            play_ambient_variant_sound( "environment", "deafness_tone_light", 90, 10, 100 );
        } else if( deafness_turns <= 90 ) {
            play_ambient_variant_sound( "environment", "deafness_tone_medium", 90, 10, 100 );
        } else if( deafness_turns >= 91 ) {
            play_ambient_variant_sound( "environment", "deafness_tone_heavy", 90, 10, 100 );
        }
    } else {
        deafness_turns += turns;
    }
}

void sfx::remove_hearing_loss_sfx() {
    if( current_deafness_turns >= deafness_turns ) {
        stop_sound_effect_fade( 10, 300 );
        g_sfx_volume_multiplier = 1;
        deafness_turns = 0;
        current_deafness_turns = 0;
        do_ambient_sfx();
    }
    current_deafness_turns++;
}

void sfx::do_footstep_sfx() {
    end_sfx_timestamp = std::chrono::high_resolution_clock::now();
    sfx_time = end_sfx_timestamp - start_sfx_timestamp;
    if( std::chrono::duration_cast<std::chrono::milliseconds> ( sfx_time ).count() > 400 ) {
        int heard_volume = sfx::get_heard_volume( g->u.pos() );
        const auto terrain = g->m.ter_at( g->u.pos() ).id;
        static std::set<ter_type> const grass = {
            ter_type( "t_grass" ),
            ter_type( "t_shrub" ),
            ter_type( "t_underbrush" ),
        };
        static std::set<ter_type> const dirt = {
            ter_type( "t_dirt" ),
            ter_type( "t_sand" ),
            ter_type( "t_dirtfloor" ),
            ter_type( "t_palisade_gate_o" ),
            ter_type( "t_sandbox" ),
        };
        static std::set<ter_type> const metal = {
            ter_type( "t_ov_smreb_cage" ),
            ter_type( "t_metal_floor" ),
            ter_type( "t_grate" ),
            ter_type( "t_bridge" ),
            ter_type( "t_elevator" ),
            ter_type( "t_guardrail_bg_dp" ),
        };
        static std::set<ter_type> const water = {
            ter_type( "t_water_sh" ),
            ter_type( "t_water_dp" ),
            ter_type( "t_swater_sh" ),
            ter_type( "t_swater_dp" ),
            ter_type( "t_water_pool" ),
            ter_type( "t_sewage" ),
        };
        static std::set<ter_type> const chain_fence = {
            ter_type( "t_chainfence_h" ),
            ter_type( "t_chainfence_v" ),
        };
        if( !g->u.wearing_something_on( bp_foot_l ) ) {
            play_variant_sound( "plmove", "walk_barefoot", heard_volume, 0, 0.8, 1.2 );
            start_sfx_timestamp = std::chrono::high_resolution_clock::now();
            return;
        } else if( grass.count( terrain ) > 0 ) {
            play_variant_sound( "plmove", "walk_grass", heard_volume, 0, 0.8, 1.2 );
            start_sfx_timestamp = std::chrono::high_resolution_clock::now();
            return;
        } else if( dirt.count( terrain ) > 0 ) {
            play_variant_sound( "plmove", "walk_dirt", heard_volume, 0, 0.8, 1.2 );
            start_sfx_timestamp = std::chrono::high_resolution_clock::now();
            return;
        } else if( metal.count( terrain ) > 0 ) {
            play_variant_sound( "plmove", "walk_metal", heard_volume, 0, 0.8, 1.2 );
            start_sfx_timestamp = std::chrono::high_resolution_clock::now();
            return;
        } else if( water.count( terrain ) > 0 ) {
            play_variant_sound( "plmove", "walk_water", heard_volume, 0, 0.8, 1.2 );
            start_sfx_timestamp = std::chrono::high_resolution_clock::now();
            return;
        } else if( chain_fence.count( terrain ) > 0 ) {
            play_variant_sound( "plmove", "clear_obstacle", heard_volume, 0, 0.8, 1.2 );
            start_sfx_timestamp = std::chrono::high_resolution_clock::now();
            return;
        } else {
            play_variant_sound( "plmove", "walk_tarmac", heard_volume, 0, 0.8, 1.2 );
            start_sfx_timestamp = std::chrono::high_resolution_clock::now();
            return;
        }
    }
}

void sfx::do_obstacle_sfx() {
    int heard_volume = sfx::get_heard_volume( g->u.pos() );
    const auto terrain = g->m.ter_at( g->u.pos() ).id;
    static std::set<ter_type> const water = {
        ter_type( "t_water_sh" ),
        ter_type( "t_water_dp" ),
        ter_type( "t_swater_sh" ),
        ter_type( "t_swater_dp" ),
        ter_type( "t_water_pool" ),
        ter_type( "t_sewage" ),
    };
    if( water.count( terrain ) > 0 ) {
        return;
    } else {
        play_variant_sound( "plmove", "clear_obstacle", heard_volume, 0, 0.8, 1.2 );
    }
}
