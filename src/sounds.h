#pragma once
#ifndef SOUNDS_H
#define SOUNDS_H

#include <string>
#include <vector>

#include "enums.h" // For point

class monster;
class player;
class Creature;
class item;
class JsonObject;

namespace sounds
{
enum class sound_t : int {
    background = 0,
    weather,
    music,
    activity,
    movement,
    alarm,
    combat, // any violent sounding activity, including construction
    speech
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
void sound( const tripoint &p, int vol, sound_t category, std::string description,
            bool ambient = false, const std::string &id = "",
            const std::string &variant = "default" );
/** Functions identical to sound(..., true). */
void ambient_sound( const tripoint &p, int vol, sound_t category, const std::string &description );
/** Creates a list of coordinates at which to draw footsteps. */
void add_footstep( const tripoint &p, int volume, int distance, monster *source );

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
}

namespace sfx
{
void load_sound_effects( JsonObject &jsobj );
void load_sound_effect_preload( JsonObject &jsobj );
void load_playlist( JsonObject &jsobj );
void play_variant_sound( const std::string &id, const std::string &variant, int volume, int angle,
                         float pitch_mix = 1.0, float pitch_max = 1.0 );
void play_variant_sound( const std::string &id, const std::string &variant, int volume );
void play_ambient_variant_sound( const std::string &id, const std::string &variant, int volume,
                                 int channel,
                                 int duration );
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
void fade_audio_group( int tag, int duration );
void fade_audio_channel( int tag, int duration );
bool is_channel_playing( int channel );
void stop_sound_effect_fade( int channel, int duration );
void do_player_death_hurt( const player &target, bool death );
void do_fatigue();
void do_obstacle();
}

#endif
