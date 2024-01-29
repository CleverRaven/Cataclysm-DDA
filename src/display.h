#pragma once
#ifndef CATA_SRC_DISPLAY_H
#define CATA_SRC_DISPLAY_H

#include <functional>

#include "widget.h"

// These are the supported data variables for coloring bodygraphs.
enum class bodygraph_var : int {
    hp = 0,      // hitpoints
    temp,        // temperature
    encumb,      // encumbrance
    status,      // limb status (bite, bleeding, ...)
    wet,         // wetness
    last // END OF ENUMS
};

class avatar;
class Character;

struct disp_overmap_cache {
    private:
        tripoint_abs_omt _center;
        tripoint_abs_omt _mission;
        std::string _om_wgt_str;
        int _width;

    public:
        disp_overmap_cache();

        // Returns true if the stored overmap string can be used with the given
        // center (player) position and mission target.
        bool is_valid_for( const tripoint_abs_omt &center,
                           const tripoint_abs_omt &mission, int width ) const {
            return _center == center && _mission == mission && _width == width;
        }

        // Rebuild the cache using the validation parameters "center" and "mission"
        // and store the associated widget string.
        void rebuild( const tripoint_abs_omt &center, const tripoint_abs_omt &mission, int width,
                      const std::string &om_wgt_str ) {
            _center = center;
            _mission = mission;
            _om_wgt_str = om_wgt_str;
            _width = width;
        }

        // Retrieve the cached widget string
        const std::string &get_val() const {
            return _om_wgt_str;
        }
};

struct disp_bodygraph_cache {
    private:
        bodygraph_var _var;
        std::map<bodypart_id, int> _bp_cur_max;
        std::string _graph_id;
        std::string _graph_wgt_str;

    public:
        explicit disp_bodygraph_cache( bodygraph_var var );

        // Returns true if the stored map of current/max HP values differ from the character.
        bool is_valid_for( const Character &u, const std::string &graph_id ) const;

        // Rebuild the cache using the bodypart HP values from the character and
        // store the resulting widget string.
        void rebuild( const Character &u, const std::string &graph_id, const std::string &bg_wgt_str );

        // Retrieve the cached widget string
        const std::string &get_val() const {
            return _graph_wgt_str;
        }
};

// The display namespace contains UI string output and colorization functions
// Some return plain strings or translations, some return a (string, color) pair,
// and some return a string with colorization tags embedded.
namespace display
{
// Functions returning plain strings
// Current date, in terms of day within season, ex. "Summer, day 17"
std::string date_string();
// Approximate time of day on the given turn, ex. "Early morning", "Around dusk"
std::string time_approx( const time_point &turn );
// Current approximate time of day, ex. "Early morning", "Around dusk"
std::string time_approx();
// Exact time if character has a watch, approx time if aboveground, "???" if unknown/underground
std::string time_string( const Character &u );
// Sundial representing the current time of day
std::string sundial_text_color( const Character &u, int width = 0 );
// Sundial representing the current time of day, exact time if player has a watch, "???" if unknown/underground
std::string sundial_time_text_color( const Character &u, int width );

// Temperature at character location, if they have a thermometer
std::string get_temp( const Character &u );

// Text descriptor for given activity level, ex. "Light", "Brisk", "Extreme"
std::string activity_level_str( float level );
// gets the malus string for character's current activity level, like "+ 25%"
std::string activity_malus_str( const Character &u );
// gets the description, printed in player_display, related to your current bmi
std::string weight_long_description( const Character &u );

// Get remotely controlled vehicle, or vehicle character is inside of
vehicle *vehicle_driven( const Character &u );
// For vehicle being driven or remotely piloted by character
// Azimuth (heading) in degrees
std::string vehicle_azimuth_text( const Character &u );
// Vehicle target/current cruise velocity (and units) with engine strain color
std::pair<std::string, nc_color> vehicle_cruise_text_color( const Character &u );
// Vehicle percent of fuel remaining for currently running engine
std::pair<std::string, nc_color> vehicle_fuel_percent_text_color( const Character &u );

// Functions returning (text, color) pairs
std::pair<translation, nc_color> weariness_text_color( size_t weariness );
std::pair<std::string, nc_color> weariness_text_color( const Character &u );
std::pair<std::string, nc_color> weary_malus_text_color( const Character &u );
std::pair<std::string, nc_color> activity_text_color( const Character &u );
std::pair<std::string, nc_color> thirst_text_color( const Character &u );
std::pair<std::string, nc_color> hunger_text_color( const Character &u );
std::pair<std::string, nc_color> weight_text_color( const Character &u );
std::pair<std::string, nc_color> health_text_color( const Character &u );
std::pair<std::string, nc_color> fatigue_text_color( const Character &u );
std::pair<std::string, nc_color> pain_text_color( const Creature &c );
std::pair<std::string, nc_color> pain_text_color( const Character &u );
// Character morale, as a color-coded ascii emoticon face
std::pair<std::string, nc_color> morale_face_color( const avatar &u );
// Helpers for morale_face_color
std::pair<std::string, nc_color> morale_emotion( int morale_cur, const mood_face &face );

// Current movement mode and color, as single letter or full word
std::pair<std::string, nc_color> move_mode_letter_color( const Character &u );
// Movement counter and mode letter, like "50(R)" or "100(W)"
std::pair<std::string, nc_color> move_count_and_mode_text_color( const avatar &u );
// Weight carried, relative to capacity, in %, like "90%"
std::pair<std::string, nc_color> carry_weight_text_color( const avatar &u );

std::pair<std::string, nc_color> temp_text_color( const Character &u,
        const bodypart_str_id &bp = bodypart_str_id::NULL_ID() );
std::pair<std::string, nc_color> power_text_color( const Character &u );
std::pair<std::string, nc_color> power_balance_text_color( const avatar &u );

std::pair<std::string, nc_color> safe_mode_text_color( bool classic_mode );
std::pair<std::string, nc_color> wind_text_color( const Character &u );
std::pair<std::string, nc_color> weather_text_color( const Character &u );

// Get visible threats by cardinal direction - Already colorized
std::string colorized_compass_text( cardinal_direction dir, int width );
std::string colorized_compass_legend_text( int width, int max_height, int &height );

// Get color-coded body graph representing body part HP
std::string colorized_bodygraph_text( const Character &u, const std::string &graph_id,
                                      bodygraph_var var, int width, int max_height, int &height );

// Get color for bodygraph part
nc_color get_bodygraph_bp_color( const Character &u, const bodypart_id &bid,
                                 bodygraph_var var );

// Define color for displaying the body temperature
nc_color bodytemp_color( const Character &u, const bodypart_id &bp );
// Returns color which this limb would have in healing menus
nc_color limb_color( const Character &u, const bodypart_id &bp, bool bleed, bool bite,
                     bool infect );

// Color for displaying the given encumbrance level
nc_color encumb_color( int level );

// Colorized symbol for the overmap tile at the given location
std::pair<std::string, nc_color> overmap_tile_symbol_color( const avatar &u,
        const tripoint_abs_omt &omt, bool edge_tile, bool &found_mi );
// Colorized symbol for an overmap note, given its full text
std::pair<std::string, nc_color> overmap_note_symbol_color( std::string_view note_text );
// Mission marker position as an offset within an overmap of given width and height
point mission_arrow_offset( const avatar &you, int width, int height );
// Fully colorized newline-separated overmap string of the given size, centered on character
std::string colorized_overmap_text( const avatar &u, int width, int height );
// Current overmap position (coordinates)
std::string overmap_position_text( const tripoint_abs_omt &loc );

std::string current_position_text( const tripoint_abs_omt &loc );

// Functions returning colorized string
// gets the string that describes your weight
std::string weight_string( const Character &u );

// Functions returning colorized string
// gets the string that describes your health
std::string health_string( const Character &u );
} // namespace display

#endif // CATA_SRC_DISPLAY_H
