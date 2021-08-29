#pragma once
#ifndef CATA_SRC_PANELS_H
#define CATA_SRC_PANELS_H

#include <functional>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include "color.h"
#include "coordinates.h"
#include "translations.h"

class JsonIn;
class JsonOut;
class avatar;
class Character;
class Creature;
struct point;

enum face_type : int {
    face_human = 0,
    face_bird,
    face_bear,
    face_cat,
    num_face_types
};

// The display namespace contains UI string output and colorization functions
// Some return plain strings or translations, some return a (string, color) pair,
// and some return a string with colorization tags embedded.
namespace display
{
// Functions returning plain strings
// Current moon phase, ex. "Full moon", "Waxing crescent"
std::string get_moon();
// Current moon phase as ascii-art, ex. "(   )", "(  ))"
std::string get_moon_graphic();
// Current approximate time of day, ex. "Early morning", "Around dusk"
std::string time_approx();

// Temperature at character location, if they have a thermometer
std::string get_temp( const Character &u );
// Change in character body temperature, ex. "(Rising)", "(Falling!!)"
std::string temp_delta_string( const Character &u );

// Text descriptor for given activity level, ex. "Light", "Brisk", "Extreme"
std::string activity_level_str( float level );
// gets the malus string for character's current activity level, like "+ 25%"
std::string activity_malus_str( const Character &u );
// gets the description, printed in player_display, related to your current bmi
std::string weight_long_description( const Character &u );

// Functions returning (text, color) pairs
std::pair<translation, nc_color> weariness_text_color( size_t weariness );
std::pair<std::string, nc_color> weariness_text_color( const Character &u );
std::pair<std::string, nc_color> activity_text_color( const Character &u );
std::pair<std::string, nc_color> thirst_text_color( const Character &u );
std::pair<std::string, nc_color> hunger_text_color( const Character &u );
std::pair<std::string, nc_color> weight_text_color( const Character &u );
std::pair<std::string, nc_color> fatigue_text_color( const Character &u );
std::pair<std::string, nc_color> pain_text_color( const Creature &c );
std::pair<std::string, nc_color> pain_text_color( const Character &u );
// Change in character body temperature, as colorized arrows
std::pair<std::string, nc_color> temp_delta_arrows( const Character &u );
// Character morale, as a color-coded ascii emoticon face
std::pair<std::string, nc_color> morale_face_color( const Character &u );
// Helpers for morale_face_color
face_type get_face_type( const Character &u );
std::string morale_emotion( const int morale_cur, const face_type face,
                            const bool horizontal_style );

// Current movement mode (as single letter) and color
std::pair<std::string, nc_color> move_mode_text_color( const Character &u );

// TODO: Swap text/string order to match previous functions
std::pair<std::string, nc_color> temp_text_color( const Character &u );
std::pair<std::string, nc_color> power_text_color( const Character &u );
std::pair<std::string, nc_color> mana_text_color( const Character &you );
std::pair<std::string, nc_color> str_text_color( const Character &p );
std::pair<std::string, nc_color> dex_text_color( const Character &p );
std::pair<std::string, nc_color> int_text_color( const Character &p );
std::pair<std::string, nc_color> per_text_color( const Character &p );

// Functions returning colorized string
// gets the string that describes your weight
std::string weight_string( const Character &u );
} // namespace display

namespace catacurses
{
class window;
} // namespace catacurses

namespace overmap_ui
{
void draw_overmap_chunk( const catacurses::window &w_minimap, const avatar &you,
                         const tripoint_abs_omt &global_omt, const point &start, int width,
                         int height );
} // namespace overmap_ui

bool default_render();

class window_panel
{
    public:
        window_panel( const std::function<void( avatar &, const catacurses::window & )> &draw_func,
                      const std::string &id, const translation &nm, int ht, int wd, bool default_toggle_,
                      const std::function<bool()> &render_func = default_render, bool force_draw = false );

        std::function<void( avatar &, const catacurses::window & )> draw;
        std::function<bool()> render;

        int get_height() const;
        int get_width() const;
        const std::string &get_id() const;
        std::string get_name() const;
        bool toggle;
        bool always_draw;

    private:
        int height;
        int width;
        std::string id;
        translation name;
};

class panel_layout
{
    public:
        panel_layout( const translation &_name,
                      const std::vector<window_panel> &_panels );

        std::string name() const;
        const std::vector<window_panel> &panels() const;
        std::vector<window_panel> &panels();
    private:
        translation _name;
        std::vector<window_panel> _panels;
};

class panel_manager
{
    public:
        panel_manager();
        ~panel_manager() = default;
        panel_manager( panel_manager && ) = default;
        panel_manager( const panel_manager & ) = default;
        panel_manager &operator=( panel_manager && ) = default;
        panel_manager &operator=( const panel_manager & ) = default;

        static panel_manager &get_manager() {
            static panel_manager single_instance;
            return single_instance;
        }

        panel_layout &get_current_layout();
        std::string get_current_layout_id() const;
        int get_width_right();
        int get_width_left();

        void show_adm();

        void init();

    private:
        bool save();
        bool load();
        void serialize( JsonOut &json );
        void deserialize( JsonIn &jsin );
        // update the screen offsets so the game knows how to adjust the main window
        void update_offsets( int x );

        // The amount of screen space from each edge the sidebar takes up
        int width_right = 0; // NOLINT(cata-serialize)
        int width_left = 0; // NOLINT(cata-serialize)
        std::string current_layout_id;
        std::map<std::string, panel_layout> layouts;

};

#endif // CATA_SRC_PANELS_H
