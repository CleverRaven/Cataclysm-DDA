#ifndef SOUNDS_H
#define SOUNDS_H

#include "enums.h" // For point
#include "cursesdef.h" // For WINDOW

#include <vector>
#include <string>

class monster;
class player;

namespace sounds {
    // Methods for recording sound events.
    /**
     * Sound at (p) of intensity (vol)
     * @param p position of sound.
     * @param vol Volume of sound.
     * @param description Description of the sound for the player,
     * if non-empty string a message is generated.
     * @param ambient If false, the sound interrupts player activities.
     * If true, activities continue.
     * @returns true if the player could hear the sound.
     */
    void sound( const tripoint &p, int vol, std::string description, bool ambient = false );
    /** Functions identical to sound(..., true). */
    void ambient_sound( const tripoint &p, int vol, std::string description );
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

    // Draw sounds as heard by monsters, including clustering.
    void draw_monster_sounds( const tripoint &offset, WINDOW *window );
    // retrieve the sound event(s?) at a location.
    std::string sound_at( const tripoint &location );
}

#endif
