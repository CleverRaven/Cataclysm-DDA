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
     * Sound at (x, y) of intensity (vol)
     * @param x x-position of sound.
     * @param y y-position of sound.
     * @param vol Volume of sound.
     * @param description Description of the sound for the player,
     * if non-empty string a message is generated.
     * @param ambient If false, the sound interrupts player activities.
     * If true, activities continue.
     * @returns true if the player could hear the sound.
     */
    void sound(int x, int y, int vol, std::string description, bool ambient = false);
    /** Functions identical to sound(..., true). */
    void ambient_sound(int x, int y, int vol, std::string description);
    /** Creates a list of coordinates at which to draw footsteps. */
    void add_footstep(int x, int y, int volume, int distance, monster *source);
    /* Make sure the sounds are all reset when we start a new game. */
    void reset_sounds();
    void reset_markers();

    // Methods for processing sound events, these
    // process_sounds() applies the sounds since the last turn to monster AI,
    void process_sounds();
    // process_sound_markers applies sound events to the player and records them for display.
    void process_sound_markers( player *p );

    /** Calculates where footstep marker should appear and puts those points into the result.
     * This is used by both curses and tile rendering code. */
    std::vector<point> get_footstep_markers();

    // Draw sounds as heard by monsters, including clustering.
    void draw_monster_sounds( const point &offset, WINDOW *window );
    // retrieve the sound event(s?) at a location.
    std::string sound_at( const point &location );
}

#endif
