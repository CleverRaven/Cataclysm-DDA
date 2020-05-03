#include "sounds.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <ostream>
#include <set>
#include <system_error>
#include <type_traits>
#include <unordered_map>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "coordinate_conversions.h"
#include "creature.h"
#include "debug.h"
#include "effect.h"
#include "enums.h"
#include "game.h"
#include "game_constants.h"
#include "item.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "npc.h"
#include "optional.h"
#include "overmapbuffer.h"
#include "player.h"
#include "player_activity.h"
#include "point.h"
#include "rng.h"
#include "safemode_ui.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"

#if defined(SDL_SOUND)
#   if defined(_MSC_VER) && defined(USE_VCPKG)
#      include <SDL2/SDL_mixer.h>
#   else
#      include <SDL_mixer.h>
#   endif
#   include <thread>
#   if defined(_WIN32) && !defined(_MSC_VER)
#       include "mingw.thread.h"
#   endif

#   define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "
#endif

weather_type previous_weather;
int prev_hostiles = 0;
int previous_speed = 0;
int previous_gear = 0;
bool audio_muted = false;
float g_sfx_volume_multiplier = 1;
auto start_sfx_timestamp = std::chrono::high_resolution_clock::now();
auto end_sfx_timestamp = std::chrono::high_resolution_clock::now();
auto sfx_time = end_sfx_timestamp - start_sfx_timestamp;
activity_id act;
std::pair<std::string, std::string> engine_external_id_and_variant;

static const efftype_id effect_alarm_clock( "alarm_clock" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_slept_through_alarm( "slept_through_alarm" );

static const trait_id trait_HEAVYSLEEPER2( "HEAVYSLEEPER2" );
static const trait_id trait_HEAVYSLEEPER( "HEAVYSLEEPER" );
static const itype_id fuel_type_muscle( "muscle" );
static const itype_id fuel_type_wind( "wind" );
static const itype_id fuel_type_battery( "battery" );

struct sound_event {
    int volume;
    sounds::sound_t category;
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

namespace io
{
// *INDENT-OFF*
template<>
std::string enum_to_string<sounds::sound_t>( sounds::sound_t data )
{
    switch ( data ) {
    case sounds::sound_t::background: return "background";
    case sounds::sound_t::weather: return "weather";
    case sounds::sound_t::music: return "music";
    case sounds::sound_t::movement: return "movement";
    case sounds::sound_t::speech: return "speech";
    case sounds::sound_t::electronic_speech: return "electronic_speech";
    case sounds::sound_t::activity: return "activity";
    case sounds::sound_t::destructive_activity: return "destructive_activity";
    case sounds::sound_t::alarm: return "alarm";
    case sounds::sound_t::combat: return "combat";
    case sounds::sound_t::alert: return "alert";
    case sounds::sound_t::order: return "order";
    case sounds::sound_t::_LAST: break;
    }
    debugmsg( "Invalid valid_target" );
    abort();
}
// *INDENT-ON*
} // namespace io

// Static globals tracking sounds events of various kinds.
// The sound events since the last monster turn.
static std::vector<std::pair<tripoint, int>> recent_sounds;
// The sound events since the last interactive player turn. (doesn't count sleep etc)
static std::vector<std::pair<tripoint, sound_event>> sounds_since_last_turn;
// The sound events currently displayed to the player.
static std::unordered_map<tripoint, sound_event> sound_markers;

// This is an attempt to handle attenuation of sound for underground areas.
// The main issue it adresses is that you can hear activity
// relatively deep underground while on the surface.
// My research indicates that attenuation through soil-like materials is as
// high as 100x the attenuation through air, plus vertical distances are
// roughly five times as large as horizontal ones.
static int sound_distance( const tripoint &source, const tripoint &sink )
{
    const int lower_z = std::min( source.z, sink.z );
    const int upper_z = std::max( source.z, sink.z );
    const int vertical_displacement = upper_z - lower_z;
    int vertical_attenuation = vertical_displacement;
    if( lower_z < 0 && vertical_displacement > 0 ) {
        // Apply a moderate bonus attenuation (5x) for the first level of vertical displacement.
        vertical_attenuation += 4;
        // At displacements greater than one, apply a large additional attenuation (100x) per level.
        const int underground_displacement = std::min( -lower_z, vertical_displacement );
        vertical_attenuation += ( underground_displacement - 1 ) * 20;
    }
    // Regardless of underground effects, scale the vertical distance by 5x.
    vertical_attenuation *= 5;
    return rl_dist( source.xy(), sink.xy() ) + vertical_attenuation;
}

void sounds::ambient_sound( const tripoint &p, int vol, sound_t category,
                            const std::string &description )
{
    sound( p, vol, category, description, true );
}

void sounds::sound( const tripoint &p, int vol, sound_t category, const std::string &description,
                    bool ambient, const std::string &id, const std::string &variant )
{
    if( vol < 0 ) {
        // Bail out if no volume.
        debugmsg( "negative sound volume %d", vol );
        return;
    }
    // Description is not an optional parameter
    if( description.empty() ) {
        debugmsg( "Sound at %d:%d has no description!", p.x, p.y );
    }
    recent_sounds.emplace_back( std::make_pair( p, vol ) );
    sounds_since_last_turn.emplace_back( std::make_pair( p,
                                         sound_event {vol, category, description, ambient,
                                                 false, id, variant} ) );
}

void sounds::sound( const tripoint &p, int vol, sound_t category, const translation &description,
                    bool ambient, const std::string &id, const std::string &variant )
{
    sounds::sound( p, vol, category, description.translated(), ambient, id, variant );
}

void sounds::add_footstep( const tripoint &p, int volume, int, monster *,
                           const std::string &footstep )
{
    sounds_since_last_turn.emplace_back( std::make_pair( p, sound_event { volume,
                                         sound_t::movement, footstep, false, true, "", ""} ) );
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

static std::vector<centroid> cluster_sounds( std::vector<std::pair<tripoint, int>> input_sounds )
{
    // If there are too many monsters and too many noise sources (which can be monsters, go figure),
    // applying sound events to monsters can dominate processing time for the whole game,
    // so we cluster sounds and apply the centroids of the sounds to the monster AI
    // to fight the combinatorial explosion.
    std::vector<centroid> sound_clusters;
    if( input_sounds.empty() ) {
        return sound_clusters;
    }
    const int num_seed_clusters =
        std::max( std::min( input_sounds.size(), static_cast<size_t>( 10 ) ),
                  static_cast<size_t>( std::log( input_sounds.size() ) ) );
    const size_t stopping_point = input_sounds.size() - num_seed_clusters;
    const size_t max_map_distance = sound_distance( tripoint( point_zero, OVERMAP_DEPTH ),
                                    tripoint( MAPSIZE_X, MAPSIZE_Y, OVERMAP_HEIGHT ) );
    // Randomly choose cluster seeds.
    for( size_t i = input_sounds.size(); i > stopping_point; i-- ) {
        size_t index = rng( 0, i - 1 );
        // The volume and cluster weight are the same for the first element.
        sound_clusters.push_back(
            // Assure the compiler that these int->float conversions are safe.
        {
            static_cast<float>( input_sounds[index].first.x ), static_cast<float>( input_sounds[index].first.y ),
            static_cast<float>( input_sounds[index].first.z ),
            static_cast<float>( input_sounds[index].second ), static_cast<float>( input_sounds[index].second )
        } );
        vector_quick_remove( input_sounds, index );
    }
    for( const auto &sound_event_pair : input_sounds ) {
        auto found_centroid = sound_clusters.begin();
        float dist_factor = max_map_distance;
        const auto cluster_end = sound_clusters.end();
        for( auto centroid_iter = sound_clusters.begin(); centroid_iter != cluster_end;
             ++centroid_iter ) {
            // Scale the distance between the two by the max possible distance.
            tripoint centroid_pos { static_cast<int>( centroid_iter->x ), static_cast<int>( centroid_iter->y ), static_cast<int>( centroid_iter->z ) };
            const int dist = sound_distance( sound_event_pair.first, centroid_pos );
            if( dist * dist < dist_factor ) {
                found_centroid = centroid_iter;
                dist_factor = dist * dist;
            }
        }
        const float volume_sum = static_cast<float>( sound_event_pair.second ) + found_centroid->weight;
        // Set the centroid location to the average of the two locations, weighted by volume.
        found_centroid->x = static_cast<float>( ( sound_event_pair.first.x * sound_event_pair.second ) +
                                                ( found_centroid->x * found_centroid->weight ) ) / volume_sum;
        found_centroid->y = static_cast<float>( ( sound_event_pair.first.y * sound_event_pair.second ) +
                                                ( found_centroid->y * found_centroid->weight ) ) / volume_sum;
        found_centroid->z = static_cast<float>( ( sound_event_pair.first.z * sound_event_pair.second ) +
                                                ( found_centroid->z * found_centroid->weight ) ) / volume_sum;
        // Set the centroid volume to the larger of the volumes.
        found_centroid->volume = std::max( found_centroid->volume,
                                           static_cast<float>( sound_event_pair.second ) );
        // Set the centroid weight to the sum of the weights.
        found_centroid->weight = volume_sum;
    }
    return sound_clusters;
}

static int get_signal_for_hordes( const centroid &centr )
{
    //Volume in  tiles. Signal for hordes in submaps
    //modify vol using weather vol.Weather can reduce monster hearing
    const int vol = centr.volume - weather::sound_attn( g->weather.weather );
    const int min_vol_cap = 60; //Hordes can't hear volume lower than this
    const int underground_div = 2; //Coefficient for volume reduction underground
    const int hordes_sig_div = SEEX; //Divider coefficient for hordes
    const int min_sig_cap = 8; //Signal for hordes can't be lower that this if it pass min_vol_cap
    const int max_sig_cap = 26; //Signal for hordes can't be higher that this
    //Lower the level - lower the sound
    int vol_hordes = ( ( centr.z < 0 ) ? vol / ( underground_div * std::abs( centr.z ) ) : vol );
    if( vol_hordes > min_vol_cap ) {
        //Calculating horde hearing signal
        int sig_power = std::ceil( static_cast<float>( vol_hordes ) / hordes_sig_div );
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
    const int weather_vol = weather::sound_attn( g->weather.weather );
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

            const point abs_ms = g->m.getabs( source.xy() );
            const point abs_sm = ms_to_sm_copy( abs_ms );
            const tripoint target( abs_sm, source.z );
            overmap_buffer.signal_hordes( target, sig_power );
        }
        // Alert all monsters (that can hear) to the sound.
        for( monster &critter : g->all_monsters() ) {
            // TODO: Generalize this to Creature::hear_sound
            const int dist = sound_distance( source, critter.pos() );
            if( vol * 2 > dist ) {
                // Exclude monsters that certainly won't hear the sound
                critter.hear_sound( source, vol, dist );
            }
        }
    }
    recent_sounds.clear();
}

// skip some sounds to avoid message spam
static bool describe_sound( sounds::sound_t category, bool from_player_position )
{
    if( from_player_position ) {
        switch( category ) {
            case sounds::sound_t::_LAST:
                debugmsg( "ERROR: Incorrect sound category" );
                return false;
            case sounds::sound_t::background:
            case sounds::sound_t::weather:
            case sounds::sound_t::music:
            // detailed music descriptions are printed in iuse::play_music
            case sounds::sound_t::movement:
            case sounds::sound_t::activity:
            case sounds::sound_t::destructive_activity:
            case sounds::sound_t::combat:
            case sounds::sound_t::alert:
            case sounds::sound_t::order:
            case sounds::sound_t::speech:
                return false;
            case sounds::sound_t::electronic_speech:
            case sounds::sound_t::alarm:
                return true;
        }
    } else {
        switch( category ) {
            case sounds::sound_t::background:
            case sounds::sound_t::weather:
            case sounds::sound_t::music:
            case sounds::sound_t::movement:
            case sounds::sound_t::activity:
            case sounds::sound_t::destructive_activity:
                return one_in( 100 );
            case sounds::sound_t::speech:
            case sounds::sound_t::electronic_speech:
            case sounds::sound_t::alarm:
            case sounds::sound_t::combat:
            case sounds::sound_t::alert:
            case sounds::sound_t::order:
                return true;
            case sounds::sound_t::_LAST:
                debugmsg( "ERROR: Incorrect sound category" );
                return false;
        }
    }
    return true;
}

void sounds::process_sound_markers( player *p )
{
    bool is_deaf = p->is_deaf();
    const float volume_multiplier = p->hearing_ability();
    const int weather_vol = weather::sound_attn( g->weather.weather );
    for( const auto &sound_event_pair : sounds_since_last_turn ) {
        const tripoint &pos = sound_event_pair.first;
        const sound_event &sound = sound_event_pair.second;
        const int distance_to_sound = sound_distance( p->pos(), pos );
        const int raw_volume = sound.volume;

        // The felt volume of a sound is not affected by negative multipliers, such as already
        // deafened players or players with sub-par hearing to begin with.
        const int felt_volume = static_cast<int>( raw_volume * std::min( 1.0f,
                                volume_multiplier ) ) - distance_to_sound;

        // Deafening is based on the felt volume, as a player may be too deaf to
        // hear the deafening sound but still suffer additional hearing loss.
        const bool is_sound_deafening = rng( felt_volume / 2, felt_volume ) >= 150;

        // Deaf players hear no sound, but still are at risk of additional hearing loss.
        if( is_deaf ) {
            if( is_sound_deafening && !p->is_immune_effect( effect_deaf ) ) {
                p->add_effect( effect_deaf, std::min( 4_minutes,
                                                      time_duration::from_turns( felt_volume - 130 ) / 8 ) );
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
            if( p->is_deaf() && !is_deaf ) {
                is_deaf = true;
                continue;
            }
        }

        // The heard volume of a sound is the player heard volume, regardless of true volume level.
        const int heard_volume = static_cast<int>( ( raw_volume - weather_vol ) *
                                 volume_multiplier ) - distance_to_sound;

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
        const std::string &description = sound.description.empty() ? _( "a noise" ) : sound.description;
        if( p->is_npc() ) {
            if( !sound.ambient ) {
                npc *guy = dynamic_cast<npc *>( p );
                guy->handle_sound( sound.category, description, heard_volume, pos );
            }
            continue;
        }

        // don't print our own noise or things without descriptions
        if( !sound.ambient && ( pos != p->pos() ) && !g->m.pl_sees( pos, distance_to_sound ) ) {
            if( !p->activity.is_distraction_ignored( distraction_type::noise ) &&
                !get_safemode().is_sound_safe( sound.description, distance_to_sound ) ) {
                const std::string query = string_format( _( "Heard %s!" ), description );
                g->cancel_activity_or_ignore_query( distraction_type::noise, query );
            }
        }

        // skip some sounds to avoid message spam
        if( describe_sound( sound.category, pos == p->pos() ) ) {
            game_message_type severity = m_info;
            if( sound.category == sound_t::combat || sound.category == sound_t::alarm ) {
                severity = m_warning;
            }
            // if we can see it, don't print a direction
            if( pos == p->pos() ) {
                add_msg( severity, _( "From your position you hear %1$s" ), description );
            } else if( p->sees( pos ) ) {
                add_msg( severity, _( "You hear %1$s" ), description );
            } else {
                std::string direction = direction_name( direction_from( p->pos(), pos ) );
                add_msg( severity, _( "From the %1$s you hear %2$s" ), direction, description );
            }
        }

        if( !p->has_effect( effect_sleep ) && p->has_effect( effect_alarm_clock ) &&
            !p->has_bionic( bionic_id( "bio_watch" ) ) ) {
            // if we don't have effect_sleep but we're in_sleep_state, either
            // we were trying to fall asleep for so long our alarm is now going
            // off or something disturbed us while trying to sleep
            const bool trying_to_sleep = p->in_sleep_state();
            if( p->get_effect( effect_alarm_clock ).get_duration() == 1_turns ) {
                if( slept_through ) {
                    add_msg( _( "Your alarm clock finally wakes you up." ) );
                } else if( !trying_to_sleep ) {
                    add_msg( _( "Your alarm clock wakes you up." ) );
                } else {
                    add_msg( _( "Your alarm clock goes off and you haven't slept a wink." ) );
                    p->activity.set_to_null();
                }
                add_msg( _( "You turn off your alarm-clock." ) );
                p->get_effect( effect_alarm_clock ).set_duration( 0_turns );
            }
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
    if( p->is_player() ) {
        sounds_since_last_turn.clear();
    }
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
        cluster_centroids.emplace_back( static_cast<int>( sound.x ), static_cast<int>( sound.y ),
                                        static_cast<int>( sound.z ) );
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

#if defined(SDL_SOUND)
void sfx::fade_audio_group( group group, int duration )
{
    Mix_FadeOutGroup( static_cast<int>( group ), duration );
}

void sfx::fade_audio_channel( channel channel, int duration )
{
    Mix_FadeOutChannel( static_cast<int>( channel ), duration );
}

bool sfx::is_channel_playing( channel channel )
{
    return Mix_Playing( static_cast<int>( channel ) ) != 0;
}

void sfx::stop_sound_effect_fade( channel channel, int duration )
{
    if( Mix_FadeOutChannel( static_cast<int>( channel ), duration ) == -1 ) {
        dbg( D_ERROR ) << "Failed to stop sound effect: " << Mix_GetError();
    }
}

void sfx::stop_sound_effect_timed( channel channel, int time )
{
    Mix_ExpireChannel( static_cast<int>( channel ), time );
}

int sfx::set_channel_volume( channel channel, int volume )
{
    int ch = static_cast<int>( channel );
    if( !Mix_Playing( ch ) ) {
        return -1;
    }
    if( Mix_FadingChannel( ch ) != MIX_NO_FADING ) {
        return -1;
    }
    return Mix_Volume( ch, volume );
}

void sfx::do_vehicle_engine_sfx()
{
    static const channel ch = channel::interior_engine_sound;
    if( !g->u.in_vehicle ) {
        fade_audio_channel( ch, 300 );
        add_msg( m_debug, "STOP interior_engine_sound, OUT OF CAR" );
        return;
    }
    if( g->u.in_sleep_state() && !audio_muted ) {
        fade_audio_channel( channel::any, 300 );
        audio_muted = true;
        return;
    } else if( g->u.in_sleep_state() && audio_muted ) {
        return;
    }
    optional_vpart_position vpart_opt = g->m.veh_at( g->u.pos() );
    vehicle *veh;
    if( vpart_opt.has_value() ) {
        veh = &vpart_opt->vehicle();
    } else {
        return;
    }
    if( !veh->engine_on ) {
        fade_audio_channel( ch, 100 );
        add_msg( m_debug, "STOP interior_engine_sound" );
        return;
    }

    std::pair<std::string, std::string> id_and_variant;

    for( size_t e = 0; e < veh->engines.size(); ++e ) {
        if( veh->is_engine_on( e ) ) {
            if( sfx::has_variant_sound( "engine_working_internal",
                                        veh->part_info( veh->engines[ e ] ).get_id().str() ) ) {
                id_and_variant = std::make_pair( "engine_working_internal",
                                                 veh->part_info( veh->engines[ e ] ).get_id().str() );
            } else if( veh->is_engine_type( e, fuel_type_muscle ) ) {
                id_and_variant = std::make_pair( "engine_working_internal", "muscle" );
            } else if( veh->is_engine_type( e, fuel_type_wind ) ) {
                id_and_variant = std::make_pair( "engine_working_internal", "wind" );
            } else if( veh->is_engine_type( e, fuel_type_battery ) ) {
                id_and_variant = std::make_pair( "engine_working_internal", "electric" );
            } else {
                id_and_variant = std::make_pair( "engine_working_internal", "combustion" );
            }
        }
    }

    if( !is_channel_playing( ch ) ) {
        play_ambient_variant_sound( id_and_variant.first, id_and_variant.second,
                                    sfx::get_heard_volume( g->u.pos() ), ch, 1000 );
        add_msg( m_debug, "START %s %s", id_and_variant.first, id_and_variant.second );
    } else {
        add_msg( m_debug, "PLAYING" );
    }
    int current_speed = veh->velocity;
    bool in_reverse = false;
    if( current_speed <= -1 ) {
        current_speed = current_speed * -1;
        in_reverse = true;
    }
    double pitch = 1.0;
    int safe_speed = veh->safe_velocity();
    int current_gear;
    if( in_reverse ) {
        current_gear = -1;
    } else if( current_speed == 0 ) {
        current_gear = 0;
    } else if( current_speed > 0 && current_speed <= safe_speed / 12 ) {
        current_gear = 1;
    } else if( current_speed > safe_speed / 12 && current_speed <= safe_speed / 5 ) {
        current_gear = 2;
    } else if( current_speed > safe_speed / 5 && current_speed <= safe_speed / 4 ) {
        current_gear = 3;
    } else if( current_speed > safe_speed / 4 && current_speed <= safe_speed / 3 ) {
        current_gear = 4;
    } else if( current_speed > safe_speed / 3 && current_speed <= safe_speed / 2 ) {
        current_gear = 5;
    } else {
        current_gear = 6;
    }
    if( veh->has_engine_type( fuel_type_muscle, true ) ||
        veh->has_engine_type( fuel_type_wind, true ) ) {
        current_gear = previous_gear;
    }

    if( current_gear > previous_gear ) {
        play_variant_sound( "vehicle", "gear_shift", get_heard_volume( g->u.pos() ), 0, 0.8, 0.8 );
        add_msg( m_debug, "GEAR UP" );
    } else if( current_gear < previous_gear ) {
        play_variant_sound( "vehicle", "gear_shift", get_heard_volume( g->u.pos() ), 0, 1.2, 1.2 );
        add_msg( m_debug, "GEAR DOWN" );
    }
    if( ( safe_speed != 0 ) ) {
        if( current_gear == 0 ) {
            pitch = 1.0;
        } else if( current_gear == -1 ) {
            pitch = 1.2;
        } else {
            pitch = 1.0 - static_cast<double>( current_speed ) / static_cast<double>( safe_speed );
        }
    }
    if( pitch <= 0.5 ) {
        pitch = 0.5;
    }

    if( current_speed != previous_speed ) {
        Mix_HaltChannel( static_cast<int>( ch ) );
        add_msg( m_debug, "STOP speed %d =/= %d", current_speed, previous_speed );
        play_ambient_variant_sound( id_and_variant.first, id_and_variant.second,
                                    sfx::get_heard_volume( g->u.pos() ), ch, 1000, pitch );
        add_msg( m_debug, "PITCH %f", pitch );
    }
    previous_speed = current_speed;
    previous_gear = current_gear;
}

void sfx::do_vehicle_exterior_engine_sfx()
{
    static const channel ch = channel::exterior_engine_sound;
    static const int ch_int = static_cast<int>( ch );
    // early bail-outs for efficiency
    if( g->u.in_vehicle ) {
        fade_audio_channel( ch, 300 );
        add_msg( m_debug, "STOP exterior_engine_sound, IN CAR" );
        return;
    }
    if( g->u.in_sleep_state() && !audio_muted ) {
        fade_audio_channel( channel::any, 300 );
        audio_muted = true;
        return;
    } else if( g->u.in_sleep_state() && audio_muted ) {
        return;
    }

    VehicleList vehs = g->m.get_vehicles();
    unsigned char noise_factor = 0;
    unsigned char vol = 0;
    vehicle *veh = nullptr;

    for( wrapped_vehicle vehicle : vehs ) {
        if( vehicle.v->vehicle_noise > 0 &&
            vehicle.v->vehicle_noise -
            sound_distance( g->u.pos(), vehicle.v->global_pos3() ) > noise_factor ) {

            noise_factor = vehicle.v->vehicle_noise - sound_distance( g->u.pos(), vehicle.v->global_pos3() );
            veh = vehicle.v;
        }
    }
    if( !noise_factor || !veh ) {
        fade_audio_channel( ch, 300 );
        add_msg( m_debug, "STOP exterior_engine_sound, NO NOISE" );
        return;
    }

    vol = MIX_MAX_VOLUME * noise_factor / veh->vehicle_noise;
    std::pair<std::string, std::string> id_and_variant;

    for( size_t e = 0; e < veh->engines.size(); ++e ) {
        if( veh->is_engine_on( e ) ) {
            if( sfx::has_variant_sound( "engine_working_external",
                                        veh->part_info( veh->engines[ e ] ).get_id().str() ) ) {
                id_and_variant = std::make_pair( "engine_working_external",
                                                 veh->part_info( veh->engines[ e ] ).get_id().str() );
            } else if( veh->is_engine_type( e, fuel_type_muscle ) ) {
                id_and_variant = std::make_pair( "engine_working_external", "muscle" );
            } else if( veh->is_engine_type( e, fuel_type_wind ) ) {
                id_and_variant = std::make_pair( "engine_working_external", "wind" );
            } else if( veh->is_engine_type( e, fuel_type_battery ) ) {
                id_and_variant = std::make_pair( "engine_working_external", "electric" );
            } else {
                id_and_variant = std::make_pair( "engine_working_external", "combustion" );
            }
        }
    }

    if( is_channel_playing( ch ) ) {
        if( engine_external_id_and_variant == id_and_variant ) {
            Mix_SetPosition( ch_int, get_heard_angle( veh->global_pos3() ), 0 );
            set_channel_volume( ch, vol );
            add_msg( m_debug, "PLAYING exterior_engine_sound, vol: ex:%d true:%d", vol, Mix_Volume( ch_int,
                     -1 ) );
        } else {
            engine_external_id_and_variant = id_and_variant;
            Mix_HaltChannel( ch_int );
            add_msg( m_debug, "STOP exterior_engine_sound, change id/var" );
            play_ambient_variant_sound( id_and_variant.first, id_and_variant.second, 128, ch, 0 );
            Mix_SetPosition( ch_int, get_heard_angle( veh->global_pos3() ), 0 );
            set_channel_volume( ch, vol );
            add_msg( m_debug, "START exterior_engine_sound %s %s vol: %d", id_and_variant.first,
                     id_and_variant.second,
                     Mix_Volume( ch_int, -1 ) );
        }
    } else {
        play_ambient_variant_sound( id_and_variant.first, id_and_variant.second, 128, ch, 0 );
        add_msg( m_debug, "Vol: %d %d", vol, Mix_Volume( ch_int, -1 ) );
        Mix_SetPosition( ch_int, get_heard_angle( veh->global_pos3() ), 0 );
        add_msg( m_debug, "Vol: %d %d", vol, Mix_Volume( ch_int, -1 ) );
        set_channel_volume( ch, vol );
        add_msg( m_debug, "START exterior_engine_sound NEW %s %s vol: ex:%d true:%d", id_and_variant.first,
                 id_and_variant.second, vol, Mix_Volume( ch_int, -1 ) );
    }
}

void sfx::do_ambient()
{
    if( g->u.in_sleep_state() && !audio_muted ) {
        fade_audio_channel( channel::any, 300 );
        audio_muted = true;
        return;
    } else if( g->u.in_sleep_state() && audio_muted ) {
        return;
    }
    audio_muted = false;
    const bool is_deaf = g->u.is_deaf();
    const int heard_volume = get_heard_volume( g->u.pos() );
    const bool is_underground = g->u.pos().z < 0;
    const bool is_sheltered = g->is_sheltered( g->u.pos() );
    const bool weather_changed = g->weather.weather != previous_weather;
    // Step in at night time / we are not indoors
    if( is_night( calendar::turn ) && !is_sheltered &&
        !is_channel_playing( channel::nighttime_outdoors_env ) && !is_deaf ) {
        fade_audio_group( group::time_of_day, 1000 );
        play_ambient_variant_sound( "environment", "nighttime", heard_volume,
                                    channel::nighttime_outdoors_env, 1000 );
        // Step in at day time / we are not indoors
    } else if( !is_night( calendar::turn ) && !is_channel_playing( channel::daytime_outdoors_env ) &&
               !is_sheltered && !is_deaf ) {
        fade_audio_group( group::time_of_day, 1000 );
        play_ambient_variant_sound( "environment", "daytime", heard_volume, channel::daytime_outdoors_env,
                                    1000 );
    }
    // We are underground
    if( ( is_underground && !is_channel_playing( channel::underground_env ) &&
          !is_deaf ) || ( is_underground &&
                          weather_changed && !is_deaf ) ) {
        fade_audio_group( group::weather, 1000 );
        fade_audio_group( group::time_of_day, 1000 );
        play_ambient_variant_sound( "environment", "underground", heard_volume, channel::underground_env,
                                    1000 );
        // We are indoors
    } else if( ( is_sheltered && !is_underground &&
                 !is_channel_playing( channel::indoors_env ) && !is_deaf ) ||
               ( is_sheltered && !is_underground &&
                 weather_changed && !is_deaf ) ) {
        fade_audio_group( group::weather, 1000 );
        fade_audio_group( group::time_of_day, 1000 );
        play_ambient_variant_sound( "environment", "indoors", heard_volume, channel::indoors_env, 1000 );
    }
    // We are indoors and it is also raining
    if( g->weather.weather >= WEATHER_DRIZZLE && g->weather.weather <= WEATHER_ACID_RAIN &&
        !is_underground
        && is_sheltered && !is_channel_playing( channel::indoors_rain_env ) ) {
        play_ambient_variant_sound( "environment", "indoors_rain", heard_volume, channel::indoors_rain_env,
                                    1000 );
    }
    if( ( !is_sheltered && g->weather.weather != WEATHER_CLEAR && !is_deaf &&
          !is_channel_playing( channel::outdoors_snow_env ) &&
          !is_channel_playing( channel::outdoors_flurry_env ) &&
          !is_channel_playing( channel::outdoors_thunderstorm_env ) &&
          !is_channel_playing( channel::outdoors_rain_env ) &&
          !is_channel_playing( channel::outdoors_drizzle_env ) &&
          !is_channel_playing( channel::outdoor_blizzard ) )
        || ( !is_sheltered &&
             weather_changed  && !is_deaf ) ) {
        fade_audio_group( group::weather, 1000 );
        // We are outside and there is precipitation
        switch( g->weather.weather ) {
            case WEATHER_ACID_DRIZZLE:
            case WEATHER_DRIZZLE:
            case WEATHER_LIGHT_DRIZZLE:
                play_ambient_variant_sound( "environment", "WEATHER_DRIZZLE", heard_volume,
                                            channel::outdoors_drizzle_env,
                                            1000 );
                break;
            case WEATHER_RAINY:
                play_ambient_variant_sound( "environment", "WEATHER_RAINY", heard_volume,
                                            channel::outdoors_rain_env,
                                            1000 );
                break;
            case WEATHER_ACID_RAIN:
            case WEATHER_THUNDER:
            case WEATHER_LIGHTNING:
                play_ambient_variant_sound( "environment", "WEATHER_THUNDER", heard_volume,
                                            channel::outdoors_thunderstorm_env,
                                            1000 );
                break;
            case WEATHER_FLURRIES:
                play_ambient_variant_sound( "environment", "WEATHER_FLURRIES", heard_volume,
                                            channel::outdoors_flurry_env,
                                            1000 );
                break;
            case WEATHER_CLEAR:
            case WEATHER_SUNNY:
            case WEATHER_CLOUDY:
            case WEATHER_SNOWSTORM:
                play_ambient_variant_sound( "environment", "WEATHER_SNOWSTORM", heard_volume,
                                            channel::outdoor_blizzard,
                                            1000 );
                break;
            case WEATHER_SNOW:
                play_ambient_variant_sound( "environment", "WEATHER_SNOW", heard_volume, channel::outdoors_snow_env,
                                            1000 );
                break;
            case WEATHER_NULL:
            case NUM_WEATHER_TYPES:
                // nothing here, those are pseudo-types, they should not be active at all.
                break;
        }
    }
    // Keep track of weather to compare for next iteration
    previous_weather = g->weather.weather;
}

// firing is the item that is fired. It may be the wielded gun, but it can also be an attached
// gunmod. p is the character that is firing, this may be a pseudo-character (used by monattack/
// vehicle turrets) or a NPC.
void sfx::generate_gun_sound( const player &source_arg, const item &firing )
{
    end_sfx_timestamp = std::chrono::high_resolution_clock::now();
    sfx_time = end_sfx_timestamp - start_sfx_timestamp;
    if( std::chrono::duration_cast<std::chrono::milliseconds> ( sfx_time ).count() < 80 ) {
        return;
    }
    const tripoint source = source_arg.pos();
    int heard_volume = get_heard_volume( source );
    if( heard_volume <= 30 ) {
        heard_volume = 30;
    }

    itype_id weapon_id = firing.typeId();
    int angle = 0;
    int distance = 0;
    std::string selected_sound;
    // this does not mean p == g->u (it could be a vehicle turret)
    if( g->u.pos() == source ) {
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
        distance = sound_distance( g->u.pos(), source );
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

void sfx::generate_melee_sound( const tripoint &source, const tripoint &target, bool hit,
                                bool targ_mon,
                                const std::string &material )
{
    // If creating a new thread for each invocation is to much, we have to consider a thread
    // pool or maybe a single thread that works continuously, but that requires a queue or similar
    // to coordinate its work.
    try {
        std::thread the_thread( sound_thread( source, target, hit, targ_mon, material ) );
        try {
            if( the_thread.joinable() ) {
                the_thread.detach();
            }
        } catch( std::system_error &err ) {
            dbg( D_ERROR ) << "Failed to detach melee sound thread: std::system_error: " << err.what();
        }
    } catch( std::system_error &err ) {
        // not a big deal, just skip playing the sound.
        dbg( D_ERROR ) << "Failed to create melee sound thread: std::system_error: " << err.what();
    }
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
        static const std::set<material_id> fleshy = {
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
        fade_audio_channel( channel::any, 100 );
        audio_muted = true;
        return;
    } else if( ( g->u.in_sleep_state() && audio_muted ) ||
               is_channel_playing( channel::chainsaw_theme ) ) {
        fade_audio_group( group::context_themes, 1000 );
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
        fade_audio_group( group::context_themes, 1000 );
        prev_hostiles = hostiles;
        return;
    } else if( hostiles >= 5 && hostiles <= 9 && !is_channel_playing( channel::danger_low_theme ) ) {
        fade_audio_group( group::context_themes, 1000 );
        play_ambient_variant_sound( "danger_low", "default", 100, channel::danger_low_theme, 1000 );
        prev_hostiles = hostiles;
        return;
    } else if( hostiles >= 10 && hostiles <= 14 &&
               !is_channel_playing( channel::danger_medium_theme ) ) {
        fade_audio_group( group::context_themes, 1000 );
        play_ambient_variant_sound( "danger_medium", "default", 100, channel::danger_medium_theme, 1000 );
        prev_hostiles = hostiles;
        return;
    } else if( hostiles >= 15 && hostiles <= 19 && !is_channel_playing( channel::danger_high_theme ) ) {
        fade_audio_group( group::context_themes, 1000 );
        play_ambient_variant_sound( "danger_high", "default", 100, channel::danger_high_theme, 1000 );
        prev_hostiles = hostiles;
        return;
    } else if( hostiles >= 20 && !is_channel_playing( channel::danger_extreme_theme ) ) {
        fade_audio_group( group::context_themes, 1000 );
        play_ambient_variant_sound( "danger_extreme", "default", 100, channel::danger_extreme_theme, 1000 );
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
    if( g->u.get_stamina() >= g->u.get_stamina_max() * .75 ) {
        fade_audio_group( group::fatigue, 2000 );
        return;
    } else if( g->u.get_stamina() <= g->u.get_stamina_max() * .74
               && g->u.get_stamina() >= g->u.get_stamina_max() * .5 &&
               g->u.male && !is_channel_playing( channel::stamina_75 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_m_low", 100, channel::stamina_75, 1000 );
        return;
    } else if( g->u.get_stamina() <= g->u.get_stamina_max() * .49
               && g->u.get_stamina() >= g->u.get_stamina_max() * .25 &&
               g->u.male && !is_channel_playing( channel::stamina_50 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_m_med", 100, channel::stamina_50, 1000 );
        return;
    } else if( g->u.get_stamina() <= g->u.get_stamina_max() * .24 && g->u.get_stamina() >= 0 &&
               g->u.male && !is_channel_playing( channel::stamina_35 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_m_high", 100, channel::stamina_35, 1000 );
        return;
    } else if( g->u.get_stamina() <= g->u.get_stamina_max() * .74
               && g->u.get_stamina() >= g->u.get_stamina_max() * .5 &&
               !g->u.male && !is_channel_playing( channel::stamina_75 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_f_low", 100, channel::stamina_75, 1000 );
        return;
    } else if( g->u.get_stamina() <= g->u.get_stamina_max() * .49
               && g->u.get_stamina() >= g->u.get_stamina_max() * .25 &&
               !g->u.male && !is_channel_playing( channel::stamina_50 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_f_med", 100, channel::stamina_50, 1000 );
        return;
    } else if( g->u.get_stamina() <= g->u.get_stamina_max() * .24 && g->u.get_stamina() >= 0 &&
               !g->u.male && !is_channel_playing( channel::stamina_35 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_f_high", 100, channel::stamina_35, 1000 );
        return;
    }
}

void sfx::do_hearing_loss( int turns )
{
    g_sfx_volume_multiplier = .1;
    fade_audio_group( group::weather, 50 );
    fade_audio_group( group::time_of_day, 50 );
    // Negative duration is just insuring we stay in sync with player condition,
    // don't play any of the sound effects for going deaf.
    if( turns == -1 ) {
        return;
    }
    play_variant_sound( "environment", "deafness_shock", 100 );
    play_variant_sound( "environment", "deafness_tone_start", 100 );
    if( turns <= 35 ) {
        play_ambient_variant_sound( "environment", "deafness_tone_light", 90, channel::deafness_tone, 100 );
    } else if( turns <= 90 ) {
        play_ambient_variant_sound( "environment", "deafness_tone_medium", 90, channel::deafness_tone,
                                    100 );
    } else if( turns >= 91 ) {
        play_ambient_variant_sound( "environment", "deafness_tone_heavy", 90, channel::deafness_tone, 100 );
    }
}

void sfx::remove_hearing_loss()
{
    stop_sound_effect_fade( channel::deafness_tone, 300 );
    g_sfx_volume_multiplier = 1;
    do_ambient();
}

void sfx::do_footstep()
{
    end_sfx_timestamp = std::chrono::high_resolution_clock::now();
    sfx_time = end_sfx_timestamp - start_sfx_timestamp;
    if( std::chrono::duration_cast<std::chrono::milliseconds> ( sfx_time ).count() > 400 ) {
        int heard_volume = sfx::get_heard_volume( g->u.pos() );
        const auto terrain = g->m.ter( g->u.pos() ).id();
        static const std::set<ter_str_id> grass = {
            ter_str_id( "t_grass" ),
            ter_str_id( "t_shrub" ),
            ter_str_id( "t_shrub_peanut" ),
            ter_str_id( "t_shrub_peanut_harvested" ),
            ter_str_id( "t_shrub_blueberry" ),
            ter_str_id( "t_shrub_blueberry_harvested" ),
            ter_str_id( "t_shrub_strawberry" ),
            ter_str_id( "t_shrub_strawberry_harvested" ),
            ter_str_id( "t_shrub_blackberry" ),
            ter_str_id( "t_shrub_blackberry_harvested" ),
            ter_str_id( "t_shrub_huckleberry" ),
            ter_str_id( "t_shrub_huckleberry_harvested" ),
            ter_str_id( "t_shrub_raspberry" ),
            ter_str_id( "t_shrub_raspberry_harvested" ),
            ter_str_id( "t_shrub_grape" ),
            ter_str_id( "t_shrub_grape_harvested" ),
            ter_str_id( "t_shrub_rose" ),
            ter_str_id( "t_shrub_rose_harvested" ),
            ter_str_id( "t_shrub_hydrangea" ),
            ter_str_id( "t_shrub_hydrangea_harvested" ),
            ter_str_id( "t_shrub_lilac" ),
            ter_str_id( "t_shrub_lilac_harvested" ),
            ter_str_id( "t_underbrush" ),
            ter_str_id( "t_underbrush_harvested_spring" ),
            ter_str_id( "t_underbrush_harvested_summer" ),
            ter_str_id( "t_underbrush_harvested_autumn" ),
            ter_str_id( "t_underbrush_harvested_winter" ),
            ter_str_id( "t_moss" ),
            ter_str_id( "t_grass_white" ),
            ter_str_id( "t_grass_long" ),
            ter_str_id( "t_grass_tall" ),
            ter_str_id( "t_grass_dead" ),
            ter_str_id( "t_grass_golf" ),
            ter_str_id( "t_golf_hole" ),
            ter_str_id( "t_trunk" ),
            ter_str_id( "t_stump" ),
        };
        static const std::set<ter_str_id> dirt = {
            ter_str_id( "t_dirt" ),
            ter_str_id( "t_dirtmound" ),
            ter_str_id( "t_dirtmoundfloor" ),
            ter_str_id( "t_sand" ),
            ter_str_id( "t_clay" ),
            ter_str_id( "t_dirtfloor" ),
            ter_str_id( "t_palisade_gate_o" ),
            ter_str_id( "t_sandbox" ),
            ter_str_id( "t_claymound" ),
            ter_str_id( "t_sandmound" ),
            ter_str_id( "t_rootcellar" ),
            ter_str_id( "t_railroad_rubble" ),
            ter_str_id( "t_railroad_track" ),
            ter_str_id( "t_railroad_track_h" ),
            ter_str_id( "t_railroad_track_v" ),
            ter_str_id( "t_railroad_track_d" ),
            ter_str_id( "t_railroad_track_d1" ),
            ter_str_id( "t_railroad_track_d2" ),
            ter_str_id( "t_railroad_tie" ),
            ter_str_id( "t_railroad_tie_d" ),
            ter_str_id( "t_railroad_tie_d" ),
            ter_str_id( "t_railroad_tie_h" ),
            ter_str_id( "t_railroad_tie_v" ),
            ter_str_id( "t_railroad_tie_d" ),
            ter_str_id( "t_railroad_track_on_tie" ),
            ter_str_id( "t_railroad_track_h_on_tie" ),
            ter_str_id( "t_railroad_track_v_on_tie" ),
            ter_str_id( "t_railroad_track_d_on_tie" ),
            ter_str_id( "t_railroad_tie" ),
            ter_str_id( "t_railroad_tie_h" ),
            ter_str_id( "t_railroad_tie_v" ),
            ter_str_id( "t_railroad_tie_d1" ),
            ter_str_id( "t_railroad_tie_d2" ),
        };
        static const std::set<ter_str_id> metal = {
            ter_str_id( "t_ov_smreb_cage" ),
            ter_str_id( "t_metal_floor" ),
            ter_str_id( "t_grate" ),
            ter_str_id( "t_bridge" ),
            ter_str_id( "t_elevator" ),
            ter_str_id( "t_guardrail_bg_dp" ),
            ter_str_id( "t_slide" ),
            ter_str_id( "t_conveyor" ),
            ter_str_id( "t_machinery_light" ),
            ter_str_id( "t_machinery_heavy" ),
            ter_str_id( "t_machinery_old" ),
            ter_str_id( "t_machinery_electronic" ),
        };
        static const std::set<ter_str_id> water = {
            ter_str_id( "t_water_moving_sh" ),
            ter_str_id( "t_water_moving_dp" ),
            ter_str_id( "t_water_sh" ),
            ter_str_id( "t_water_dp" ),
            ter_str_id( "t_swater_sh" ),
            ter_str_id( "t_swater_dp" ),
            ter_str_id( "t_water_pool" ),
            ter_str_id( "t_sewage" ),
        };
        static const std::set<ter_str_id> chain_fence = {
            ter_str_id( "t_chainfence" ),
        };
        if( !g->u.wearing_something_on( bp_foot_l ) ) {
            play_variant_sound( "plmove", "walk_barefoot", heard_volume, 0, 0.8, 1.2 );
            start_sfx_timestamp = std::chrono::high_resolution_clock::now();
            return;
        } else if( sfx::has_variant_sound( "plmove", terrain.str() ) ) {
            play_variant_sound( "plmove", terrain.str(), heard_volume, 0, 0.8, 1.2 );
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

void sfx::do_obstacle( const std::string &obst )
{
    int heard_volume = sfx::get_heard_volume( g->u.pos() );
    //const auto terrain = g->m.ter( g->u.pos() ).id();
    static const std::set<std::string> water = {
        "t_water_sh",
        "t_water_dp",
        "t_water_moving_sh",
        "t_water_moving_dp",
        "t_swater_sh",
        "t_swater_dp",
        "t_water_pool",
        "t_sewage",
    };
    if( sfx::has_variant_sound( "plmove", obst ) ) {
        play_variant_sound( "plmove", obst, heard_volume, 0, 0.8, 1.2 );
    } else if( water.count( obst ) > 0 ) {
        play_variant_sound( "plmove", "walk_water", heard_volume, 0, 0.8, 1.2 );
    } else {
        play_variant_sound( "plmove", "clear_obstacle", heard_volume, 0, 0.8, 1.2 );
    }
}

void sfx::play_activity_sound( const std::string &id, const std::string &variant, int volume )
{
    if( act != g->u.activity.id() ) {
        act = g->u.activity.id();
        play_ambient_variant_sound( id, variant, volume, channel::player_activities, 0 );
    }
}

void sfx::end_activity_sounds()
{
    act = activity_id::NULL_ID();
    fade_audio_channel( channel::player_activities, 2000 );
}

#else // if defined(SDL_SOUND)

/** Dummy implementations for builds without sound */
/*@{*/
void sfx::load_sound_effects( const JsonObject & ) { }
void sfx::load_sound_effect_preload( const JsonObject & ) { }
void sfx::load_playlist( const JsonObject & ) { }
void sfx::play_variant_sound( const std::string &, const std::string &, int, int, double, double ) { }
void sfx::play_variant_sound( const std::string &, const std::string &, int ) { }
void sfx::play_ambient_variant_sound( const std::string &, const std::string &, int, channel, int,
                                      double, int ) { }
void sfx::play_activity_sound( const std::string &, const std::string &, int ) { }
void sfx::end_activity_sounds() { }
void sfx::generate_gun_sound( const player &, const item & ) { }
void sfx::generate_melee_sound( const tripoint &, const tripoint &, bool, bool,
                                const std::string & ) { }
void sfx::do_hearing_loss( int ) { }
void sfx::remove_hearing_loss() { }
void sfx::do_projectile_hit( const Creature & ) { }
void sfx::do_footstep() { }
void sfx::do_danger_music() { }
void sfx::do_vehicle_engine_sfx() { }
void sfx::do_vehicle_exterior_engine_sfx() { }
void sfx::do_ambient() { }
void sfx::fade_audio_group( group, int ) { }
void sfx::fade_audio_channel( channel, int ) { }
bool sfx::is_channel_playing( channel )
{
    return false;
}
int sfx::set_channel_volume( channel, int )
{
    return 0;
}
bool sfx::has_variant_sound( const std::string &, const std::string & )
{
    return false;
}
void sfx::stop_sound_effect_fade( channel, int ) { }
void sfx::stop_sound_effect_timed( channel, int ) {}
void sfx::do_player_death_hurt( const player &, bool ) { }
void sfx::do_fatigue() { }
void sfx::do_obstacle( const std::string & ) { }
/*@}*/

#endif // if defined(SDL_SOUND)

/** Functions from sfx that do not use the SDL_mixer API at all. They can be used in builds
  * without sound support. */
/*@{*/
int sfx::get_heard_volume( const tripoint &source )
{
    int distance = sound_distance( g->u.pos(), source );
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
    int angle = coord_to_angle( g->u.pos(), source ) + 90;
    //add_msg(m_warning, "angle: %i", angle);
    return ( angle );
}
/*@}*/
