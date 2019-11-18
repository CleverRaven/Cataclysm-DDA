#pragma once
#ifndef SOUNDS_H
#define SOUNDS_H

#include <string>
#include <vector>
#include <utility>

#include "enum_traits.h"

class monster;
class player;
class Creature;
class item;
class JsonObject;
class translation;
struct tripoint;

namespace sounds
{
enum class sound_t : int {
    background = 0,
    weather,
    music,
    movement,
    speech,
    activity,
    destructive_activity,
    alarm,
    combat, // any violent sounding activity
    alert, // louder than speech to get attention
    order,  // loudest to get attention
    _LAST // must always be last
};

// Methods for recording sound events.
/**
 * Sound at (p) of intensity (vol)
 *
 * If the description parameter is a non-empty string, then a string message about the
 * sound is generated for the player.
 *
 * @param p position of sound.
 * @param vol Volume of sound.
 * @param category general type of sound for faster parsing
 * @param description Description of the sound for the player
 * @param ambient Sound does not interrupt player activity if this is true
 * @param id Id of sound effect
 * @param variant Variant of sound effect given in id
 * @returns true if the player could hear the sound.
 */
void sound( const tripoint &p, int vol, sound_t category, const std::string &description,
            bool ambient = false, const std::string &id = "",
            const std::string &variant = "default" );
void sound( const tripoint &p, int vol, sound_t category, const translation &description,
            bool ambient = false, const std::string &id = "",
            const std::string &variant = "default" );
/** Functions identical to sound(..., true). */
void ambient_sound( const tripoint &p, int vol, sound_t category, const std::string &description );
/** Creates a list of coordinates at which to draw footsteps. */
void add_footstep( const tripoint &p, int volume, int distance, monster *source,
                   const std::string &footstep );

/* Make sure the sounds are all reset when we start a new game. */
void reset_sounds();
void reset_markers();

// Methods for processing sound events, these
// process_sounds() applies the sounds since the last turn to monster AI,
void process_sounds();
// process_sound_markers applies sound events to the player and records them for display.
void process_sound_markers( player *p );

// Return list of points that have sound events the player can hear.
std::vector<tripoint> get_footstep_markers();
// Return list of all sounds and the list of sound cluster centroids.
std::pair<std::vector<tripoint>, std::vector<tripoint>> get_monster_sounds();
// retrieve the sound event(s?) at a location.
std::string sound_at( const tripoint &location );
/** Tells us if sound has been enabled in options */
extern bool sound_enabled;
} // namespace sounds

template<>
struct enum_traits<sounds::sound_t> {
    static constexpr auto last = sounds::sound_t::_LAST;
};

namespace sfx
{
//Channel assignments:
enum class channel : int {
    any = -1,                   //Finds the first available channel
    daytime_outdoors_env = 0,
    nighttime_outdoors_env,
    underground_env,
    indoors_env,
    indoors_rain_env,
    outdoors_snow_env,
    outdoors_flurry_env,
    outdoors_thunderstorm_env,
    outdoors_rain_env,
    outdoors_drizzle_env,
    outdoor_blizzard,
    deafness_tone,
    danger_extreme_theme,
    danger_high_theme,
    danger_medium_theme,
    danger_low_theme,
    stamina_75,
    stamina_50,
    stamina_35,
    idle_chainsaw,
    chainsaw_theme,
    player_activities,
    exterior_engine_sound,
    interior_engine_sound,
    radio,
    MAX_CHANNEL                 //the last reserved channel
};

//Group Assignments:
enum class group : int {
    weather = 1,    //SFX related to weather
    time_of_day,    //SFX related to time of day
    context_themes, //SFX related to context themes
    fatigue         //SFX related to fatigue
};

void load_sound_effects( JsonObject &jsobj );
void load_sound_effect_preload( JsonObject &jsobj );
void load_playlist( JsonObject &jsobj );
void play_variant_sound( const std::string &id, const std::string &variant, int volume, int angle,
                         double pitch_min = -1.0, double pitch_max = -1.0 );
void play_variant_sound( const std::string &id, const std::string &variant, int volume );
void play_ambient_variant_sound( const std::string &id, const std::string &variant, int volume,
                                 channel channel, int fade_in_duration, double pitch = -1.0, int loops = -1 );
void play_activity_sound( const std::string &id, const std::string &variant, int volume );
void end_activity_sounds();
void generate_gun_sound( const player &source_arg, const item &firing );
void generate_melee_sound( const tripoint &source, const tripoint &target, bool hit,
                           bool targ_mon = false, const std::string &material = "flesh" );
void do_hearing_loss( int turns = -1 );
void remove_hearing_loss();
void do_projectile_hit( const Creature &target );
int get_heard_volume( const tripoint &source );
int get_heard_angle( const tripoint &source );
void do_footstep();
void do_danger_music();
void do_ambient();
void do_vehicle_engine_sfx();
void do_vehicle_exterior_engine_sfx();
void fade_audio_group( group group, int duration );
void fade_audio_channel( channel channel, int duration );
bool is_channel_playing( channel channel );
bool has_variant_sound( const std::string &id, const std::string &variant );
void stop_sound_effect_fade( channel channel, int duration );
void stop_sound_effect_timed( channel channel, int time );
int set_channel_volume( channel channel, int volume );
void do_player_death_hurt( const player &target, bool death );
void do_fatigue();
// @param obst should be string id of obstacle terrain or vehicle part
void do_obstacle( const std::string &obst = "" );
} // namespace sfx

#endif
