#include "sounds.h"

#include "coordinate_conversions.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "enums.h"
#include "output.h"
#include "overmapbuffer.h"
#include "effect.h"
#include "translations.h"
#include "messages.h"
#include "monster.h"
#include "line.h"
#include "string_formatter.h"
#include "mtype.h"
#include "weather.h"
#include "npc.h"
#include "item.h"
#include "player.h"
#include "path_info.h"
#include "options.h"
#include "time.h"
#include "mapdata.h"
#include "itype.h"
#include "map_iterator.h"

#include <chrono>
#include <algorithm>
#include <cmath>

#ifdef SDL_SOUND
#   include <SDL_mixer.h>
#   include <thread>
#   if ((defined _WIN32 || defined WINDOWS) && !defined _MSC_VER)
#       include "mingw.thread.h"
#   endif
#endif

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

const efftype_id effect_alarm_clock( "alarm_clock" );
const efftype_id effect_deaf( "deaf" );
const efftype_id effect_narcosis( "narcosis" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_slept_through_alarm( "slept_through_alarm" );

static const trait_id trait_HEAVYSLEEPER2( "HEAVYSLEEPER2" );
static const trait_id trait_HEAVYSLEEPER( "HEAVYSLEEPER" );

struct sound_event {
    int volume;
    std::string description;
    bool ambient;
    bool footstep;
    std::string id;
    std::string variant;
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

void sounds::ambient_sound( const tripoint &p, int vol, const std::string &description )
{
    sound( p, vol, description, true );
}

void sounds::sound( const tripoint &p, int vol, std::string description, bool ambient,
                    const std::string &id, const std::string &variant )
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
    // to fight the combinatorial explosion.
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

int get_signal_for_hordes( const centroid &centr )
{
    //Volume in  tiles. Signal for hordes in submaps
    //modify vol using weather vol.Weather can reduce monster hearing
    const int vol = centr.volume - weather_data( g->weather ).sound_attn;
    const int min_vol_cap = 60; //Hordes can't hear volume lower than this
    const int underground_div = 2; //Coefficient for volume reduction underground
    const int hordes_sig_div = SEEX; //Divider coefficient for hordes
    const int min_sig_cap = 8; //Signal for hordes can't be lower that this if it pass min_vol_cap
    const int max_sig_cap = 26; //Signal for hordes can't be higher that this
    //Lower the level - lower the sound
    int vol_hordes = ( ( centr.z < 0 ) ? vol / ( underground_div * std::abs( centr.z ) ) : vol );
    if( vol_hordes > min_vol_cap ) {
        //Calculating horde hearing signal
        int sig_power = std::ceil( ( float ) vol_hordes / hordes_sig_div );
        //Capping minimum horde hearing signal
        sig_power = std::max( sig_power, min_sig_cap );
        //Capping extremely high signal to hordes
        sig_power = std::min( sig_power, max_sig_cap );
        add_msg( m_debug, "vol %d  vol_hordes %d sig_power %d ", vol, vol_hordes, sig_power );
        return sig_power;
    }
    return 0;
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
        int sig_power = get_signal_for_hordes( this_centroid );
        if( sig_power > 0 ) {

            const point abs_ms = g->m.getabs( source.x, source.y );
            const point abs_sm = ms_to_sm_copy( abs_ms );
            const tripoint target( abs_sm.x, abs_sm.y, source.z );
            overmap_buffer.signal_hordes( target, sig_power );
        }
        // Alert all monsters (that can hear) to the sound.
        for( monster &critter : g->all_monsters() ) {
            // @todo: Generalize this to Creature::hear_sound
            const int dist = rl_dist( source, critter.pos() );
            if( vol * 2 > dist ) {
                // Exclude monsters that certainly won't hear the sound
                critter.hear_sound( source, vol, dist );
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
        const tripoint &pos = sound_event_pair.first;
        const sound_event &sound = sound_event_pair.second;

        const int distance_to_sound = rl_dist( p->pos().x, p->pos().y, pos.x, pos.y ) +
                                      abs( p->pos().z - pos.z ) * 10;
        const int raw_volume = sound.volume;

        // The felt volume of a sound is not affected by negative multipliers, such as already
        // deafened players or players with sub-par hearing to begin with.
        const int felt_volume = ( int )( raw_volume * std::min( 1.0f,
                                         volume_multiplier ) ) - distance_to_sound;

        // Deafening is based on the felt volume, as a player may be too deaf to
        // hear the deafening sound but still suffer additional hearing loss.
        const bool is_sound_deafening = rng( felt_volume / 2, felt_volume ) >= 150;

        // Deaf players hear no sound, but still are at risk of additional hearing loss.
        if( is_deaf ) {
            if( is_sound_deafening && !p->is_immune_effect( effect_deaf ) ) {
                p->add_effect( effect_deaf, std::min( 4_minutes, time_duration::from_turns( felt_volume - 130 ) / 8 ) );
                if( !p->has_trait( trait_id( "NOPAIN" ) ) ) {
                    p->add_msg_if_player( m_bad, _( "Your eardrums suddenly ache!" ) );
                    if( p->get_pain() < 10 ) {
                        p->mod_pain( rng( 0, 2 ) );
                    }
                }
            }
            continue;
        }

        if( is_sound_deafening && !p->is_immune_effect( effect_deaf ) ) {
            const time_duration deafness_duration = time_duration::from_turns( felt_volume - 130 ) / 4;
            p->add_effect( effect_deaf, deafness_duration );
            if( p->is_deaf() ) {
                is_deaf = true;
                sfx::do_hearing_loss( to_turns<int>( deafness_duration ) );
                continue;
            }
        }

        // The heard volume of a sound is the player heard volume, regardless of true volume level.
        const int heard_volume = ( int )( ( raw_volume - weather_vol ) * volume_multiplier ) -
                                 distance_to_sound;

        if( heard_volume <= 0 && pos != p->pos() ) {
            continue;
        }

        // Player volume meter includes all sounds from their tile and adjacent tiles
        // TODO: Add noises from vehicle player is in.
        if( distance_to_sound <= 1 ) {
            p->volume = std::max( p->volume, heard_volume );
        }

        // Secure the flag before wake_up() clears the effect
        bool slept_through = p->has_effect( effect_slept_through_alarm );
        // See if we need to wake someone up
        if( p->has_effect( effect_sleep ) ) {
            if( ( ( !( p->has_trait( trait_HEAVYSLEEPER ) ||
                     p->has_trait( trait_HEAVYSLEEPER2 ) ) && dice( 2, 15 ) < heard_volume ) ||
                ( p->has_trait( trait_HEAVYSLEEPER ) && dice( 3, 15 ) < heard_volume ) ||
                ( p->has_trait( trait_HEAVYSLEEPER2 ) && dice( 6, 15 ) < heard_volume ) ) &&
                !p->has_effect( effect_narcosis ) ) {
                //Not kidding about sleep-through-firefight
                p->wake_up();
                add_msg( m_warning, _( "Something is making noise." ) );
            } else {
                continue;
            }
        }

        const std::string &description = sound.description;
        if( !sound.ambient && ( pos != p->pos() ) && !g->m.pl_sees( pos, distance_to_sound ) ) {
            if( !p->activity.ignore_trivial ) {
                const std::string query = description.empty()
                                          ? _( "Heard a noise!" )
                                          : string_format( _( "Heard %s!" ), description.c_str() );

                if( g->cancel_activity_or_ignore_query( query ) ) {
                    p->activity.ignore_trivial = true;
                    for( auto activity : p->backlog ) {
                        activity.ignore_trivial = true;
                    }
                }
            }
        }

        if( !description.empty() ) {
            // If it came from us, don't print a direction
            if( pos == p->pos() ) {
                add_msg( _( "You hear %s" ), description.c_str() );
            } else {
                // Else print a direction as well
                std::string direction = direction_name( direction_from( p->pos(), pos ) );
                add_msg( m_warning, _( "From the %1$s you hear %2$s" ), direction.c_str(), description.c_str() );
            }
        }

        if( !p->has_effect( effect_sleep ) && p->has_effect( effect_alarm_clock ) &&
            !p->has_bionic( bionic_id( "bio_watch" ) ) ) {
            if( p->get_effect( effect_alarm_clock ).get_duration() < 2_turns ) {
                if( slept_through ) {
                    p->add_msg_if_player( _( "Your alarm-clock finally wakes you up." ) );
                } else {
                    p->add_msg_if_player( _( "Your alarm-clock wakes you up." ) );
                }
                p->add_msg_if_player( _( "You turn off your alarm-clock." ) );
            }
            p->get_effect( effect_alarm_clock ).set_duration( 0_turns );
        }

        const std::string &sfx_id = sound.id;
        const std::string &sfx_variant = sound.variant;
        if( !sfx_id.empty() ) {
            sfx::play_variant_sound( sfx_id, sfx_variant, sfx::get_heard_volume( pos ) );
        }

        // Place footstep markers.
        if( pos == p->pos() || p->sees( pos ) ) {
            // If we are or can see the source, don't draw a marker.
            continue;
        }

        int err_offset;
        if( ( heard_volume + distance_to_sound ) / distance_to_sound < 2 ) {
            err_offset = 3;
        } else if( ( heard_volume + distance_to_sound ) / distance_to_sound < 3 ) {
            err_offset = 2;
        } else {
            err_offset = 1;
        }

        // If Z-coordinate is different, draw even when you can see the source
        const bool diff_z = pos.z != p->posz();

        // Enumerate the valid points the player *cannot* see.
        // Unless the source is on a different z-level, then any point is fine
        std::vector<tripoint> unseen_points;
        for( const tripoint &newp : g->m.points_in_radius( pos, err_offset ) ) {
            if( diff_z || !p->sees( newp ) ) {
                unseen_points.emplace_back( newp );
            }
        }

        // Then place the sound marker in a random one.
        if( !unseen_points.empty() ) {
            sound_markers.emplace( random_entry( unseen_points ), sound );
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
        cluster_centroids.emplace_back( ( int )sound.x, ( int )sound.y, ( int )sound.z );
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

#ifdef SDL_SOUND

bool is_underground( const tripoint &p )
{
    return p.z < 0;
}

void sfx::fade_audio_group( int tag, int duration )
{
    Mix_FadeOutGroup( tag, duration );
}

void sfx::fade_audio_channel( int tag, int duration )
{
    Mix_FadeOutChannel( tag, duration );
}

bool sfx::is_channel_playing( int channel )
{
    return Mix_Playing( channel ) != 0;
}

void sfx::stop_sound_effect_fade( int channel, int duration )
{
    if( Mix_FadeOutChannel( channel, duration ) == -1 ) {
        dbg( D_ERROR ) << "Failed to stop sound effect: " << Mix_GetError();
    }
}

void sfx::do_ambient()
{
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
    const bool is_deaf = g->u.get_effect_int( effect_deaf ) > 0;
    const int heard_volume = get_heard_volume( g->u.pos() );
    const bool is_underground = ::is_underground( g->u.pos() );
    const bool is_sheltered = g->is_sheltered( g->u.pos() );
    const bool weather_changed = g->weather != previous_weather;
    // Step in at night time / we are not indoors
    if( calendar::turn.is_night() && !is_sheltered &&
        !is_channel_playing( 1 ) && !is_deaf ) {
        fade_audio_group( 2, 1000 );
        play_ambient_variant_sound( "environment", "nighttime", heard_volume, 1, 1000 );
        // Step in at day time / we are not indoors
    } else if( !calendar::turn.is_night() && !is_channel_playing( 0 ) &&
               !is_sheltered && !is_deaf ) {
        fade_audio_group( 2, 1000 );
        play_ambient_variant_sound( "environment", "daytime", heard_volume, 0, 1000 );
    }
    // We are underground
    if( ( is_underground && !is_channel_playing( 2 ) &&
          !is_deaf ) || ( is_underground &&
                          weather_changed && !is_deaf ) ) {
        fade_audio_group( 1, 1000 );
        fade_audio_group( 2, 1000 );
        play_ambient_variant_sound( "environment", "underground", heard_volume, 2,
                                    1000 );
        // We are indoors
    } else if( ( is_sheltered && !is_underground &&
                 !is_channel_playing( 3 ) && !is_deaf ) ||
               ( is_sheltered && !is_underground &&
                 weather_changed && !is_deaf ) ) {
        fade_audio_group( 1, 1000 );
        fade_audio_group( 2, 1000 );
        play_ambient_variant_sound( "environment", "indoors", heard_volume, 3, 1000 );
    }
    // We are indoors and it is also raining
    if( g->weather >= WEATHER_DRIZZLE && g->weather <= WEATHER_ACID_RAIN && !is_underground
        && !is_channel_playing( 4 ) ) {
        play_ambient_variant_sound( "environment", "indoors_rain", heard_volume, 4,
                                    1000 );
    }
    if( ( !is_sheltered && g->weather != WEATHER_CLEAR
          && !is_channel_playing( 5 ) &&
          !is_channel_playing( 6 ) && !is_channel_playing( 7 ) && !is_channel_playing( 8 )
          &&
          !is_channel_playing( 9 ) && !is_deaf )
        || ( !is_sheltered &&
             weather_changed  && !is_deaf ) ) {
        fade_audio_group( 1, 1000 );
        // We are outside and there is precipitation
        switch( g->weather ) {
            case WEATHER_ACID_DRIZZLE:
            case WEATHER_DRIZZLE:
                play_ambient_variant_sound( "environment", "WEATHER_DRIZZLE", heard_volume, 9,
                                            1000 );
                break;
            case WEATHER_RAINY:
                play_ambient_variant_sound( "environment", "WEATHER_RAINY", heard_volume, 8,
                                            1000 );
                break;
            case WEATHER_ACID_RAIN:
            case WEATHER_THUNDER:
            case WEATHER_LIGHTNING:
                play_ambient_variant_sound( "environment", "WEATHER_THUNDER", heard_volume, 7,
                                            1000 );
                break;
            case WEATHER_FLURRIES:
                play_ambient_variant_sound( "environment", "WEATHER_FLURRIES", heard_volume, 6,
                                            1000 );
                break;
            case WEATHER_CLEAR:
            case WEATHER_SUNNY:
            case WEATHER_CLOUDY:
            case WEATHER_SNOWSTORM:
            case WEATHER_SNOW:
                play_ambient_variant_sound( "environment", "WEATHER_SNOW", heard_volume, 5,
                                            1000 );
                break;
            case WEATHER_NULL:
            case NUM_WEATHER_TYPES:
                // nothing here, those are pseudo-types, they should not be active at all.
                break;
        }
    }
    // Keep track of weather to compare for next iteration
    previous_weather = g->weather;
}

// firing is the item that is fired. It may be the wielded gun, but it can also be an attached
// gunmod. p is the character that is firing, this may be a pseudo-character (used by monattack/
// vehicle turrets) or a NPC.
void sfx::generate_gun_sound( const player &p, const item &firing )
{
    end_sfx_timestamp = std::chrono::high_resolution_clock::now();
    sfx_time = end_sfx_timestamp - start_sfx_timestamp;
    if( std::chrono::duration_cast<std::chrono::milliseconds> ( sfx_time ).count() < 80 ) {
        return;
    }
    const tripoint source = p.pos();
    int heard_volume = get_heard_volume( source );
    if( heard_volume <= 30 ) {
        heard_volume = 30;
    }

    itype_id weapon_id = firing.typeId();
    int angle;
    int distance;
    std::string selected_sound;
    // this does not mean p == g->u (it could be a vehicle turret)
    if( g->u.pos() == source ) {
        angle = 0;
        distance = 0;
        selected_sound = "fire_gun";

        const auto mods = firing.gunmods();
        if( std::any_of( mods.begin(), mods.end(),
                         []( const item * e ) {
                             return e->type->gunmod->loudness < 0;
                         } ) ) {
            weapon_id = "weapon_fire_suppressed";
        }

    } else {
        angle = get_heard_angle( source );
        distance = rl_dist( g->u.pos(), source );
        if( distance <= 17 ) {
            selected_sound = "fire_gun";
        } else {
            selected_sound = "fire_gun_distant";
        }
    }

    play_variant_sound( selected_sound, weapon_id, heard_volume, angle, 0.8, 1.2 );
    start_sfx_timestamp = std::chrono::high_resolution_clock::now();
}

namespace sfx
{
struct sound_thread {
    sound_thread( const tripoint &source, const tripoint &target, bool hit, bool targ_mon,
                  const std::string &material );

    bool hit;
    bool targ_mon;
    std::string material;

    skill_id weapon_skill;
    int weapon_volume;
    // volume and angle for calls to play_variant_sound
    int ang_src;
    int vol_src;
    int vol_targ;
    int ang_targ;

    // Operator overload required for thread API.
    void operator()() const;
};
} // namespace sfx

void sfx::generate_melee_sound( const tripoint &source, const tripoint &target, bool hit, bool targ_mon,
                                const std::string &material )
{
    // If creating a new thread for each invocation is to much, we have to consider a thread
    // pool or maybe a single thread that works continuously, but that requires a queue or similar
    // to coordinate its work.
    std::thread the_thread( sound_thread( source, target, hit, targ_mon, material ) );
    the_thread.detach();
}

sfx::sound_thread::sound_thread( const tripoint &source, const tripoint &target, const bool hit,
                                 const bool targ_mon, const std::string &material )
    : hit( hit )
    , targ_mon( targ_mon )
    , material( material )
{
    // This is function is run in the main thread.
    const int heard_volume = get_heard_volume( source );
    const player *p = g->critter_at<npc>( source );
    if( !p ) {
        p = &g->u;
        // sound comes from the same place as the player is, calculation of angle wouldn't work
        ang_src = 0;
        vol_src = heard_volume;
        vol_targ = heard_volume;
    } else {
        ang_src = get_heard_angle( source );
        vol_src = std::max( heard_volume - 30, 0 );
        vol_targ = std::max( heard_volume - 20, 0 );
    }
    ang_targ = get_heard_angle( target );
    weapon_skill = p->weapon.melee_skill();
    weapon_volume = p->weapon.volume() / units::legacy_volume_factor;
}

// Operator overload required for thread API.
void sfx::sound_thread::operator()() const
{
    // This is function is run in a separate thread. One must be careful and not access game data
    // that might change (e.g. g->u.weapon, the character could switch weapons while this thread
    // runs).
    std::this_thread::sleep_for( std::chrono::milliseconds( rng( 1, 2 ) ) );
    std::string variant_used;

    static const skill_id skill_bashing( "bashing" );
    static const skill_id skill_cutting( "cutting" );
    static const skill_id skill_stabbing( "stabbing" );

    if( weapon_skill == skill_bashing && weapon_volume <= 8 ) {
        variant_used = "small_bash";
        play_variant_sound( "melee_swing", "small_bash", vol_src, ang_src, 0.8, 1.2 );
    } else if( weapon_skill == skill_bashing && weapon_volume >= 9 ) {
        variant_used = "big_bash";
        play_variant_sound( "melee_swing", "big_bash", vol_src, ang_src, 0.8, 1.2 );
    } else if( ( weapon_skill == skill_cutting || weapon_skill == skill_stabbing ) &&
               weapon_volume <= 6 ) {
        variant_used = "small_cutting";
        play_variant_sound( "melee_swing", "small_cutting", vol_src, ang_src, 0.8, 1.2 );
    } else if( ( weapon_skill == skill_cutting || weapon_skill == skill_stabbing ) &&
               weapon_volume >= 7 ) {
        variant_used = "big_cutting";
        play_variant_sound( "melee_swing", "big_cutting", vol_src, ang_src, 0.8, 1.2 );
    } else {
        variant_used = "default";
        play_variant_sound( "melee_swing", "default", vol_src, ang_src, 0.8, 1.2 );
    }
    if( hit ) {
        if( targ_mon ) {
            if( material == "steel" ) {
                std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 12,
                                             weapon_volume * 16 ) ) );
                play_variant_sound( "melee_hit_metal", variant_used, vol_targ, ang_targ, 0.8, 1.2 );
            } else {
                std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 12,
                                             weapon_volume * 16 ) ) );
                play_variant_sound( "melee_hit_flesh", variant_used, vol_targ, ang_targ, 0.8, 1.2 );
            }
        } else {
            std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 9,
                                         weapon_volume * 12 ) ) );
            play_variant_sound( "melee_hit_flesh", variant_used, vol_targ, ang_targ, 0.8, 1.2 );
        }
    }
}

void sfx::do_projectile_hit( const Creature &target )
{
    const int heard_volume = sfx::get_heard_volume( target.pos() );
    const int angle = get_heard_angle( target.pos() );
    if( target.is_monster() ) {
        const monster &mon = dynamic_cast<const monster &>( target );
        static std::set<material_id> const fleshy = {
            material_id( "flesh" ),
            material_id( "hflesh" ),
            material_id( "iflesh" ),
            material_id( "veggy" ),
            material_id( "bone" ),
        };
        const bool is_fleshy = std::any_of( fleshy.begin(), fleshy.end(), [&mon]( const material_id & m ) {
            return mon.made_of( m );
        } );

        if( is_fleshy ) {
            play_variant_sound( "bullet_hit", "hit_flesh", heard_volume, angle, 0.8, 1.2 );
            return;
        } else if( mon.made_of( material_id( "stone" ) ) ) {
            play_variant_sound( "bullet_hit", "hit_wall", heard_volume, angle, 0.8, 1.2 );
            return;
        } else if( mon.made_of( material_id( "steel" ) ) ) {
            play_variant_sound( "bullet_hit", "hit_metal", heard_volume, angle, 0.8, 1.2 );
            return;
        } else {
            play_variant_sound( "bullet_hit", "hit_flesh", heard_volume, angle, 0.8, 1.2 );
            return;
        }
    }
    play_variant_sound( "bullet_hit", "hit_flesh", heard_volume, angle, 0.8, 1.2 );
}

void sfx::do_player_death_hurt( const player &target, bool death )
{
    int heard_volume = get_heard_volume( target.pos() );
    const bool male = target.male;
    if( !male && !death ) {
        play_variant_sound( "deal_damage", "hurt_f", heard_volume );
    } else if( male && !death ) {
        play_variant_sound( "deal_damage", "hurt_m", heard_volume );
    } else if( !male && death ) {
        play_variant_sound( "clean_up_at_end", "death_f", heard_volume );
    } else if( male && death ) {
        play_variant_sound( "clean_up_at_end", "death_m", heard_volume );
    }
}

void sfx::do_danger_music()
{
    if( g->u.in_sleep_state() && !audio_muted ) {
        fade_audio_channel( -1, 100 );
        audio_muted = true;
        return;
    } else if( ( g->u.in_sleep_state() && audio_muted ) || is_channel_playing( 19 ) ) {
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

void sfx::do_fatigue()
{
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

void sfx::do_hearing_loss( int turns )
{
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

void sfx::remove_hearing_loss()
{
    if( current_deafness_turns >= deafness_turns ) {
        stop_sound_effect_fade( 10, 300 );
        g_sfx_volume_multiplier = 1;
        deafness_turns = 0;
        current_deafness_turns = 0;
        do_ambient();
    }
    current_deafness_turns++;
}

void sfx::do_footstep()
{
    end_sfx_timestamp = std::chrono::high_resolution_clock::now();
    sfx_time = end_sfx_timestamp - start_sfx_timestamp;
    if( std::chrono::duration_cast<std::chrono::milliseconds> ( sfx_time ).count() > 400 ) {
        int heard_volume = sfx::get_heard_volume( g->u.pos() );
        const auto terrain = g->m.ter( g->u.pos() ).id();
        static std::set<ter_str_id> const grass = {
            ter_str_id( "t_grass" ),
            ter_str_id( "t_shrub" ),
            ter_str_id( "t_underbrush" ),
        };
        static std::set<ter_str_id> const dirt = {
            ter_str_id( "t_dirt" ),
            ter_str_id( "t_sand" ),
            ter_str_id( "t_clay" ),
            ter_str_id( "t_dirtfloor" ),
            ter_str_id( "t_palisade_gate_o" ),
            ter_str_id( "t_sandbox" ),
        };
        static std::set<ter_str_id> const metal = {
            ter_str_id( "t_ov_smreb_cage" ),
            ter_str_id( "t_metal_floor" ),
            ter_str_id( "t_grate" ),
            ter_str_id( "t_bridge" ),
            ter_str_id( "t_elevator" ),
            ter_str_id( "t_guardrail_bg_dp" ),
        };
        static std::set<ter_str_id> const water = {
            ter_str_id( "t_water_sh" ),
            ter_str_id( "t_water_dp" ),
            ter_str_id( "t_swater_sh" ),
            ter_str_id( "t_swater_dp" ),
            ter_str_id( "t_water_pool" ),
            ter_str_id( "t_sewage" ),
        };
        static std::set<ter_str_id> const chain_fence = {
            ter_str_id( "t_chainfence" ),
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

void sfx::do_obstacle()
{
    int heard_volume = sfx::get_heard_volume( g->u.pos() );
    const auto terrain = g->m.ter( g->u.pos() ).id();
    static std::set<ter_str_id> const water = {
        ter_str_id( "t_water_sh" ),
        ter_str_id( "t_water_dp" ),
        ter_str_id( "t_swater_sh" ),
        ter_str_id( "t_swater_dp" ),
        ter_str_id( "t_water_pool" ),
        ter_str_id( "t_sewage" ),
    };
    if( water.count( terrain ) > 0 ) {
        return;
    } else {
        play_variant_sound( "plmove", "clear_obstacle", heard_volume, 0, 0.8, 1.2 );
    }
}

#else // ifdef SDL_SOUND

/** Dummy implementations for builds without sound */
/*@{*/
void sfx::load_sound_effects( JsonObject & ) { }
void sfx::load_playlist( JsonObject & ) { }
void sfx::play_variant_sound( const std::string &, const std::string &, int, int, float, float ) { }
void sfx::play_variant_sound( const std::string &, const std::string &, int ) { }
void sfx::play_ambient_variant_sound( const std::string &, const std::string &, int, int, int ) { }
void sfx::generate_gun_sound( const player &, const item & ) { }
void sfx::generate_melee_sound( const tripoint &, const tripoint &, bool, bool, const std::string & ) { }
void sfx::do_hearing_loss( int ) { }
void sfx::remove_hearing_loss() { }
void sfx::do_projectile_hit( const Creature & ) { }
void sfx::do_footstep() { }
void sfx::do_danger_music() { }
void sfx::do_ambient() { }
void sfx::fade_audio_group( int, int ) { }
void sfx::fade_audio_channel( int, int ) { }
bool is_channel_playing( int )
{
    return false;
}
void sfx::stop_sound_effect_fade( int, int ) { }
void sfx::do_player_death_hurt( const player &, bool ) { }
void sfx::do_fatigue() { }
void sfx::do_obstacle() { }
/*@}*/

#endif // ifdef SDL_SOUND

/** Functions from sfx that do not use the SDL_mixer API at all. They can be used in builds
  * without sound support. */
/*@{*/
int sfx::get_heard_volume( const tripoint &source )
{
    int distance = rl_dist( g->u.pos(), source );
    // fract = -100 / 24
    const float fract = -4.166666;
    int heard_volume = fract * distance - 1 + 100;
    if( heard_volume <= 0 ) {
        heard_volume = 0;
    }
    heard_volume *= g_sfx_volume_multiplier;
    return ( heard_volume );
}

int sfx::get_heard_angle( const tripoint &source )
{
    int angle = g->m.coord_to_angle( g->u.posx(), g->u.posy(), source.x, source.y ) + 90;
    //add_msg(m_warning, "angle: %i", angle);
    return ( angle );
}
/*@}*/
