#pragma once
#ifndef CATA_SRC_PANELS_H
#define CATA_SRC_PANELS_H

#include <functional>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include "bodypart.h"
#include "color.h"
#include "coordinates.h"
#include "translations.h"
#include "widget.h"

class JsonIn;
class JsonOut;
class avatar;
class Character;
class Creature;
class mood_face;
struct point;

enum face_type : int {
    face_human = 0,
    face_bird,
    face_bear,
    face_cat,
    num_face_types
};

namespace catacurses
{
class window;
} // namespace catacurses

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
// Current date, in terms of day within season, ex. "Summer, day 17"
std::string date_string();
// Current approximate time of day, ex. "Early morning", "Around dusk"
std::string time_approx();
// Exact time if character has a watch, approx time if aboveground, "???" if unknown/underground
std::string time_string( const Character &u );

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
std::pair<std::string, nc_color> weary_malus_text_color( const Character &u );
std::pair<std::string, nc_color> activity_text_color( const Character &u );
std::pair<std::string, nc_color> thirst_text_color( const Character &u );
std::pair<std::string, nc_color> hunger_text_color( const Character &u );
std::pair<std::string, nc_color> weight_text_color( const Character &u );
std::pair<std::string, nc_color> fatigue_text_color( const Character &u );
std::pair<std::string, nc_color> health_text_color( const Character &u );
std::pair<std::string, nc_color> pain_text_color( const Creature &c );
std::pair<std::string, nc_color> pain_text_color( const Character &u );
// Change in character body temperature, as colorized arrows
std::pair<std::string, nc_color> temp_delta_arrows( const Character &u );
// Character morale, as a color-coded ascii emoticon face
std::pair<std::string, nc_color> morale_face_color( const avatar &u );
// Helpers for morale_face_color
std::pair<std::string, nc_color> morale_emotion( const int morale_cur, const mood_face &face );

// Current movement mode and color, as single letter or full word
std::pair<std::string, nc_color> move_mode_letter_color( const Character &u );
std::pair<std::string, nc_color> move_mode_text_color( const Character &u );

std::pair<std::string, nc_color> temp_text_color( const Character &u );
std::pair<std::string, nc_color> power_text_color( const Character &u );
std::pair<std::string, nc_color> mana_text_color( const Character &you );
std::pair<std::string, nc_color> str_text_color( const Character &p );
std::pair<std::string, nc_color> dex_text_color( const Character &p );
std::pair<std::string, nc_color> int_text_color( const Character &p );
std::pair<std::string, nc_color> per_text_color( const Character &p );
std::pair<std::string, nc_color> safe_mode_text_color( const bool classic_mode );
std::pair<std::string, nc_color> wind_text_color( const Character &u );
std::pair<std::string, nc_color> weather_text_color( const Character &u );

// Define color for displaying the body temperature
nc_color bodytemp_color( const Character &u, const bodypart_id &bp );
// Returns color which this limb would have in healing menus
nc_color limb_color( const Character &u, const bodypart_id &bp, bool bleed, bool bite,
                     bool infect );
// Color for displaying the given encumbrance level
nc_color encumb_color( const int level );

// Color name for given irradiation level, used for radiation badge description
std::string rad_badge_color_name( const int rad );
// Color for displaying the given irradiation level
nc_color rad_badge_color( const int rad );
// Highlighted badge color for character's radiation badge, if they have one
std::pair<std::string, nc_color> rad_badge_text_color( const Character &u );

// Functions returning colorized string
// gets the string that describes your weight
std::string weight_string( const Character &u );

// Prints a list of nearby monsters
void print_mon_info( const avatar &u, const catacurses::window &, int hor_padding = 0,
                     bool compact = false );
} // namespace display

namespace overmap_ui
{
void draw_overmap_chunk( const catacurses::window &w_minimap, const avatar &you,
                         const tripoint_abs_omt &global_omt, const point &start, int width,
                         int height );
} // namespace overmap_ui

bool default_render();

// Arguments to pass into the static draw function (in window_panel::draw)
// Includes public avatar (_ava) and window (_win) references, and private widget reference
// passed to the constructor, accessible with get_widget().
struct draw_args {
    public:
        const avatar &_ava;
        const catacurses::window &_win;

        draw_args( const avatar &a, const catacurses::window &w, const widget_id &wgt ) :
            _ava( a ), _win( w ), _wgt( wgt ) {}

        widget *get_widget() const {
            return _wgt.is_null() ? nullptr : const_cast<widget *>( &*_wgt );
        }
    private:
        widget_id _wgt;
};

// A window_panel is a rectangular region or drawable area within the sidebar window.
// It corresponds to a section that the player may toggle or rearrange from the in-game sidebar options.
// It is associated with a draw function (taking draw_args with avatar and window), along with id and name.
// The height, width, and default toggle state must be provided to the constructor as well.
class window_panel
{
    public:
        window_panel( const std::function<void( const draw_args & )> &draw_func,
                      const std::string &id, const translation &nm, int ht, int wd, bool default_toggle_,
                      const std::function<bool()> &render_func = default_render, bool force_draw = false );

        std::function<void( const draw_args & )> draw;
        std::function<bool()> render;

        int get_height() const;
        int get_width() const;
        const std::string &get_id() const;
        std::string get_name() const;
        void set_widget( const widget_id &w );
        const widget_id &get_widget() const;
        bool toggle;
        bool always_draw;

    private:
        int height;
        int width;
        widget_id wgt;
        std::string id;
        translation name;
};

// A panel_layout is a collection of window_panels drawn in order from top to bottom.
// It is associated with the user-selectable layouts named "classic", "compact", "labels", etc.
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

// The panel_manager allows the player choose their current panel layout and window panels.
// The player's selected panel_layout, enabled window_panels and what order they appear in,
// are saved to the PATH_INFO::panel_options() file, typically config/panel_options.json.
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

        friend widget;
};

#endif // CATA_SRC_PANELS_H
