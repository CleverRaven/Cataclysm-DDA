#include "sounds.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include "activity_type.h"
#include "cached_options.h" // IWYU pragma: keep
#include "calendar.h"
#include "character.h"
#include "coordinate_conversions.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "debug.h"
#include "effect.h"
#include "enums.h"
#include "game.h"
#include "game_constants.h"
#include "itype.h" // IWYU pragma: keep
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "music.h"
#include "npc.h"
#include "output.h"
#include "overmapbuffer.h"
#include "player_activity.h"
#include "point.h"
#include "rng.h"
#include "safemode_ui.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "uistate.h"
#include "units.h"
#include "veh_type.h" // IWYU pragma: keep
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_type.h"

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

static int prev_hostiles = 0;
static int previous_speed = 0;
static int previous_gear = 0;
static bool audio_muted = false;
#endif

static weather_type_id previous_weather;
#if defined(SDL_SOUND)
static std::optional<bool> previous_is_night;
#endif
static float g_sfx_volume_multiplier = 1.0f;
static std::chrono::high_resolution_clock::time_point start_sfx_timestamp =
    std::chrono::high_resolution_clock::now();
static std::chrono::high_resolution_clock::time_point end_sfx_timestamp =
    std::chrono::high_resolution_clock::now();
static auto sfx_time = end_sfx_timestamp - start_sfx_timestamp;
static activity_id act;
static std::pair<std::string, std::string> engine_external_id_and_variant;

static const bionic_id bio_sleep_shutdown( "bio_sleep_shutdown" );

static const efftype_id effect_alarm_clock( "alarm_clock" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_slept_through_alarm( "slept_through_alarm" );

static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_muscle( "muscle" );
static const itype_id fuel_type_wind( "wind" );
static const itype_id itype_weapon_fire_suppressed( "weapon_fire_suppressed" );

static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );

static const material_id material_bone( "bone" );
static const material_id material_flesh( "flesh" );
static const material_id material_hflesh( "hflesh" );
static const material_id material_iflesh( "iflesh" );
static const material_id material_steel( "steel" );
static const material_id material_stone( "stone" );
static const material_id material_veggy( "veggy" );

static const skill_id skill_bashing( "bashing" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_stabbing( "stabbing" );

static const ter_str_id ter_t_bridge( "t_bridge" );
static const ter_str_id ter_t_chainfence( "t_chainfence" );
static const ter_str_id ter_t_clay( "t_clay" );
static const ter_str_id ter_t_claymound( "t_claymound" );
static const ter_str_id ter_t_conveyor( "t_conveyor" );
static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_dirtfloor( "t_dirtfloor" );
static const ter_str_id ter_t_dirtmound( "t_dirtmound" );
static const ter_str_id ter_t_dirtmoundfloor( "t_dirtmoundfloor" );
static const ter_str_id ter_t_elevator( "t_elevator" );
static const ter_str_id ter_t_golf_hole( "t_golf_hole" );
static const ter_str_id ter_t_grass( "t_grass" );
static const ter_str_id ter_t_grass_dead( "t_grass_dead" );
static const ter_str_id ter_t_grass_golf( "t_grass_golf" );
static const ter_str_id ter_t_grass_long( "t_grass_long" );
static const ter_str_id ter_t_grass_tall( "t_grass_tall" );
static const ter_str_id ter_t_grass_white( "t_grass_white" );
static const ter_str_id ter_t_grate( "t_grate" );
static const ter_str_id ter_t_guardrail_bg_dp( "t_guardrail_bg_dp" );
static const ter_str_id ter_t_metal_floor( "t_metal_floor" );
static const ter_str_id ter_t_moss( "t_moss" );
static const ter_str_id ter_t_ov_smreb_cage( "t_ov_smreb_cage" );
static const ter_str_id ter_t_palisade_gate_o( "t_palisade_gate_o" );
static const ter_str_id ter_t_railroad_rubble( "t_railroad_rubble" );
static const ter_str_id ter_t_railroad_tie( "t_railroad_tie" );
static const ter_str_id ter_t_railroad_tie_d( "t_railroad_tie_d" );
static const ter_str_id ter_t_railroad_tie_d1( "t_railroad_tie_d1" );
static const ter_str_id ter_t_railroad_tie_d2( "t_railroad_tie_d2" );
static const ter_str_id ter_t_railroad_tie_h( "t_railroad_tie_h" );
static const ter_str_id ter_t_railroad_tie_v( "t_railroad_tie_v" );
static const ter_str_id ter_t_railroad_track( "t_railroad_track" );
static const ter_str_id ter_t_railroad_track_d( "t_railroad_track_d" );
static const ter_str_id ter_t_railroad_track_d1( "t_railroad_track_d1" );
static const ter_str_id ter_t_railroad_track_d2( "t_railroad_track_d2" );
static const ter_str_id ter_t_railroad_track_d_on_tie( "t_railroad_track_d_on_tie" );
static const ter_str_id ter_t_railroad_track_h( "t_railroad_track_h" );
static const ter_str_id ter_t_railroad_track_h_on_tie( "t_railroad_track_h_on_tie" );
static const ter_str_id ter_t_railroad_track_on_tie( "t_railroad_track_on_tie" );
static const ter_str_id ter_t_railroad_track_v( "t_railroad_track_v" );
static const ter_str_id ter_t_railroad_track_v_on_tie( "t_railroad_track_v_on_tie" );
static const ter_str_id ter_t_rootcellar( "t_rootcellar" );
static const ter_str_id ter_t_sand( "t_sand" );
static const ter_str_id ter_t_sandbox( "t_sandbox" );
static const ter_str_id ter_t_sandmound( "t_sandmound" );
static const ter_str_id ter_t_shrub( "t_shrub" );
static const ter_str_id ter_t_shrub_blackberry( "t_shrub_blackberry" );
static const ter_str_id ter_t_shrub_blackberry_harvested( "t_shrub_blackberry_harvested" );
static const ter_str_id ter_t_shrub_blueberry( "t_shrub_blueberry" );
static const ter_str_id ter_t_shrub_blueberry_harvested( "t_shrub_blueberry_harvested" );
static const ter_str_id ter_t_shrub_grape( "t_shrub_grape" );
static const ter_str_id ter_t_shrub_grape_harvested( "t_shrub_grape_harvested" );
static const ter_str_id ter_t_shrub_huckleberry( "t_shrub_huckleberry" );
static const ter_str_id ter_t_shrub_huckleberry_harvested( "t_shrub_huckleberry_harvested" );
static const ter_str_id ter_t_shrub_hydrangea( "t_shrub_hydrangea" );
static const ter_str_id ter_t_shrub_hydrangea_harvested( "t_shrub_hydrangea_harvested" );
static const ter_str_id ter_t_shrub_lilac( "t_shrub_lilac" );
static const ter_str_id ter_t_shrub_lilac_harvested( "t_shrub_lilac_harvested" );
static const ter_str_id ter_t_shrub_peanut( "t_shrub_peanut" );
static const ter_str_id ter_t_shrub_peanut_harvested( "t_shrub_peanut_harvested" );
static const ter_str_id ter_t_shrub_raspberry( "t_shrub_raspberry" );
static const ter_str_id ter_t_shrub_raspberry_harvested( "t_shrub_raspberry_harvested" );
static const ter_str_id ter_t_shrub_rose( "t_shrub_rose" );
static const ter_str_id ter_t_shrub_rose_harvested( "t_shrub_rose_harvested" );
static const ter_str_id ter_t_shrub_strawberry( "t_shrub_strawberry" );
static const ter_str_id ter_t_shrub_strawberry_harvested( "t_shrub_strawberry_harvested" );
static const ter_str_id ter_t_slide( "t_slide" );
static const ter_str_id ter_t_stump( "t_stump" );
static const ter_str_id ter_t_trunk( "t_trunk" );
static const ter_str_id ter_t_underbrush( "t_underbrush" );
static const ter_str_id ter_t_underbrush_harvested_autumn( "t_underbrush_harvested_autumn" );
static const ter_str_id ter_t_underbrush_harvested_spring( "t_underbrush_harvested_spring" );
static const ter_str_id ter_t_underbrush_harvested_summer( "t_underbrush_harvested_summer" );
static const ter_str_id ter_t_underbrush_harvested_winter( "t_underbrush_harvested_winter" );

static const trait_id trait_HEAVYSLEEPER( "HEAVYSLEEPER" );
static const trait_id trait_HEAVYSLEEPER2( "HEAVYSLEEPER2" );

struct monster_sound_event {
    int volume;
    bool provocative;
};

struct sound_event {
    int volume;
    sounds::sound_t category;
    std::string description;
    bool ambient;
    bool footstep;
    std::string id;
    std::string variant;
    std::string season;
};

struct centroid {
    // Values have to be floats to prevent rounding errors.
    float x;
    float y;
    float z;
    float volume;
    float weight;
    bool provocative;
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
    case sounds::sound_t::LAST: break;
    }
    cata_fatal( "Invalid valid_target" );
}
// *INDENT-ON*
} // namespace io

// Static globals tracking sounds events of various kinds.
// The sound events since the last monster turn.
static std::vector<std::pair<tripoint, monster_sound_event>> recent_sounds;
// The sound events since the last interactive player turn. (doesn't count sleep etc)
static std::vector<std::pair<tripoint, sound_event>> sounds_since_last_turn;
// The sound events currently displayed to the player.
static std::unordered_map<tripoint, sound_event> sound_markers;

// This is an attempt to handle attenuation of sound for underground areas.
// The main issue it addresses is that you can hear activity
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

static std::string season_str( const season_type &season )
{
    switch( season ) {
        case season_type::SPRING:
            return "spring";
        case season_type::SUMMER:
            return "summer";
        case season_type::AUTUMN:
            return "autumn";
        case season_type::WINTER:
            return "winter";
        default:
            return "";
    }
}

static bool is_provocative( sounds::sound_t category )
{
    switch( category ) {
        case sounds::sound_t::background:
        case sounds::sound_t::weather:
        case sounds::sound_t::music:
        case sounds::sound_t::activity:
        case sounds::sound_t::destructive_activity:
        case sounds::sound_t::alarm:
        case sounds::sound_t::combat:
        case sounds::sound_t::movement:
            return false;
        case sounds::sound_t::speech:
        case sounds::sound_t::electronic_speech:
        case sounds::sound_t::alert:
        case sounds::sound_t::order:
            return true;
        case sounds::sound_t::LAST:
            break;
    }
    cata_fatal( "Invalid sound_t category" );
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
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    recent_sounds.emplace_back( p, monster_sound_event{ vol, is_provocative( category ) } );
    sounds_since_last_turn.emplace_back( p,
                                         sound_event { vol, category, description, ambient,
                                                 false, id, variant, seas_str } );
}

void sounds::sound( const tripoint &p, int vol, sound_t category, const translation &description,
                    bool ambient, const std::string &id, const std::string &variant )
{
    sounds::sound( p, vol, category, description.translated(), ambient, id, variant );
}

void sounds::add_footstep( const tripoint &p, int volume, int, monster *,
                           const std::string &footstep )
{
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    sounds_since_last_turn.emplace_back( p, sound_event { volume,
                                         sound_t::movement, footstep, false, true, "", "", seas_str} );
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

static std::vector<centroid> cluster_sounds( std::vector<std::pair<tripoint, monster_sound_event>>
        input_sounds )
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
            static_cast<float>( input_sounds[index].second.volume ), static_cast<float>( input_sounds[index].second.volume ),
            input_sounds[index].second.provocative
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
        const float volume_sum = static_cast<float>( sound_event_pair.second.volume ) +
                                 found_centroid->weight;
        // Set the centroid location to the average of the two locations, weighted by volume.
        found_centroid->x = static_cast<float>( ( sound_event_pair.first.x *
                                                sound_event_pair.second.volume ) +
                                                ( found_centroid->x * found_centroid->weight ) ) / volume_sum;
        found_centroid->y = static_cast<float>( ( sound_event_pair.first.y *
                                                sound_event_pair.second.volume ) +
                                                ( found_centroid->y * found_centroid->weight ) ) / volume_sum;
        found_centroid->z = static_cast<float>( ( sound_event_pair.first.z *
                                                sound_event_pair.second.volume ) +
                                                ( found_centroid->z * found_centroid->weight ) ) / volume_sum;
        // Set the centroid volume to the larger of the volumes.
        found_centroid->volume = std::max( found_centroid->volume,
                                           static_cast<float>( sound_event_pair.second.volume ) );
        // Set the centroid weight to the sum of the weights.
        found_centroid->weight = volume_sum;
        // Set and keep provocative if any sound in the centroid is provocative
        found_centroid->provocative |= sound_event_pair.second.provocative;
    }
    return sound_clusters;
}

static int get_signal_for_hordes( const centroid &centr )
{
    //Volume in  tiles. Signal for hordes in submaps
    //modify vol using weather vol.Weather can reduce monster hearing
    const int vol = centr.volume - get_weather().weather_id->sound_attn;
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
        add_msg_debug( debugmode::DF_SOUND, "vol %d  vol_hordes %d sig_power %d ", vol, vol_hordes,
                       sig_power );
        return sig_power;
    }
    return 0;
}

void sounds::process_sounds()
{
    std::vector<centroid> sound_clusters = cluster_sounds( recent_sounds );
    const int weather_vol = get_weather().weather_id->sound_attn;
    for( const centroid &this_centroid : sound_clusters ) {
        // Since monsters don't go deaf ATM we can just use the weather modified volume
        // If they later get physical effects from loud noises we'll have to change this
        // to use the unmodified volume for those effects.
        const int vol = this_centroid.volume - weather_vol;
        const tripoint source = tripoint( this_centroid.x, this_centroid.y, this_centroid.z );
        // --- Monster sound handling here ---
        // Alert all hordes
        int sig_power = get_signal_for_hordes( this_centroid );
        if( sig_power > 0 ) {

            const point abs_ms = get_map().getabs( source.xy() );
            // TODO: fix point types
            const point_abs_sm abs_sm( ms_to_sm_copy( abs_ms ) );
            const tripoint_abs_sm target( abs_sm, source.z );
            overmap_buffer.signal_hordes( target, sig_power );
        }
        // Alert all monsters (that can hear) to the sound.
        for( monster &critter : g->all_monsters() ) {
            // TODO: Generalize this to Creature::hear_sound
            const int dist = sound_distance( source, critter.pos() );
            if( vol * 2 > dist ) {
                // Exclude monsters that certainly won't hear the sound
                critter.hear_sound( source, vol, dist, this_centroid.provocative );
            }
        }
        // Trigger sound-triggered traps and ensure they are still valid
        for( const trap *trapType : trap::get_sound_triggered_traps() ) {
            for( const tripoint &tp : get_map().trap_locations( trapType->id ) ) {
                const int dist = sound_distance( source, tp );
                const trap &tr = get_map().tr_at( tp );
                // Exclude traps that certainly won't hear the sound
                if( vol * 2 > dist ) {
                    if( tr.triggered_by_sound( vol, dist ) ) {
                        tr.trigger( tp );
                    }
                }
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
            case sounds::sound_t::LAST:
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
            case sounds::sound_t::LAST:
                debugmsg( "ERROR: Incorrect sound category" );
                return false;
        }
    }
    return true;
}

void sounds::process_sound_markers( Character *you )
{
    bool is_deaf = you->is_deaf();
    const float volume_multiplier = you->hearing_ability();
    const int weather_vol = get_weather().weather_id->sound_attn;
    // NOLINTNEXTLINE(modernize-loop-convert)
    for( std::size_t i = 0; i < sounds_since_last_turn.size(); i++ ) {
        // copy values instead of making references here to fix use-after-free error
        // sounds_since_last_turn may be inserted with new elements inside the loop
        // so the references may become invalid after the vector enlarged its internal buffer
        const tripoint pos = sounds_since_last_turn[i].first;
        const sound_event sound = sounds_since_last_turn[i].second;
        const int distance_to_sound = sound_distance( you->pos(), pos );
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
            if( is_sound_deafening && !you->is_immune_effect( effect_deaf ) ) {
                you->add_effect( effect_deaf, std::min( 4_minutes,
                                                        time_duration::from_turns( felt_volume - 130 ) / 8 ) );
                if( !you->has_flag( json_flag_PAIN_IMMUNE ) ) {
                    you->add_msg_if_player( m_bad, _( "Your eardrums suddenly ache!" ) );
                    if( you->get_pain() < 10 ) {
                        you->mod_pain( rng( 0, 2 ) );
                    }
                }
            }
            continue;
        }

        if( is_sound_deafening && !you->is_immune_effect( effect_deaf ) ) {
            const time_duration deafness_duration = time_duration::from_turns( felt_volume - 130 ) / 4;
            you->add_effect( effect_deaf, deafness_duration );
            if( you->is_deaf() && !is_deaf ) {
                is_deaf = true;
                continue;
            }
        }

        // The heard volume of a sound is the player heard volume, regardless of true volume level.
        const int heard_volume = static_cast<int>( ( raw_volume - weather_vol ) *
                                 volume_multiplier ) - distance_to_sound;

        if( heard_volume <= 0 && pos != you->pos() ) {
            continue;
        }

        // Player volume meter includes all sounds from their tile and adjacent tiles
        if( distance_to_sound <= 1 ) {
            you->volume = std::max( you->volume, heard_volume );
        }

        // Noises from vehicle player is in.
        if( you->controlling_vehicle ) {
            vehicle *veh = veh_pointer_or_null( get_map().veh_at( you->pos() ) );
            const int noise = veh ? static_cast<int>( veh->vehicle_noise ) : 0;

            you->volume = std::max( you->volume, noise );
        }

        // Secure the flag before wake_up() clears the effect
        bool slept_through = you->has_effect( effect_slept_through_alarm );
        // See if we need to wake someone up
        if( you->has_effect( effect_sleep ) ) {
            if( ( ( !( you->has_trait( trait_HEAVYSLEEPER ) ||
                       you->has_trait( trait_HEAVYSLEEPER2 ) ) && dice( 2, 15 ) < heard_volume ) ||
                  ( you->has_trait( trait_HEAVYSLEEPER ) && dice( 3, 15 ) < heard_volume ) ||
                  ( you->has_trait( trait_HEAVYSLEEPER2 ) && dice( 6, 15 ) < heard_volume ) ) &&
                !you->has_effect( effect_narcosis ) &&
                !you->has_bionic( bio_sleep_shutdown ) ) {
                //Not kidding about sleep-through-firefight
                you->wake_up();
                add_msg( m_warning, _( "Something is making noise." ) );
            } else {
                continue;
            }
        }
        const std::string &description = sound.description.empty() ? _( "a noise" ) : sound.description;
        if( you->is_npc() ) {
            if( !sound.ambient ) {
                npc *guy = dynamic_cast<npc *>( you );
                guy->handle_sound( sound.category, description, heard_volume, pos );
            }
            continue;
        }

        if( sound.category == sound_t::music ) {
            music::activate_music_id( music::music_id::sound );
        }

        // don't print our own noise or things without descriptions
        if( !sound.ambient && ( pos != you->pos() ) && !get_map().pl_sees( pos, distance_to_sound ) ) {
            if( uistate.distraction_noise &&
                !you->activity.is_distraction_ignored( distraction_type::noise ) &&
                !get_safemode().is_sound_safe( sound.description, distance_to_sound, you->controlling_vehicle ) ) {
                const std::string query = string_format( _( "Heard %s!" ),
                                          trim_trailing_punctuations( description ) );
                g->cancel_activity_or_ignore_query( distraction_type::noise, query );
            }
        }

        // skip some sounds to avoid message spam
        const bool from_player = pos == you->pos() || ( sound.category == sound_t::movement &&
                                 distance_to_sound <= 1 );
        if( describe_sound( sound.category, from_player ) ) {
            game_message_type severity = m_info;
            if( sound.category == sound_t::combat || sound.category == sound_t::alarm ) {
                severity = m_warning;
            }
            // if we can see it, don't print a direction
            if( pos == you->pos() ) {
                add_msg( severity, _( "From your position you hear %1$s" ), description );
            } else if( you->sees( pos ) ) {
                add_msg( severity, _( "You hear %1$s" ), description );
            } else {
                std::string direction = direction_name( direction_from( you->pos(), pos ) );
                add_msg( severity, _( "From the %1$s you hear %2$s" ), direction, description );
            }
        }

        if( !you->has_effect( effect_sleep ) && you->has_effect( effect_alarm_clock ) &&
            !you->has_flag( STATIC( json_character_flag( "ALARMCLOCK" ) ) ) ) {
            // if we don't have effect_sleep but we're in_sleep_state, either
            // we were trying to fall asleep for so long our alarm is now going
            // off or something disturbed us while trying to sleep
            const bool trying_to_sleep = you->in_sleep_state();
            if( you->get_effect( effect_alarm_clock ).get_duration() == 1_turns ) {
                if( slept_through ) {
                    add_msg( _( "Your alarm clock finally wakes you up." ) );
                } else if( !trying_to_sleep ) {
                    add_msg( _( "Your alarm clock wakes you up." ) );
                } else {
                    add_msg( _( "Your alarm clock goes off and you haven't slept a wink." ) );
                    you->activity.set_to_null();
                }
                add_msg( _( "You turn off your alarm-clock." ) );
                you->get_effect( effect_alarm_clock ).set_duration( 0_turns );
            }
        }

        const std::string &sfx_id = sound.id;
        const std::string &sfx_variant = sound.variant;
        const std::string &sfx_season = sound.season;
        const bool indoors = !is_creature_outside( get_player_character() );
        const bool night = is_night( calendar::turn );
        if( !sfx_id.empty() ) {
            sfx::play_variant_sound( sfx_id, sfx_variant, sfx_season, indoors, night,
                                     sfx::get_heard_volume( pos ) );
        }

        // Place footstep markers.
        if( pos == you->pos() || you->sees( pos ) ) {
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
        const bool diff_z = pos.z != you->posz();

        // Enumerate the valid points the player *cannot* see.
        // Unless the source is on a different z-level, then any point is fine
        std::vector<tripoint> unseen_points;
        for( const tripoint &newp : get_map().points_in_radius( pos, err_offset ) ) {
            if( diff_z || !you->sees( newp ) ) {
                unseen_points.emplace_back( newp );
            }
        }

        // Then place the sound marker in a random one.
        if( !unseen_points.empty() ) {
            sound_markers.emplace( random_entry( unseen_points ), sound );
        }
    }
    if( you->is_avatar() ) {
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
    for( const centroid &sound : sound_clusters ) {
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
    if( test_mode ) {
        return;
    }
    Mix_FadeOutGroup( static_cast<int>( group ), duration );
}

void sfx::fade_audio_channel( channel channel, int duration )
{
    if( test_mode ) {
        return;
    }
    Mix_FadeOutChannel( static_cast<int>( channel ), duration );
}

bool sfx::is_channel_playing( channel channel )
{
    if( test_mode ) {
        return false;
    }
    return Mix_Playing( static_cast<int>( channel ) ) != 0;
}

void sfx::stop_sound_effect_fade( channel channel, int duration )
{
    if( test_mode ) {
        return;
    }
    if( Mix_FadeOutChannel( static_cast<int>( channel ), duration ) == -1 ) {
        dbg( D_ERROR ) << "Failed to stop sound effect: " << Mix_GetError();
    }
}

void sfx::stop_sound_effect_timed( channel channel, int time )
{
    if( test_mode ) {
        return;
    }
    Mix_ExpireChannel( static_cast<int>( channel ), time );
}

int sfx::set_channel_volume( channel channel, int volume )
{
    if( test_mode ) {
        return 0;
    }
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
    if( test_mode ) {
        return;
    }

    static const channel ch = channel::interior_engine_sound;
    const Character &player_character = get_player_character();
    if( !player_character.in_vehicle ) {
        fade_audio_channel( ch, 300 );
        add_msg_debug( debugmode::DF_SOUND, "STOP interior_engine_sound, OUT OF CAR" );
        return;
    }
    if( player_character.in_sleep_state() && !audio_muted ) {
        fade_audio_channel( channel::any, 300 );
        audio_muted = true;
        return;
    } else if( player_character.in_sleep_state() && audio_muted ) {
        return;
    }
    optional_vpart_position vpart_opt = get_map().veh_at( player_character.pos() );
    vehicle *veh;
    if( vpart_opt.has_value() ) {
        veh = &vpart_opt->vehicle();
    } else {
        return;
    }
    if( !veh->engine_on ) {
        fade_audio_channel( ch, 100 );
        add_msg_debug( debugmode::DF_SOUND, "STOP interior_engine_sound" );
        return;
    }

    std::pair<std::string, std::string> id_and_variant;
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( player_character );
    const bool night = is_night( calendar::turn );

    for( const int p : veh->engines ) {
        const vehicle_part &vp = veh->part( p );
        if( !veh->is_engine_on( vp ) ) {
            continue;
        }
        std::string variant = vp.info().id.str();
        if( sfx::has_variant_sound( "engine_working_internal", variant, seas_str, indoors, night ) ) {
            // has special sound variant for this vpart id
        } else if( veh->is_engine_type( vp, fuel_type_muscle ) ) {
            variant = "muscle";
        } else if( veh->is_engine_type( vp, fuel_type_wind ) ) {
            variant = "wind";
        } else if( veh->is_engine_type( vp, fuel_type_battery ) ) {
            variant = "electric";
        } else {
            variant = "combustion";
        }
        id_and_variant = std::make_pair( "engine_working_internal", variant );
    }

    if( !is_channel_playing( ch ) ) {
        play_ambient_variant_sound( id_and_variant.first, id_and_variant.second,
                                    seas_str, indoors, night,
                                    sfx::get_heard_volume( player_character.pos() ), ch, 1000 );
        add_msg_debug( debugmode::DF_SOUND, "START %s %s", id_and_variant.first, id_and_variant.second );
    } else {
        add_msg_debug( debugmode::DF_SOUND, "PLAYING" );
    }
    int current_speed = veh->velocity;
    bool in_reverse = false;
    if( current_speed <= -1 ) {
        current_speed = current_speed * -1;
        in_reverse = true;
    }
    // Getting the safe speed for a stationary vehicle is expensive and unnecessary, so the calculation
    // is delayed until it is needed.
    std::optional<int> safe_speed_cached;
    auto safe_speed = [veh, &safe_speed_cached]() {
        if( !safe_speed_cached ) {
            safe_speed_cached = veh->safe_velocity();
        }
        return *safe_speed_cached;
    };
    int current_gear;
    if( in_reverse ) {
        current_gear = -1;
    } else if( current_speed == 0 ) {
        current_gear = 0;
    } else if( current_speed > 0 && current_speed <= safe_speed() / 12 ) {
        current_gear = 1;
    } else if( current_speed > safe_speed() / 12 && current_speed <= safe_speed() / 5 ) {
        current_gear = 2;
    } else if( current_speed > safe_speed() / 5 && current_speed <= safe_speed() / 4 ) {
        current_gear = 3;
    } else if( current_speed > safe_speed() / 4 && current_speed <= safe_speed() / 3 ) {
        current_gear = 4;
    } else if( current_speed > safe_speed() / 3 && current_speed <= safe_speed() / 2 ) {
        current_gear = 5;
    } else {
        current_gear = 6;
    }
    if( veh->has_engine_type( fuel_type_muscle, true ) ||
        veh->has_engine_type( fuel_type_wind, true ) ) {
        current_gear = previous_gear;
    }

    if( current_gear > previous_gear ) {
        play_variant_sound( "vehicle", "gear_shift", seas_str, indoors, night,
                            get_heard_volume( player_character.pos() ), 0_degrees, 0.8, 0.8 );
        add_msg_debug( debugmode::DF_SOUND, "GEAR UP" );
    } else if( current_gear < previous_gear ) {
        play_variant_sound( "vehicle", "gear_shift", seas_str, indoors, night,
                            get_heard_volume( player_character.pos() ), 0_degrees, 1.2, 1.2 );
        add_msg_debug( debugmode::DF_SOUND, "GEAR DOWN" );
    }
    double pitch = 1.0;
    if( current_gear != 0 ) {
        if( current_gear == -1 ) {
            pitch = 1.2;
        } else if( safe_speed() != 0 ) {
            pitch = 1.0 - static_cast<double>( current_speed ) / static_cast<double>( safe_speed() );
            pitch = std::max( pitch, 0.5 );
        }
    }

    if( current_speed != previous_speed ) {
        Mix_HaltChannel( static_cast<int>( ch ) );
        add_msg_debug( debugmode::DF_SOUND, "STOP speed %d =/= %d", current_speed, previous_speed );
        play_ambient_variant_sound( id_and_variant.first, id_and_variant.second,
                                    seas_str, indoors, night,
                                    sfx::get_heard_volume( player_character.pos() ), ch, 1000, pitch );
        add_msg_debug( debugmode::DF_SOUND, "PITCH %f", pitch );
    }
    previous_speed = current_speed;
    previous_gear = current_gear;
}

void sfx::do_vehicle_exterior_engine_sfx()
{
    if( test_mode ) {
        return;
    }

    static const channel ch = channel::exterior_engine_sound;
    static const int ch_int = static_cast<int>( ch );
    const Character &player_character = get_player_character();
    // early bail-outs for efficiency
    if( player_character.in_vehicle ) {
        fade_audio_channel( ch, 300 );
        add_msg_debug( debugmode::DF_SOUND, "STOP exterior_engine_sound, IN CAR" );
        return;
    }
    if( player_character.in_sleep_state() && !audio_muted ) {
        fade_audio_channel( channel::any, 300 );
        audio_muted = true;
        return;
    } else if( player_character.in_sleep_state() && audio_muted ) {
        return;
    }

    VehicleList vehs = get_map().get_vehicles();
    unsigned char noise_factor = 0;
    unsigned char vol = 0;
    vehicle *veh = nullptr;

    for( wrapped_vehicle vehicle : vehs ) {
        if( vehicle.v->vehicle_noise > 0 &&
            vehicle.v->vehicle_noise -
            sound_distance( player_character.pos(), vehicle.v->global_pos3() ) > noise_factor ) {

            noise_factor = vehicle.v->vehicle_noise - sound_distance( player_character.pos(),
                           vehicle.v->global_pos3() );
            veh = vehicle.v;
        }
    }
    if( !noise_factor || !veh ) {
        fade_audio_channel( ch, 300 );
        add_msg_debug( debugmode::DF_SOUND, "STOP exterior_engine_sound, NO NOISE" );
        return;
    }

    vol = MIX_MAX_VOLUME * noise_factor / veh->vehicle_noise;
    std::pair<std::string, std::string> id_and_variant;
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( player_character );
    const bool night = is_night( calendar::turn );

    for( const int p : veh->engines ) {
        const vehicle_part &vp = veh->part( p );
        if( !veh->is_engine_on( vp ) ) {
            continue;
        }
        std::string variant = vp.info().id.str();
        if( sfx::has_variant_sound( "engine_working_external", variant, seas_str, indoors, night ) ) {
            // has special sound variant for this vpart id
        } else if( veh->is_engine_type( vp, fuel_type_muscle ) ) {
            variant = "muscle";
        } else if( veh->is_engine_type( vp, fuel_type_wind ) ) {
            variant = "wind";
        } else if( veh->is_engine_type( vp, fuel_type_battery ) ) {
            variant = "electric";
        } else {
            variant = "combustion";
        }
        id_and_variant = std::make_pair( "engine_working_external", variant );
    }

    if( is_channel_playing( ch ) ) {
        if( engine_external_id_and_variant == id_and_variant ) {
            Mix_SetPosition( ch_int, to_degrees( get_heard_angle( veh->global_pos3() ) ), 0 );
            set_channel_volume( ch, vol );
            add_msg_debug( debugmode::DF_SOUND, "PLAYING exterior_engine_sound, vol: ex:%d true:%d", vol,
                           Mix_Volume( ch_int, -1 ) );
        } else {
            engine_external_id_and_variant = id_and_variant;
            Mix_HaltChannel( ch_int );
            add_msg_debug( debugmode::DF_SOUND, "STOP exterior_engine_sound, change id/var" );
            play_ambient_variant_sound( id_and_variant.first, id_and_variant.second,
                                        seas_str, indoors, night, 128, ch, 0 );
            Mix_SetPosition( ch_int, to_degrees( get_heard_angle( veh->global_pos3() ) ), 0 );
            set_channel_volume( ch, vol );
            add_msg_debug( debugmode::DF_SOUND, "START exterior_engine_sound %s %s vol: %d",
                           id_and_variant.first,
                           id_and_variant.second,
                           Mix_Volume( ch_int, -1 ) );
        }
    } else {
        play_ambient_variant_sound( id_and_variant.first, id_and_variant.second,
                                    seas_str, indoors, night, 128, ch, 0 );
        add_msg_debug( debugmode::DF_SOUND, "Vol: %d %d", vol, Mix_Volume( ch_int, -1 ) );
        Mix_SetPosition( ch_int, to_degrees( get_heard_angle( veh->global_pos3() ) ), 0 );
        add_msg_debug( debugmode::DF_SOUND, "Vol: %d %d", vol, Mix_Volume( ch_int, -1 ) );
        set_channel_volume( ch, vol );
        add_msg_debug( debugmode::DF_SOUND, "START exterior_engine_sound NEW %s %s vol: ex:%d true:%d",
                       id_and_variant.first,
                       id_and_variant.second, vol, Mix_Volume( ch_int, -1 ) );
    }
}

void sfx::do_ambient()
{
    if( test_mode ) {
        return;
    }

    const Character &player_character = get_player_character();
    if( player_character.in_sleep_state() && !audio_muted ) {
        fade_audio_channel( channel::any, 300 );
        audio_muted = true;
        return;
    } else if( player_character.in_sleep_state() && audio_muted ) {
        return;
    }
    audio_muted = false;
    const bool is_deaf = player_character.is_deaf();
    const int heard_volume = get_heard_volume( player_character.pos() );
    const bool is_underground = player_character.pos().z < 0;
    const bool is_sheltered = g->is_sheltered( player_character.pos() );
    const bool night = is_night( calendar::turn );
    const bool weather_changed =
        get_weather().weather_id != previous_weather ||
        !previous_is_night.has_value() || previous_is_night.value() != night;
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( player_character );
    // Step in at night time / we are not indoors
    if( night && !is_sheltered &&
        !is_channel_playing( channel::nighttime_outdoors_env ) && !is_deaf ) {
        fade_audio_group( group::time_of_day, 1000 );
        play_ambient_variant_sound( "environment", "nighttime", seas_str,
                                    indoors, night, heard_volume,
                                    channel::nighttime_outdoors_env, 1000 );
        // Step in at day time / we are not indoors
    } else if( !night && !is_channel_playing( channel::daytime_outdoors_env ) &&
               !is_sheltered && !is_deaf ) {
        fade_audio_group( group::time_of_day, 1000 );
        play_ambient_variant_sound( "environment", "daytime", seas_str,
                                    indoors, night, heard_volume,
                                    channel::daytime_outdoors_env, 1000 );
    }
    // We are underground
    if( ( is_underground && !is_channel_playing( channel::underground_env ) &&
          !is_deaf ) || ( is_underground &&
                          weather_changed && !is_deaf ) ) {
        fade_audio_group( group::weather, 1000 );
        fade_audio_group( group::time_of_day, 1000 );
        play_ambient_variant_sound( "environment", "underground", seas_str,
                                    indoors, night, heard_volume,
                                    channel::underground_env, 1000 );
        // We are indoors
    } else if( ( is_sheltered && !is_underground &&
                 !is_channel_playing( channel::indoors_env ) && !is_deaf ) ||
               ( is_sheltered && !is_underground &&
                 weather_changed && !is_deaf ) ) {
        fade_audio_group( group::weather, 1000 );
        fade_audio_group( group::time_of_day, 1000 );
        play_ambient_variant_sound( "environment", "indoors", seas_str,
                                    indoors, night, heard_volume,
                                    channel::indoors_env, 1000 );
    }

    // We are indoors and it is also raining
    if( get_weather().weather_id->rains &&
        get_weather().weather_id->precip != precip_class::very_light &&
        !is_underground && is_sheltered && !is_channel_playing( channel::indoors_rain_env ) ) {
        play_ambient_variant_sound( "environment", "indoors_rain", seas_str, indoors,
                                    night, heard_volume, channel::indoors_rain_env, 1000 );
    }
    auto outdoor_playing = []( bool first_only ) {
        static const std::set<channel> outdoor_channels {
            channel::outdoors_snow_env,
            channel::outdoors_flurry_env,
            channel::outdoors_thunderstorm_env,
            channel::outdoors_rainstorm_env,
            channel::outdoors_rain_env,
            channel::outdoors_drizzle_env,
            channel::outdoor_blizzard,
            channel::outdoors_clear_env,
            channel::outdoors_sunny_env,
            channel::outdoors_cloudy_env,
            channel::outdoors_portal_storm_env
        };
        std::set<channel> active_channels;
        for( const channel &ch : outdoor_channels ) {
            if( is_channel_playing( ch ) ) {
                active_channels.emplace( ch );
                if( first_only ) {
                    return active_channels;
                }
            }
        }
        return active_channels;
    };
    if( !is_sheltered && !is_deaf &&
        get_weather().weather_id->sound_category != weather_sound_category::silent ) {
        if( weather_changed || outdoor_playing( true ).empty() ) {
            fade_audio_group( group::weather, 1000 );
            for( const channel &ch : outdoor_playing( false ) ) {
                fade_audio_channel( ch, 1000 );
            }
            // We are outside and there is audible weather
            switch( get_weather().weather_id->sound_category ) {
                case weather_sound_category::drizzle:
                    play_ambient_variant_sound( "environment", "WEATHER_DRIZZLE", seas_str,
                                                indoors, night, heard_volume,
                                                channel::outdoors_drizzle_env, 1000 );
                    break;
                case weather_sound_category::rainy:
                    play_ambient_variant_sound( "environment", "WEATHER_RAINY", seas_str,
                                                indoors, night, heard_volume,
                                                channel::outdoors_rain_env, 1000 );
                    break;
                case weather_sound_category::rainstorm:
                    play_ambient_variant_sound( "environment", "WEATHER_RAINSTORM", seas_str,
                                                indoors, night, heard_volume,
                                                channel::outdoors_rainstorm_env, 1000 );
                    break;
                case weather_sound_category::thunder:
                    play_ambient_variant_sound( "environment", "WEATHER_THUNDER", seas_str,
                                                indoors, night, heard_volume,
                                                channel::outdoors_thunderstorm_env, 1000 );
                    break;
                case weather_sound_category::flurries:
                    play_ambient_variant_sound( "environment", "WEATHER_FLURRIES", seas_str,
                                                indoors, night, heard_volume,
                                                channel::outdoors_flurry_env, 1000 );
                    break;
                case weather_sound_category::snowstorm:
                    play_ambient_variant_sound( "environment", "WEATHER_SNOWSTORM", seas_str,
                                                indoors, night, heard_volume,
                                                channel::outdoor_blizzard, 1000 );
                    break;
                case weather_sound_category::snow:
                    play_ambient_variant_sound( "environment", "WEATHER_SNOW", seas_str,
                                                indoors, night, heard_volume,
                                                channel::outdoors_snow_env, 1000 );
                    break;
                case weather_sound_category::silent:
                    break;
                case weather_sound_category::portal_storm:
                    play_ambient_variant_sound( "environment", "WEATHER_PORTAL_STORM", seas_str,
                                                indoors, night, heard_volume,
                                                channel::outdoors_portal_storm_env, 1000 );
                    break;
                case weather_sound_category::clear:
                    play_ambient_variant_sound( "environment", "WEATHER_CLEAR", seas_str,
                                                indoors, night, heard_volume,
                                                channel::outdoors_clear_env, 1000 );
                    break;
                case weather_sound_category::sunny:
                    play_ambient_variant_sound( "environment", "WEATHER_SUNNY", seas_str,
                                                indoors, night, heard_volume,
                                                channel::outdoors_sunny_env, 1000 );
                    break;
                case weather_sound_category::cloudy:
                    play_ambient_variant_sound( "environment", "WEATHER_CLOUDY", seas_str,
                                                indoors, night, heard_volume,
                                                channel::outdoors_cloudy_env, 1000 );
                    break;
                case weather_sound_category::last:
                    debugmsg( "Invalid weather sound category." );
                    break;
            }
        }
    } else {
        // Sheltered, deaf, or silent weather.
        // Don't play outdoor weather, fade out active audio channels
        std::set<channel> outdoor_active = outdoor_playing( false );
        for( const channel &ch : outdoor_active ) {
            fade_audio_channel( ch, 1000 );
        }
    }
    // Keep track of weather to compare for next iteration
    previous_weather = get_weather().weather_id;
    previous_is_night = night;
}

// firing is the item that is fired. It may be the wielded gun, but it can also be an attached
// gunmod. p is the character that is firing, this may be a pseudo-character (used by monattack/
// vehicle turrets) or a NPC.
void sfx::generate_gun_sound( const Character &source_arg, const item &firing )
{
    if( test_mode ) {
        return;
    }

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
    units::angle angle = 0_degrees;
    int distance = 0;
    std::string selected_sound;
    const Character &player_character = get_player_character();
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( player_character );
    const bool night = is_night( calendar::turn );
    // this does not mean p == avatar (it could be a vehicle turret)
    if( player_character.pos() == source ) {
        selected_sound = "fire_gun";

        const auto mods = firing.gunmods();
        if( std::any_of( mods.begin(), mods.end(),
        []( const item * e ) {
        return e->type->gunmod->loudness < 0;
    } ) && firing.gun_skill().str() != "archery" ) {
            weapon_id = itype_weapon_fire_suppressed;
        }

    } else {
        angle = get_heard_angle( source );
        distance = sound_distance( player_character.pos(), source );
        if( distance <= 17 ) {
            selected_sound = "fire_gun";
        } else {
            selected_sound = "fire_gun_distant";
        }
    }

    play_variant_sound( selected_sound, weapon_id.str(), seas_str, indoors, night,
                        heard_volume, angle, 0.8, 1.2 );
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
    units::angle ang_src;
    int vol_src;
    int vol_targ;
    units::angle ang_targ;

    // Operator overload required for thread API.
    void operator()() const;
};
} // namespace sfx

void sfx::generate_melee_sound( const tripoint &source, const tripoint &target, bool hit,
                                bool targ_mon,
                                const std::string &material )
{
    if( test_mode ) {
        return;
    }
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
    npc *np = get_creature_tracker().creature_at<npc>( source );
    const Character &you = np ? static_cast<Character &>( *np ) :
                           dynamic_cast<Character &>( get_player_character() );
    if( !you.is_npc() ) {
        // sound comes from the same place as the player is, calculation of angle wouldn't work
        ang_src = 0_degrees;
        vol_src = heard_volume;
        vol_targ = heard_volume;
    } else {
        ang_src = get_heard_angle( source );
        vol_src = std::max( heard_volume - 30, 0 );
        vol_targ = std::max( heard_volume - 20, 0 );
    }
    const item_location weapon = you.get_wielded_item();
    ang_targ = get_heard_angle( target );
    weapon_skill = weapon ? weapon->melee_skill() : skill_id::NULL_ID();
    weapon_volume = weapon ? weapon->volume() / units::legacy_volume_factor : 0;
}

// Operator overload required for thread API.
void sfx::sound_thread::operator()() const
{
    // This is function is run in a separate thread. One must be careful and not access game data
    // that might change (e.g. g->u.weapon, the character could switch weapons while this thread
    // runs).
    std::this_thread::sleep_for( std::chrono::milliseconds( rng( 1, 2 ) ) );
    std::string variant_used;
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( get_player_character() );
    const bool night = is_night( calendar::turn );

    if( weapon_skill == skill_bashing && weapon_volume <= 8 ) {
        variant_used = "small_bash";
        play_variant_sound( "melee_swing", "small_bash", seas_str, indoors, night,
                            vol_src, ang_src, 0.8, 1.2 );
    } else if( weapon_skill == skill_bashing && weapon_volume >= 9 ) {
        variant_used = "big_bash";
        play_variant_sound( "melee_swing", "big_bash", seas_str, indoors, night,
                            vol_src, ang_src, 0.8, 1.2 );
    } else if( ( weapon_skill == skill_cutting || weapon_skill == skill_stabbing ) &&
               weapon_volume <= 6 ) {
        variant_used = "small_cutting";
        play_variant_sound( "melee_swing", "small_cutting", seas_str, indoors, night,
                            vol_src, ang_src, 0.8, 1.2 );
    } else if( ( weapon_skill == skill_cutting || weapon_skill == skill_stabbing ) &&
               weapon_volume >= 7 ) {
        variant_used = "big_cutting";
        play_variant_sound( "melee_swing", "big_cutting", seas_str, indoors, night,
                            vol_src, ang_src, 0.8, 1.2 );
    } else {
        variant_used = "default";
        play_variant_sound( "melee_swing", "default", seas_str, indoors, night,
                            vol_src, ang_src, 0.8, 1.2 );
    }
    if( hit ) {
        if( targ_mon ) {
            if( material == "steel" ) {
                std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 12,
                                             weapon_volume * 16 ) ) );
                play_variant_sound( "melee_hit_metal", variant_used, seas_str, indoors,
                                    night, vol_targ, ang_targ, 0.8, 1.2 );
            } else {
                std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 12,
                                             weapon_volume * 16 ) ) );
                play_variant_sound( "melee_hit_flesh", variant_used, seas_str, indoors,
                                    night, vol_targ, ang_targ, 0.8, 1.2 );
            }
        } else {
            std::this_thread::sleep_for( std::chrono::milliseconds( rng( weapon_volume * 9,
                                         weapon_volume * 12 ) ) );
            play_variant_sound( "melee_hit_flesh", variant_used, seas_str, indoors,
                                night, vol_targ, ang_targ, 0.8, 1.2 );
        }
    }
}

void sfx::do_projectile_hit( const Creature &target )
{
    if( test_mode ) {
        return;
    }

    const int heard_volume = sfx::get_heard_volume( target.pos() );
    const units::angle angle = get_heard_angle( target.pos() );
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( get_player_character() );
    const bool night = is_night( calendar::turn );
    if( target.is_monster() ) {
        const monster &mon = dynamic_cast<const monster &>( target );
        static const std::set<material_id> fleshy = {
            material_flesh,
            material_hflesh,
            material_iflesh,
            material_veggy,
            material_bone,
        };
        const bool is_fleshy = std::any_of( fleshy.begin(), fleshy.end(), [&mon]( const material_id & m ) {
            return mon.made_of( m );
        } );

        if( !is_fleshy && mon.made_of( material_stone ) ) {
            play_variant_sound( "bullet_hit", "hit_wall", seas_str, indoors, night,
                                heard_volume, angle, 0.8, 1.2 );
            return;
        } else if( !is_fleshy && mon.made_of( material_steel ) ) {
            play_variant_sound( "bullet_hit", "hit_metal", seas_str, indoors, night,
                                heard_volume, angle, 0.8, 1.2 );
            return;
        } else {
            play_variant_sound( "bullet_hit", "hit_flesh", seas_str, indoors, night,
                                heard_volume, angle, 0.8, 1.2 );
            return;
        }
    }
    play_variant_sound( "bullet_hit", "hit_flesh", seas_str, indoors, night,
                        heard_volume, angle, 0.8, 1.2 );
}

void sfx::do_player_death_hurt( const Character &target, bool death )
{
    if( test_mode ) {
        return;
    }

    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( get_player_character() );
    const bool night = is_night( calendar::turn );
    int heard_volume = get_heard_volume( target.pos() );
    const bool male = target.male;
    if( !male && !death ) {
        play_variant_sound( "deal_damage", "hurt_f", seas_str, indoors, night, heard_volume );
    } else if( male && !death ) {
        play_variant_sound( "deal_damage", "hurt_m", seas_str, indoors, night, heard_volume );
    } else if( !male && death ) {
        play_variant_sound( "clean_up_at_end", "death_f", seas_str, indoors, night, heard_volume );
    } else if( male && death ) {
        play_variant_sound( "clean_up_at_end", "death_m", seas_str, indoors, night, heard_volume );
    }
}

void sfx::do_danger_music()
{
    if( test_mode ) {
        return;
    }

    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    Character &player_character = get_player_character();
    const bool indoors = !is_creature_outside( player_character );
    const bool night = is_night( calendar::turn );
    if( player_character.in_sleep_state() && !audio_muted ) {
        fade_audio_channel( channel::any, 100 );
        audio_muted = true;
        return;
    } else if( ( player_character.in_sleep_state() && audio_muted ) ||
               is_channel_playing( channel::chainsaw_theme ) ) {
        fade_audio_group( group::context_themes, 1000 );
        return;
    }
    audio_muted = false;
    int hostiles = 0;
    for( Creature *&critter : player_character.get_visible_creatures( 40 ) ) {
        if( player_character.attitude_to( *critter ) == Creature::Attitude::HOSTILE ) {
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
        play_ambient_variant_sound( "danger_low", "default", seas_str, indoors,
                                    night, 100, channel::danger_low_theme, 1000 );
        prev_hostiles = hostiles;
        return;
    } else if( hostiles >= 10 && hostiles <= 14 &&
               !is_channel_playing( channel::danger_medium_theme ) ) {
        fade_audio_group( group::context_themes, 1000 );
        play_ambient_variant_sound( "danger_medium", "default", seas_str, indoors,
                                    night, 100, channel::danger_medium_theme, 1000 );
        prev_hostiles = hostiles;
        return;
    } else if( hostiles >= 15 && hostiles <= 19 && !is_channel_playing( channel::danger_high_theme ) ) {
        fade_audio_group( group::context_themes, 1000 );
        play_ambient_variant_sound( "danger_high", "default", seas_str, indoors,
                                    night, 100, channel::danger_high_theme, 1000 );
        prev_hostiles = hostiles;
        return;
    } else if( hostiles >= 20 && !is_channel_playing( channel::danger_extreme_theme ) ) {
        fade_audio_group( group::context_themes, 1000 );
        play_ambient_variant_sound( "danger_extreme", "default", seas_str, indoors,
                                    night, 100, channel::danger_extreme_theme, 1000 );
        prev_hostiles = hostiles;
        return;
    }
    prev_hostiles = hostiles;
}

void sfx::do_fatigue()
{
    if( test_mode ) {
        return;
    }

    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    Character &player_character = get_player_character();
    const bool indoors = !is_creature_outside( player_character );
    const bool night = is_night( calendar::turn );
    /*15: Stamina 75%
    16: Stamina 50%
    17: Stamina 25%*/
    if( player_character.get_stamina() >= player_character.get_stamina_max() * .75 ) {
        fade_audio_group( group::fatigue, 2000 );
        return;
    } else if( player_character.get_stamina() <= player_character.get_stamina_max() * .74 &&
               player_character.get_stamina() >= player_character.get_stamina_max() * .5 &&
               player_character.male && !is_channel_playing( channel::stamina_75 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_m_low", seas_str, indoors,
                                    night, 100, channel::stamina_75, 1000 );
        return;
    } else if( player_character.get_stamina() <= player_character.get_stamina_max() * .49 &&
               player_character.get_stamina() >= player_character.get_stamina_max() * .25 &&
               player_character.male && !is_channel_playing( channel::stamina_50 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_m_med", seas_str, indoors,
                                    night, 100, channel::stamina_50, 1000 );
        return;
    } else if( player_character.get_stamina() <= player_character.get_stamina_max() * .24 &&
               player_character.get_stamina() >= 0 && player_character.male &&
               !is_channel_playing( channel::stamina_35 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_m_high", seas_str, indoors,
                                    night, 100, channel::stamina_35, 1000 );
        return;
    } else if( player_character.get_stamina() <= player_character.get_stamina_max() * .74 &&
               player_character.get_stamina() >= player_character.get_stamina_max() * .5 &&
               !player_character.male && !is_channel_playing( channel::stamina_75 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_f_low", seas_str, indoors,
                                    night, 100, channel::stamina_75, 1000 );
        return;
    } else if( player_character.get_stamina() <= player_character.get_stamina_max() * .49 &&
               player_character.get_stamina() >= player_character.get_stamina_max() * .25 &&
               !player_character.male && !is_channel_playing( channel::stamina_50 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_f_med", seas_str, indoors,
                                    night, 100, channel::stamina_50, 1000 );
        return;
    } else if( player_character.get_stamina() <= player_character.get_stamina_max() * .24 &&
               player_character.get_stamina() >= 0 && !player_character.male &&
               !is_channel_playing( channel::stamina_35 ) ) {
        fade_audio_group( group::fatigue, 1000 );
        play_ambient_variant_sound( "plmove", "fatigue_f_high", seas_str, indoors,
                                    night, 100, channel::stamina_35, 1000 );
        return;
    }
}

void sfx::do_hearing_loss( int turns )
{
    if( test_mode ) {
        return;
    }

    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( get_player_character() );
    const bool night = is_night( calendar::turn );
    g_sfx_volume_multiplier = .1;
    fade_audio_group( group::weather, 50 );
    fade_audio_group( group::time_of_day, 50 );
    // Negative duration is just insuring we stay in sync with player condition,
    // don't play any of the sound effects for going deaf.
    if( turns == -1 ) {
        return;
    }
    play_variant_sound( "environment", "deafness_shock", seas_str, indoors, night, 100 );
    play_variant_sound( "environment", "deafness_tone_start", seas_str, indoors, night, 100 );
    if( turns <= 35 ) {
        play_ambient_variant_sound( "environment", "deafness_tone_light", seas_str,
                                    indoors, night, 90, channel::deafness_tone, 100 );
    } else if( turns <= 90 ) {
        play_ambient_variant_sound( "environment", "deafness_tone_medium", seas_str,
                                    indoors, night, 90, channel::deafness_tone, 100 );
    } else if( turns >= 91 ) {
        play_ambient_variant_sound( "environment", "deafness_tone_heavy", seas_str,
                                    indoors, night, 90, channel::deafness_tone, 100 );
    }
}

void sfx::remove_hearing_loss()
{
    if( test_mode ) {
        return;
    }
    stop_sound_effect_fade( channel::deafness_tone, 300 );
    g_sfx_volume_multiplier = 1;
    do_ambient();
}

void sfx::do_footstep()
{
    if( test_mode ) {
        return;
    }

    end_sfx_timestamp = std::chrono::high_resolution_clock::now();
    sfx_time = end_sfx_timestamp - start_sfx_timestamp;
    if( std::chrono::duration_cast<std::chrono::milliseconds> ( sfx_time ).count() > 400 ) {
        const Character &player_character = get_player_character();
        int heard_volume = sfx::get_heard_volume( player_character.pos() );
        const auto terrain = get_map().ter( player_character.pos() ).id();
        static const std::set<ter_str_id> grass = {
            ter_t_grass,
            ter_t_shrub,
            ter_t_shrub_peanut,
            ter_t_shrub_peanut_harvested,
            ter_t_shrub_blueberry,
            ter_t_shrub_blueberry_harvested,
            ter_t_shrub_strawberry,
            ter_t_shrub_strawberry_harvested,
            ter_t_shrub_blackberry,
            ter_t_shrub_blackberry_harvested,
            ter_t_shrub_huckleberry,
            ter_t_shrub_huckleberry_harvested,
            ter_t_shrub_raspberry,
            ter_t_shrub_raspberry_harvested,
            ter_t_shrub_grape,
            ter_t_shrub_grape_harvested,
            ter_t_shrub_rose,
            ter_t_shrub_rose_harvested,
            ter_t_shrub_hydrangea,
            ter_t_shrub_hydrangea_harvested,
            ter_t_shrub_lilac,
            ter_t_shrub_lilac_harvested,
            ter_t_underbrush,
            ter_t_underbrush_harvested_spring,
            ter_t_underbrush_harvested_summer,
            ter_t_underbrush_harvested_autumn,
            ter_t_underbrush_harvested_winter,
            ter_t_moss,
            ter_t_grass_white,
            ter_t_grass_long,
            ter_t_grass_tall,
            ter_t_grass_dead,
            ter_t_grass_golf,
            ter_t_golf_hole,
            ter_t_trunk,
            ter_t_stump,
        };
        static const std::set<ter_str_id> dirt = {
            ter_t_dirt,
            ter_t_dirtmound,
            ter_t_dirtmoundfloor,
            ter_t_sand,
            ter_t_clay,
            ter_t_dirtfloor,
            ter_t_palisade_gate_o,
            ter_t_sandbox,
            ter_t_claymound,
            ter_t_sandmound,
            ter_t_rootcellar,
            ter_t_railroad_rubble,
            ter_t_railroad_track,
            ter_t_railroad_track_h,
            ter_t_railroad_track_v,
            ter_t_railroad_track_d,
            ter_t_railroad_track_d1,
            ter_t_railroad_track_d2,
            ter_t_railroad_tie,
            ter_t_railroad_tie_d,
            ter_t_railroad_tie_h,
            ter_t_railroad_tie_v,
            ter_t_railroad_track_on_tie,
            ter_t_railroad_track_h_on_tie,
            ter_t_railroad_track_v_on_tie,
            ter_t_railroad_track_d_on_tie,
            ter_t_railroad_tie_d1,
            ter_t_railroad_tie_d2,
        };
        static const std::set<ter_str_id> metal = {
            ter_t_ov_smreb_cage,
            ter_t_metal_floor,
            ter_t_grate,
            ter_t_bridge,
            ter_t_elevator,
            ter_t_guardrail_bg_dp,
            ter_t_slide,
            ter_t_conveyor,
        };
        static const std::set<ter_str_id> chain_fence = {
            ter_t_chainfence,
        };

        const auto play_plmove_sound_variant = [&]( const std::string & variant,
                                               const std::string & season,
                                               const std::optional<bool> &indoors,
        const std::optional<bool> &night ) {
            play_variant_sound( "plmove", variant, season, indoors, night,
                                heard_volume, 0_degrees, 0.8, 1.2 );
            start_sfx_timestamp = std::chrono::high_resolution_clock::now();
        };

        auto veh_displayed_part = get_map().veh_at( player_character.pos() ).part_displayed();

        const season_type seas = season_of_year( calendar::turn );
        const std::string seas_str = season_str( seas );
        const bool indoors = !is_creature_outside( player_character );
        const bool night = is_night( calendar::turn );
        if( !veh_displayed_part && ( terrain->has_flag( ter_furn_flag::TFLAG_DEEP_WATER ) ||
                                     terrain->has_flag( ter_furn_flag::TFLAG_SHALLOW_WATER ) ) ) {
            play_plmove_sound_variant( "walk_water", seas_str, indoors, night );
            return;
        }
        if( player_character.is_barefoot() ) {
            play_plmove_sound_variant( "walk_barefoot", seas_str, indoors, night );
            return;
        }
        if( veh_displayed_part ) {
            const std::string &part_id = veh_displayed_part->part().info().id.str();
            if( has_variant_sound( "plmove", part_id, seas_str, indoors, night ) ) {
                play_plmove_sound_variant( part_id, seas_str, indoors, night );
            } else if( veh_displayed_part->has_feature( VPFLAG_AISLE ) ) {
                play_plmove_sound_variant( "walk_tarmac", seas_str, indoors, night );
            } else {
                play_plmove_sound_variant( "clear_obstacle", seas_str, indoors, night );
            }
            return;
        }
        if( sfx::has_variant_sound( "plmove", terrain.str(), seas_str, indoors, night ) ) {
            play_plmove_sound_variant( terrain.str(), seas_str, indoors, night );
            return;
        }
        if( grass.count( terrain ) > 0 ) {
            play_plmove_sound_variant( "walk_grass", seas_str, indoors, night );
            return;
        }
        if( dirt.count( terrain ) > 0 ) {
            play_plmove_sound_variant( "walk_dirt", seas_str, indoors, night );
            return;
        }
        if( metal.count( terrain ) > 0 ) {
            play_plmove_sound_variant( "walk_metal", seas_str, indoors, night );
            return;
        }
        if( chain_fence.count( terrain ) > 0 ) {
            play_plmove_sound_variant( "clear_obstacle", seas_str, indoors, night );
            return;
        }

        play_plmove_sound_variant( "walk_tarmac", seas_str, indoors, night );
    }
}

void sfx::do_obstacle( const std::string &obst )
{
    if( test_mode ) {
        return;
    }

    end_sfx_timestamp = std::chrono::high_resolution_clock::now();
    sfx_time = end_sfx_timestamp - start_sfx_timestamp;
    if( std::chrono::duration_cast<std::chrono::milliseconds> ( sfx_time ).count() > 400 ) {
        const Character &player_character = get_player_character();
        const season_type seas = season_of_year( calendar::turn );
        const std::string seas_str = season_str( seas );
        const bool indoors = !is_creature_outside( player_character );
        const bool night = is_night( calendar::turn );
        int heard_volume = sfx::get_heard_volume( player_character.pos() );
        if( sfx::has_variant_sound( "plmove", obst, seas_str, indoors, night ) ) {
            play_variant_sound( "plmove", obst, seas_str, indoors, night,
                                heard_volume, 0_degrees, 0.8, 1.2 );
        } else if( ter_str_id( obst ).is_valid() &&
                   ( ter_id( obst )->has_flag( ter_furn_flag::TFLAG_SHALLOW_WATER ) ||
                     ter_id( obst )->has_flag( ter_furn_flag::TFLAG_DEEP_WATER ) ) ) {
            play_variant_sound( "plmove", "walk_water", seas_str, indoors, night,
                                heard_volume, 0_degrees, 0.8, 1.2 );
        } else {
            play_variant_sound( "plmove", "clear_obstacle", seas_str, indoors,
                                night, heard_volume, 0_degrees, 0.8, 1.2 );
        }
        // prevent footsteps from triggering
        start_sfx_timestamp = std::chrono::high_resolution_clock::now();
    }
}

void sfx::play_activity_sound( const std::string &id, const std::string &variant, int volume )
{
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    play_activity_sound( id, variant, seas_str, volume );
}

void sfx::play_activity_sound( const std::string &id, const std::string &variant,
                               const std::string &season, int volume )
{
    if( test_mode ) {
        return;
    }
    Character &player_character = get_player_character();
    const bool indoors = !is_creature_outside( player_character );
    const bool night = is_night( calendar::turn );
    if( act != player_character.activity.id() ) {
        act = player_character.activity.id();
        play_ambient_variant_sound( id, variant, season, indoors, night,
                                    volume, channel::player_activities, 0 );
    }
}

void sfx::end_activity_sounds()
{
    if( test_mode ) {
        return;
    }
    act = activity_id::NULL_ID();
    fade_audio_channel( channel::player_activities, 2000 );
}

void sfx::play_variant_sound( const std::string &id, const std::string &variant, int volume )
{
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( get_player_character() );
    const bool night = is_night( calendar::turn );
    play_variant_sound( id, variant, seas_str, indoors, night, volume );
}

void sfx::play_variant_sound( const std::string &id, const std::string &variant, int volume,
                              units::angle angle, double pitch_min, double pitch_max )
{
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( get_player_character() );
    const bool night = is_night( calendar::turn );
    play_variant_sound( id, variant, seas_str, indoors, night,
                        volume, angle, pitch_min, pitch_max );
}

void sfx::play_ambient_variant_sound( const std::string &id, const std::string &variant, int volume,
                                      channel channel, int fade_in_duration, double pitch, int loops )
{
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( get_player_character() );
    const bool night = is_night( calendar::turn );
    play_ambient_variant_sound( id, variant, seas_str, indoors, night,
                                volume, channel, fade_in_duration, pitch, loops );
}

bool sfx::has_variant_sound( const std::string &id, const std::string &variant )
{
    const season_type seas = season_of_year( calendar::turn );
    const std::string seas_str = season_str( seas );
    const bool indoors = !is_creature_outside( get_player_character() );
    const bool night = is_night( calendar::turn );
    return has_variant_sound( id, variant, seas_str, indoors, night );
}

#else // if defined(SDL_SOUND)

/** Dummy implementations for builds without sound */
/*@{*/
void sfx::load_sound_effects( const JsonObject & ) { }
void sfx::load_sound_effect_preload( const JsonObject & ) { }
void sfx::load_playlist( const JsonObject & ) { }
void sfx::play_variant_sound( const std::string &, const std::string &, int, units::angle, double,
                              double ) { }
void sfx::play_variant_sound( const std::string &, const std::string &, int ) { }
void sfx::play_ambient_variant_sound( const std::string &, const std::string &, int, channel, int,
                                      double, int ) { }
void sfx::play_activity_sound( const std::string &, const std::string &, int ) { }
void sfx::end_activity_sounds() { }
void sfx::generate_gun_sound( const Character &, const item & ) { }
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
void sfx::stop_sound_effect_timed( channel, int ) { }
void sfx::do_player_death_hurt( const Character &, bool ) { }
void sfx::do_fatigue() { }
void sfx::do_obstacle( const std::string & ) { }
void sfx::play_variant_sound( const std::string &, const std::string &, const std::string &,
                              const std::optional<bool> &, const std::optional<bool> &, int ) { }
/*@}*/

#endif // if defined(SDL_SOUND)

/** Functions from sfx that do not use the SDL_mixer API at all. They can be used in builds
  * without sound support. */
/*@{*/
int sfx::get_heard_volume( const tripoint &source )
{
    int distance = sound_distance( get_player_character().pos(), source );
    // fract = -100 / 24
    const float fract = -4.166666f;
    int heard_volume = fract * distance - 1 + 100;
    if( heard_volume <= 0 ) {
        heard_volume = 0;
    }
    heard_volume *= g_sfx_volume_multiplier;
    return heard_volume;
}

units::angle sfx::get_heard_angle( const tripoint &source )
{
    units::angle angle = coord_to_angle( get_player_character().pos(), source ) + 90_degrees;
    //add_msg(m_warning, "angle: %i", angle);
    return angle;
}
/*@}*/
