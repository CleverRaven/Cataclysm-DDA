#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <string>

enum option_key {
OPT_NULL = 0,
OPT_FORCE_YN,            // Y/N versus y/n
OPT_USE_CELSIUS,         // Display temp as C not F
OPT_USE_METRIC_SYS,      // Display speed as Km/h not mph
OPT_NO_CBLINK,           // No bright backgrounds
OPT_24_HOUR,             // 24 hour time
OPT_SNAP_TO_TARGET,      // game::firing snaps to target
OPT_SAFEMODE,            // Safemode on by default?
OPT_SAFEMODEPROXIMITY,   // Range after which safemode kicks in
OPT_AUTOSAFEMODE,        // Autosafemode on by default?
OPT_AUTOSAFEMODETURNS,   // Number of turns untill safemode kicks back in
OPT_AUTOSAVE,            // Automatically save the game on intervals.
OPT_AUTOSAVE_TURNS,      // Turns between autosaves
OPT_AUTOSAVE_MINUTES,    // Minimum realtime minutes between autosaves
OPT_GRADUAL_NIGHT_LIGHT, // be so cool at night :)
OPT_RAIN_ANIMATION,      // Enable the rain and other weather animation
OPT_CIRCLEDIST,          // Compute distance with pythagorean theorem
OPT_QUERY_DISASSEMBLE,   // Query before disassembling items
OPT_DROP_EMPTY,          // auto drop empty containers after use
OPT_HIDE_CURSOR,         // hide mouse cursor
OPT_SKILL_RUST,          // level of skill rust
OPT_DELETE_WORLD,        // Delete workd every time New Character is created
OPT_INITIAL_POINTS,      // Set the number of character points
OPT_MAX_TRAIT_POINTS,    // Set the number of trait points
OPT_INITIAL_TIME,        // Sets the starting hour (0-24)
OPT_VIEWPORT_X,          // Set the width of the terrain window, in characters
OPT_VIEWPORT_Y,          // Set the height of the terrain window, in characters
OPT_MOVE_VIEW_OFFSET,    // Sensitivity of shift+(movement)
OPT_STATIC_SPAWN,        // Makes zombies spawn using the new static system
OPT_CLASSIC_ZOMBIES,     // Only spawn the more classic zombies and buildings.
OPT_SEASON_LENGTH,       // Season length, in days
OPT_STATIC_NPC,          // Spawn static npcs
OPT_RANDOM_NPC,          // Spawn random npcs
OPT_RAD_MUTATION,        // Radiation mutates
OPT_SAVESLEEP,           // Ask to save before sleeping
OPT_AUTO_PICKUP,         // Enable Item Auto Pickup
OPT_AUTO_PICKUP_ZERO,    // Auto Pickup 0 Volume and Weith items
NUM_OPTION_KEYS
};

struct option_table
{
    double options[NUM_OPTION_KEYS];

    option_table()
    {
        for (int i = 0; i < NUM_OPTION_KEYS; i++)
        {   //setup default values where needed
            switch(i)
            {
            case OPT_VIEWPORT_X:
            case OPT_VIEWPORT_Y:
                options[i] = 12;
                break;
            case OPT_INITIAL_TIME:
                options[i] = 8;
                break;
            case OPT_SEASON_LENGTH:
                options[i] = 14;
                break;
            case OPT_MOVE_VIEW_OFFSET:
                options[i] = 1;
                break;
            case OPT_AUTOSAVE_TURNS:
                options[i] = 30;
                break;
            case OPT_AUTOSAVE_MINUTES:
                options[i] = 5;
                break;
            case OPT_MAX_TRAIT_POINTS:
                options[i] = 12;
                break;
            default:
                options[i] = 0;
            }
        }
    };

    double& operator[] (option_key i) { return options[i]; };
    double& operator[] (int i) { return options[i]; };
};

extern option_table OPTIONS;

bool option_is_bool(option_key id);
char option_max_options(option_key id);
char option_min_options(option_key id);
void show_options();
void load_options();
void save_options();
std::string option_string(option_key key);
std::string option_name(option_key key);
std::string option_desc(option_key key);

#endif
