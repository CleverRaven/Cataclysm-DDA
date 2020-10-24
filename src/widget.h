#pragma once
#ifndef CATA_SRC_WIDGET_H
#define CATA_SRC_WIDGET_H

#include <string>
#include <vector>

#include "avatar.h"
//#include "cata_variant.h"
#include "enum_traits.h"
#include "string_id.h"
#include "type_id.h"

class JsonObject;
template<typename T>
class generic_factory;

// Supported data variables for widgets, as enum
// widget_var names may be given as the "var" field in widget JSON
// Use io::string_to_enum<widget_var>( widget_string ) to convert strings to these
enum class widget_var : int {
    focus,
    hunger,
    move,
    mood,
    pain,
    sound,
    speed,
    stamina,
    thirst,
    fatigue,
    weariness,
    mana,
    stat_str,
    stat_dex,
    stat_int,
    stat_per,
    bp_hp,
    bp_encumb,
    bp_warmth,
    pain_description,
    hunger_description,
    thirst_description,
    fatigue_description,
    last
};
template<>
struct enum_traits<widget_var> {
    static constexpr widget_var last = widget_var::last;
};

// UI widget with labeled numeric, graph, or phrase display of underlying variables.
//
class widget
{
    private:
        friend class generic_factory<widget>;

        widget_id id;
        bool was_loaded = false;

    public:
        widget() = default;
        widget( const widget_id &id ) : id( id ) {}

        // Attributes from JSON
        // ----
        // Display style to indicate the value: "numeric", "graph", "phrases"
        std::string _style;
        // Displayed label in the UI
        std::string _label;
        // Binding variable enum like stamina, bp_hp or stat_dex
        widget_var _var;
        // Minimum var value, optional
        int _var_min = 0;
        // Maximum var value, required for graph widgets
        int _var_max = 10;
        // Body part variable is linked to
        bodypart_id _bp_id;
        // Width in characters of widget, not including label
        int _width = 0;
        // String of symbols for graph widgets, mapped in increasing order like "0123..."
        std::string _symbols;
        // Graph fill style ("bucket" or "pool")
        std::string _fill;
        // String values mapped to numeric values or ranges
        std::vector<std::string> _strings;
        // Colors mapped to values or ranges
        std::vector<nc_color> _colors;
        // Child widget ids for layout style
        std::vector<widget_id> _widgets;
        // Child widget layout arrangement / direction
        std::string _arrange;

        // Load JSON data for a widget (uses generic factory)
        static void load_widget( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string &src );
        static void reset();

        // Layout widget within max_width, including child widgets. Calling layout on a regular
        // (non-layout style) widget is the same as show(), but will pad with spaces inside the
        // label area, so the returned string is equal to max_width.
        std::string layout( const avatar &ava, unsigned int max_width = 0 );
        // Display labeled widget, with value (number, graph, or string) from an avatar
        std::string show( const avatar &ava );
        // Return a colorized string for a legacy sidebar formatting function
        std::string color_legacyfunc_string( const avatar &ava );
        bool uses_legacyfunc();

        // Evaluate and return the bound "var" associated value for an avatar
        int get_var_value( const avatar &ava );
        // Return the maximum "var" value from "var_max", or max for avatar (HP, mana, etc.)
        int get_var_max( const avatar &ava );

        // Return a color-enhanced value_string
        std::string color_value_string( int value, int value_max = 0 );
        // Return a string for how a given value will render in the UI
        std::string value_string( int value, int value_max = 0 );
        // Return a suitable color for a given value
        nc_color value_color( int value, int value_max = 0 );

        // Return a formatted numeric string
        std::string num( int value, int value_max = 0 );
        // Return the phrase mapped to a given value for a "phrase" style
        std::string phrase( int value, int value_max = 0 );
        // Return the graph part of this widget, rendered with "bucket" or "pool" fill
        std::string graph( int value, int value_max = 0 );

};

#endif // CATA_SRC_WIDGET_H
