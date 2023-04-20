#pragma once
#ifndef CATA_SRC_WIDGET_H
#define CATA_SRC_WIDGET_H

#include <string>
#include <vector>

#include "avatar.h"
#include "enum_traits.h"
#include "generic_factory.h"
#include "panels.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"

// These are the supported data variables for widgets, defined as enum widget_var.
// widget_var names may be given as the "var" field in widget JSON.
enum class widget_var : int {
    focus,          // Current focus, integer
    move,           // Current move counter, integer
    move_remainder, // Current remaining moves, integer
    move_cost,      // Modified base movement cost, integer (from run_cost)
    pain,           // Current perceived pain, integer
    sound,          // Current sound level, integer
    speed,          // Current speed, integer
    stamina,        // Current stamina 0-10000, greater being fuller stamina reserves
    fatigue,        // Current fatigue, integer
    health,         // Current hidden health value, -200 to +200
    mana,           // Current available mana, integer
    max_mana,       // Current maximum mana, integer
    power_percentage, // Bionic power, relative to capacity
    log_power_balance, // Logarithm of bionic power balance
    morale_level,   // Current morale level, integer (may be negative)
    weariness_level, // Current weariness level, integer
    weary_transition_level, // Current weariness level, integer
    stat_str,       // Base STR (strength) stat, integer
    stat_dex,       // Base DEX (dexterity) stat, integer
    stat_int,       // Base INT (intelligence) stat, integer
    stat_per,       // Base PER (perception) stat, integer
    bp_hp,          // Current hit points of given "bodypart", integer
    bp_encumb,      // Current encumbrance of given "bodypart", integer
    bp_warmth,      // Current warmth of given "bodypart", integer
    bp_wetness,     // Current wetness of given "bodypart", integer
    mood,           // TODO
    cardio_fit,     // Cardio fitness, integer near BMR
    cardio_acc,     // Cardio accumulator, integer
    carry_weight,   // Weight carried, relative to capacity, in % (0 - >100)
    // Text vars
    activity_text,  // Activity level text, color string
    body_graph,     // Body graph showing color-coded body part health
    body_graph_temp,     // Body graph showing color-coded body part temperature
    body_graph_encumb,     // Body graph showing color-coded body part encumbrance
    body_graph_status,     // Body graph showing color-coded body part status (bite, bleeding, ...)
    body_graph_wet,        // Body graph showing color-coded body part wetness
    bp_armor_outer_text, // Outermost armor on body part, with color/damage bars
    carry_weight_text,   // Weight carried, relative to capacity, in %
    compass_text,   // Compass / visible threats by cardinal direction
    compass_legend_text, // Names of visible creatures that appear on the compass
    date_text,      // Current date, in terms of day within season
    env_temp_text,  // Environment temperature, if character has thermometer
    mood_text,      // Mood as a text emote, color string
    move_count_mode_text, // Movement counter and mode letter like "50(R)", color string
    overmap_loc_text,// Local overmap position, pseudo latitude/longitude with Z-level
    overmap_text,   // Local overmap and mission marker, multi-line color string
    pain_text,      // Pain description text, color string
    place_text,     // Place name in world where character is
    power_text,     // Remaining power from bionics, color string
    power_balance_text, // Power balance during the last turn
    safe_mode_text, // Safe mode text, color string
    safe_mode_classic_text, // Safe mode text, classic mode color string.
    style_text,     // Active martial arts style name
    sundial_text,   // Sundial representing the time of day
    sundial_time_text,   // Current time - exact if character has a watch, sundial otherwise
    time_text,      // Current time - exact if character has a watch, approximate otherwise
    veh_azimuth_text, // Azimuth or heading in degrees, string
    veh_cruise_text, // Current/target cruising speed in vehicle, color string
    veh_fuel_text,  // Current/total fuel for active vehicle engine, color string
    weariness_text, // Weariness description text, color string
    weary_malus_text, // Weariness malus or penalty
    weather_text,   // Weather/sky conditions (if visible), color string
    wielding_text,  // Currently wielded weapon or item name
    wielding_simple_text,  // Currently wielded weapon or item name, without mode and ammo
    wielding_mode_text,  // Currently wielded weapon's firing mode
    wielding_ammo_text,  // Currently wielded weapon's ammo
    wind_text,      // Wind level and direction, color string
    last // END OF ENUMS
};

// Use enum_traits for generic iteration over widget_var, and string (de-)serialization.
// Use io::string_to_enum<widget_var>( widget_string ) to convert a string to widget_var.
template<>
struct enum_traits<widget_var> {
    static constexpr widget_var last = widget_var::last;
};

// This is deliberately separate from "direction".
// The values correspond to the indexed directions returned from avatar::get_mon_visible
enum class cardinal_direction : int {
    NORTH = 0,
    NORTHEAST = 1,
    EAST = 2,
    SOUTHEAST = 3,
    SOUTH = 4,
    SOUTHWEST = 5,
    WEST = 6,
    NORTHWEST = 7,
    LOCAL = 8,
    num_cardinal_directions
};

// Use enum_traits for generic iteration over cardinal_direction, and string (de-)serialization.
// Use io::string_to_enum<cardinal_direction>( string ) to convert a string to cardinal_direction.
template<>
struct enum_traits<cardinal_direction> {
    static constexpr cardinal_direction last = cardinal_direction::num_cardinal_directions;
};

// Used when determining bodypart status indicators in sidebar widgets.
enum class bodypart_status : int {
    BITTEN,
    INFECTED,
    BROKEN,
    SPLINTED,
    BANDAGED,
    DISINFECTED,
    BLEEDING,
    num_bodypart_status
};

template<>
struct enum_traits<bodypart_status> {
    static constexpr bodypart_status last = bodypart_status::num_bodypart_status;
};

// Determines how text and labels are aligned for widgets
enum class widget_alignment : int {
    LEFT,
    CENTER,
    RIGHT,
    num_widget_alignments
};

template<>
struct enum_traits<widget_alignment> {
    static constexpr widget_alignment last = widget_alignment::num_widget_alignments;
};

// Use generic_factory for loading JSON data.
class JsonObject;
template<typename T>
class generic_factory;
class widget;

// Forward declaration, due to codependency on panels.h
class window_panel;

struct widget_clause {
    private:
        friend class widget;
        std::string id;
        std::string sym;
        translation text;
        nc_color color = c_unset;
        int value = INT_MIN;
        std::vector<widget_id> widgets;

        // Condition for using this clause
        bool has_condition = false;
        std::function<bool( const dialogue & )> condition;
        bool meets_condition( const std::string &opt_var = "" ) const;
        bool meets_condition( const std::set<bodypart_id> &bps ) const;

        static const widget_clause *get_clause_for_id( const std::string &clause_id, const widget_id &wgt,
                int thresh_val = INT_MIN, bool skip_condition = false );

    public:
        void load( const JsonObject &jo );

        /**
         * Static accessors:
         *
         * Returns the requested variable for the specified requirement, or a default value if
         * the requirements aren't met. Here are the default values for each field:
         *      id => ""
         *     sym => ""
         *    text => translation()
         *   color => c_white
         *   value => INT_MIN
         *
         * If multiple entries meet the requirement, you can specify a threshold value to get the
         * entry with the highest value below that threshold.
         *
         * If a clause also has a "condition" field, that condition must also return true in order
         * for that clause to be usable.
         */
        static int get_val_for_id( const std::string &clause_id,
                                   const widget_id &wgt, bool skip_condition = false );
        static const translation &get_text_for_id( const std::string &clause_id,
                const widget_id &wgt, bool skip_condition = false );
        static const std::string &get_sym_for_id( const std::string &clause_id,
                const widget_id &wgt, bool skip_condition = false );
        static nc_color get_color_for_id( const std::string &clause_id,
                                          const widget_id &wgt, bool skip_condition = false );
};

// A widget is a UI element displaying information from the underlying value of a widget_var.
// It may be loaded from a JSON object having "type": "widget".
class widget
{
    private:
        friend class generic_factory<widget>;
        friend struct mod_tracker;

        widget_id id;
        std::vector<std::pair<widget_id, mod_id>> src;
        bool was_loaded = false;
        const widget_clause *get_clause( const std::string &clause_id = "" ) const;
        std::vector<const widget_clause *> get_clauses() const;

    public:
        widget() = default;
        explicit widget( const widget_id &id ) : id( id ) {}

        // Attributes from JSON
        // ----
        // Display style to indicate the value: "numeric", "graph", "text"
        std::string _style;
        // Displayed label in the UI
        translation _label;
        // Description and help displayed in the UI
        std::string _description;
        // Width of the longest label within this layout's widgets (for "rows")
        int _label_width = 0;
        // Separator used to separate the label from the text. This is inherited from any parent widgets if none is found.
        std::string _separator;
        // Amount of padding to put between the label and text, as well as this widget and other widgets.
        int _padding;
        // Binding variable enum like stamina, bp_hp or stat_dex
        widget_var _var = widget_var::last;
        // Minimum meaningful var value, set by set_default_var_range
        int _var_min = INT_MIN;
        // Maximum meaningful var value, set by set_default_var_range
        int _var_max = INT_MAX;
        // True if this widget has an explicitly defined separator. False if it is inherited.
        bool explicit_separator;
        // True if this widget has an explicitly defined padding. False if it is inherited.
        bool explicit_padding;
        // Pad labels to match label length of child or sibling widgets
        bool _pad_labels;

        // Normal var range (low, high), set by set_default_var_range
        std::pair<int, int> _var_norm = std::make_pair( INT_MIN, INT_MAX );
        // Body part variable is linked to
        std::set<bodypart_id> _bps;
        // Width in characters of widget, not including label
        int _width = 0;
        // Height in characters of widget, only matters for style == widget
        int _height = 0;
        // Maximum height this widget can occupy (0 == no limit)
        int _height_max = 0;
        // String of symbols for graph widgets, mapped in increasing order like "0123..."
        std::string _symbols;
        // Graph fill style ("bucket" or "pool")
        std::string _fill;
        // String values mapped to numeric values or ranges
        translation _string;
        // Colors mapped to values or ranges
        std::vector<nc_color> _colors;
        // Optional color breaks in percent of the value's range; length = lenght(colors) - 1
        std::vector<int> _breaks;
        // Child widget ids for layout style
        std::vector<widget_id> _widgets;
        // Child widget layout arrangement / direction
        std::string _arrange;
        // Id of body_graph to use for widget_var::body_graph
        std::string _body_graph;
        // Compass direction corresponding to the indexed directions from avatar::get_mon_visible
        cardinal_direction _direction;
        // Flags for special widget behaviors
        std::set<flag_id> _flags;
        // clauses used to define text, colors and values
        std::vector<widget_clause> _clauses;
        // clause containing default values, in case none of _clauses have true conditions
        widget_clause _default_clause;
        // Alignment of the widget text (Default = RIGHT)
        widget_alignment _text_align;
        // Alignment of the widget label, if any (Default = LEFT)
        widget_alignment _label_align;

        // Load JSON data for a widget (uses generic factory widget_factory)
        static void load_widget( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string &src );
        // Finalize anything that must wait until all widgets are loaded
        static void finalize();
        // Recursively derive _label_width for nested layouts in this widget
        static int finalize_label_width_recursive( const widget_id &id );
        // Recursively derive _separator for nested layouts in this widget
        static void finalize_inherited_fields_recursive( const widget_id &id,
                const std::string &label_separator, int col_padding );
        // Reset to defaults using generic widget_factory
        static void reset();
        // Get all widget instances from the factory
        static const std::vector<widget> &get_all();
        // Get this widget's id
        const widget_id &getId() const;

        // Layout this widget within max_width, including child widgets. Calling layout on a regular
        // (non-layout style) widget is the same as show(), but will pad with spaces inside the
        // label area, so the returned string is equal to max_width.
        std::string layout( const avatar &ava, unsigned int max_width = 0, int label_width = 0,
                            bool skip_pad = false );
        // Display labeled widget, with value (number, graph, or string) from an avatar
        std::string show( const avatar &ava, unsigned int max_width );
        // Return a window_panel for rendering this widget at given width (and possibly height)
        window_panel get_window_panel( int width, int req_height = 1 );
        // Return a colorized string for a _var associated with a description function
        std::string color_text_function_string( const avatar &ava, unsigned int max_width );
        // Return true if the current _var is one which uses a description function
        bool uses_text_function() const;

        // Evaluate and return the bound "var" associated value for an avatar
        int get_var_value( const avatar &ava ) const;
        // True if this widget has the given flag. Used to specify certain behaviors.
        bool has_flag( const flag_id &flag ) const;
        bool has_flag( const std::string &flag ) const;
        // Assuming there's only one bodypart in _bps, return it (or bp_null if none exist)
        const bodypart_id &only_bp() const;

        // Set _var_min, _var_norm, and _var_max to values from the avatar
        void set_default_var_range( const avatar &ava );

        // Return a color-enhanced value_string
        std::string color_value_string( int value, int width_max = 0 );
        // Return a string for how a given value will render in the UI
        std::string value_string( int value, int width_max = 0 );
        // Return a suitable color for a given value
        nc_color value_color( int value );

        // Return a formatted numeric string
        std::string number( int value, bool from_condition ) const;
        // Return the numeric value(s) from all true conditional clauses in this widget
        std::string number_cond( enumeration_conjunction join_type = enumeration_conjunction::none ) const;
        // Return the text clause for "text" style
        std::string text( bool from_condition, int width = 0 );
        // Return the text clause(s) from all true conditional clauses in this widget
        std::string text_cond( bool no_join = false, int width = 0 );
        // Return the symbol mapped to a given value for "symbol" style
        std::string sym( bool from_condition );
        // Return the symbol(s) from all true conditional clauses in this widget
        std::string sym_cond( bool no_join = true,
                              enumeration_conjunction join_type = enumeration_conjunction::none ) const;
        // Return the text/symbols as list for this widget
        std::string sym_text( bool from_condition, int width = 0 );
        // Return the text/symbols from all true conditional clauses in this widget
        std::string sym_text_cond( bool no_join = true, int width = 0 );
        // Return the widgets clause for "layout" style
        std::vector<string_id<widget>> widgets( bool from_condition );
        // Return the widgets from all true conditional clauses in this widget
        std::vector<string_id<widget>> widgets_cond();
        // Return the graph part of this widget, rendered with "bucket" or "pool" fill
        std::string graph( int value ) const;
        // Takes a string generated by widget::layout and draws the text to the window w.
        // If the string contains newline characters, the text is broken up into lines.
        // Returns the new row index after drawing.
        // Note: Not intended to be called directly, only public for unit testing.
        static int custom_draw_multiline( const std::string &widget_string, const catacurses::window &w,
                                          int margin, int width, int row_num );
};

/************************************ Widget-adjacent functions ************************************/

// Split legend keys into X lines, where X = height.
// Lines use the provided width. This effectively limits the text to a 'width'x'height' box.
std::string format_widget_multiline( const std::vector<std::string> &keys, int max_height,
                                     int width, int &height, bool join = false );

#endif // CATA_SRC_WIDGET_H

