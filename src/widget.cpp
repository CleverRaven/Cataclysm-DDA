#include "widget.h"

#include "color.h"
#include "generic_factory.h"
#include "json.h"
#include "panels.h"

// Use generic factory wrappers for widgets to use standardized JSON loading methods
namespace
{
generic_factory<widget> widget_factory( "widgets" );
} // namespace

template<>
const widget &string_id<widget>::obj() const
{
    return widget_factory.obj( *this );
}

template<>
bool string_id<widget>::is_valid() const
{
    return widget_factory.is_valid( *this );
}

void widget::load_widget( const JsonObject &jo, const std::string &src )
{
    widget_factory.load( jo, src );
}

void widget::reset()
{
    widget_factory.reset();
}

// Convert widget "var" enums to string equivalents
namespace io
{
template<>
std::string enum_to_string<widget_var>( widget_var data )
{
    switch( data ) {
        case widget_var::focus:
            return "focus";
        case widget_var::hunger:
            return "hunger";
        case widget_var::move:
            return "move";
        case widget_var::mood:
            return "mood";
        case widget_var::pain:
            return "pain";
        case widget_var::sound:
            return "sound";
        case widget_var::speed:
            return "speed";
        case widget_var::stamina:
            return "stamina";
        case widget_var::thirst:
            return "thirst";
        case widget_var::fatigue:
            return "fatigue";
        case widget_var::weariness_level:
            return "weariness_level";
        case widget_var::mana:
            return "mana";
        case widget_var::morale_level:
            return "morale_level";
        // Base stats
        case widget_var::stat_str:
            return "stat_str";
        case widget_var::stat_dex:
            return "stat_dex";
        case widget_var::stat_int:
            return "stat_int";
        case widget_var::stat_per:
            return "stat_per";
        // Bodypart attributes
        case widget_var::bp_hp:
            return "bp_hp";
        case widget_var::bp_encumb:
            return "bp_encumb";
        case widget_var::bp_warmth:
            return "bp_warmth";
        case widget_var::bp_wetness:
            return "bp_wetness";
        // Description functions
        case widget_var::pain_text:
            return "pain_text";
        case widget_var::hunger_text:
            return "hunger_text";
        case widget_var::thirst_text:
            return "thirst_text";
        case widget_var::fatigue_text:
            return "fatigue_text";
        case widget_var::weight_text:
            return "weight_text";
        // Fall-through - invalid
        case widget_var::last:
            break;
    }
    debugmsg( "Invalid widget_var" );
    abort();
}

} // namespace io

void widget::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "strings", _strings );
    optional( jo, was_loaded, "width", _width, 1 );
    optional( jo, was_loaded, "symbols", _symbols, "-" );
    optional( jo, was_loaded, "fill", _fill, "bucket" );
    optional( jo, was_loaded, "label", _label, translation() );
    optional( jo, was_loaded, "style", _style, "number" );
    optional( jo, was_loaded, "arrange", _arrange, "columns" );
    optional( jo, was_loaded, "var_min", _var_min );
    optional( jo, was_loaded, "var_max", _var_max );

    if( jo.has_string( "var" ) ) {
        _var = io::string_to_enum<widget_var>( jo.get_string( "var" ) );
    }

    if( jo.has_string( "bodypart" ) ) {
        _bp_id = bodypart_id( jo.get_string( "bodypart" ) );
    }

    if( jo.has_array( "colors" ) ) {
        _colors.clear();
        for( const std::string color_name : jo.get_array( "colors" ) ) {
            _colors.emplace_back( get_all_colors().name_to_color( color_name ) );
        }
    }
    if( jo.has_array( "widgets" ) ) {
        _widgets.clear();
        for( const std::string wid : jo.get_array( "widgets" ) ) {
            _widgets.emplace_back( widget_id( wid ) );
        }
    }
}

int widget::get_var_max( const avatar &ava )
{
    // Some vars (like HP) have an inherent maximum, used unless the widget overrides it
    int max_val = 1;
    // max_val (used only for graphs) is set to a known maximum if the attribute has one; otherwise,
    // it is up to the graph widget to set "var_max" so the graph widget can determine a scaling.
    switch( _var ) {
        case widget_var::stamina:
            max_val = ava.get_stamina_max();
            break;
        case widget_var::mana:
            max_val = ava.magic->max_mana( ava );
            break;
        case widget_var::morale_level:
            // TODO: Determine actual max
            max_val = 100;
            break;
        case widget_var::bp_hp:
            // HP for body part
            max_val = ava.get_part_hp_max( _bp_id );
            break;
        case widget_var::bp_warmth:
            // From weather.h: Body temperature is measured on a scale of 0u to 10000u,
            // where 10u = 0.02C and 5000u is 37C
            max_val = 10000;
            break;
        case widget_var::bp_wetness:
            max_val = 100; // ???
            break;
        default:
            break;
    }
    // JSON-defined var_max may override it
    if( _var_max > 0 ) {
        max_val = _var_max;
    }
    return max_val;
}

int widget::get_var_value( const avatar &ava )
{
    // Numeric value to be rendered in the widget
    int value = 0;

    // Each "var" value refers to some attribute, typically of the avatar, that yields a numeric
    // value, and can be displayed as a numeric field, a graph, or a series of text phrases.
    switch( _var ) {
        // Vars with a known max val
        case widget_var::stamina:
            value = ava.get_stamina();
            break;
        case widget_var::mana:
            value = ava.magic->available_mana();
            break;
        case widget_var::morale_level:
            value = ava.get_morale_level();
            break;
        case widget_var::bp_hp:
            // HP for body part
            value = ava.get_part_hp_cur( _bp_id );
            break;
        case widget_var::bp_warmth:
            // Body part warmth/temperature
            value = ava.get_part_temp_cur( _bp_id );
            break;
        case widget_var::bp_wetness:
            // Body part wetness
            value = ava.get_part_wetness( _bp_id );
            break;
        case widget_var::focus:
            value = ava.get_focus();
            break;
        case widget_var::speed:
            value = ava.get_speed();
            break;
        case widget_var::move:
            value = ava.movecounter;
            break;
        case widget_var::pain:
            value = ava.get_perceived_pain();
            break;
        case widget_var::fatigue:
            value = ava.get_fatigue();
            break;
        case widget_var::weariness_level:
            value = ava.weariness_level();
            break;
        case widget_var::stat_str:
            value = ava.get_str();
            break;
        case widget_var::stat_dex:
            value = ava.get_dex();
            break;
        case widget_var::stat_int:
            value = ava.get_int();
            break;
        case widget_var::stat_per:
            value = ava.get_per();
            break;
        case widget_var::sound:
            value = ava.volume;
            break;
        case widget_var::bp_encumb:
            // Encumbrance for body part
            value = ava.get_part_encumbrance_data( _bp_id ).encumbrance;
            break;

        // TODO
        case widget_var::mood:
        // see morale_emotion
        case widget_var::hunger:
        // see display::hunger_text_color()
        case widget_var::thirst:
        // see display::thirst_text_color()
        default:
            value = 0;
    }
    return value;
}

std::string widget::show( const avatar &ava )
{
    if( uses_text_function() ) {
        // Text functions are a carry-over from before widgets, with existing functions generating
        // descriptive colorized text for avatar attributes.  The "value" for these is immaterial;
        // only the final color string is shown.  Bypass value calculation and call the
        // text-rendering function directly.
        return color_text_function_string( ava );
    } else {
        // For normal widgets, get current numeric value and potential maximum,
        // and return a color string rendering of that value in the appropriate style.
        int value = get_var_value( ava );
        int value_max = get_var_max( ava );
        return color_value_string( value, value_max );
    }
}

bool widget::uses_text_function()
{
    switch( _var ) {
        case widget_var::pain_text:
        case widget_var::hunger_text:
        case widget_var::thirst_text:
        case widget_var::fatigue_text:
        case widget_var::weight_text:
            return true;
        default:
            return false;
    }
}

std::string widget::color_text_function_string( const avatar &ava )
{
    std::string ret;
    std::pair<std::string, nc_color> desc;
    switch( _var ) {
        case widget_var::pain_text:
            desc = display::pain_text_color( ava );
            break;
        case widget_var::hunger_text:
            desc = display::hunger_text_color( ava );
            break;
        case widget_var::thirst_text:
            desc = display::thirst_text_color( ava );
            break;
        case widget_var::fatigue_text:
            desc = display::fatigue_text_color( ava );
            break;
        case widget_var::weight_text:
            desc = display::weight_text_color( ava );
            break;
        default:
            debugmsg( "Unexpected widget_var %s - no text_color function defined",
                      io::enum_to_string<widget_var>( _var ) );
            return "???";
    }
    ret += colorize( desc.first, desc.second );
    return ret;
}

std::string widget::color_value_string( int value, int value_max )
{
    if( value_max == 0 ) {
        value_max = _var_max;
    }
    std::string val_string = value_string( value, value_max );
    const nc_color cur_color = value_color( value, value_max );
    if( cur_color == c_unset ) {
        return val_string;
    } else {
        return colorize( val_string, cur_color );
    }
}

std::string widget::value_string( int value, int value_max )
{
    std::string ret;
    if( _style == "graph" ) {
        ret += graph( value, value_max );
    } else if( _style == "text" ) {
        ret += text( value, value_max );
    } else if( _style == "number" ) {
        ret += number( value, value_max );
    } else {
        ret += "???";
    }
    return ret;
}

nc_color widget::value_color( int value, int value_max )
{
    if( _colors.empty() ) {
        return c_unset;
    }
    // Scale to value_max
    if( value_max > 0 ) {
        if( value <= value_max ) {
            // Scale value range from [0, 1] to map color range
            const double scale = static_cast<double>( value ) / value_max;
            const int color_max = _colors.size() - 1;
            // Include 0.5f offset to make up for floor piling values up at the bottom
            const int color_index = std::floor( scale * color_max + 0.5f );
            return _colors[color_index];
        } else {
            return _colors.back();
        }
    }
    // Assume colors map to 0, 1, 2 ...
    if( value < num_colors ) {
        return _colors[value];
    }
    // Last color as last resort
    return _colors.back();
}

std::string widget::number( int value, int /* value_max */ )
{
    return string_format( "%d", value );
}

std::string widget::text( int value, int /* value_max */ )
{
    return _strings.at( value ).translated();
}

std::string widget::graph( int value, int value_max )
{
    // graph "depth is equal to the number of nonzero symbols
    int depth = _symbols.length() - 1;
    // Max integer value this graph can show
    int max_graph_val = _width * depth;
    // Scale value range to current graph resolution (width x depth)
    if( value_max > 0 && value_max != max_graph_val ) {
        // Scale max source value to max graph value
        value = max_graph_val * value / value_max;
    }

    // Negative values are not (yet) supported
    if( value < 0 ) {
        value = 0;
    }
    // Truncate to maximum value displayable by graph
    if( value > max_graph_val ) {
        value = max_graph_val;
    }

    int quot;
    int rem;

    std::string ret;
    if( _fill == "bucket" ) {
        quot = value / depth; // number of full cells/buckets
        rem = value % depth;  // partly full next cell, maybe
        // Full cells at the front
        ret += std::string( quot, _symbols.back() );
        // Any partly-full cells?
        if( _width > quot ) {
            // Current partly-full cell
            ret += _symbols[rem];
            // Any more zero cells at the end
            if( _width > quot + 1 ) {
                ret += std::string( _width - quot - 1, _symbols[0] );
            }
        }
    } else if( _fill == "pool" ) {
        quot = value / _width; // baseline depth of the pool
        rem = value % _width;  // number of cells at next depth
        // Most-filled cells come first
        if( rem > 0 ) {
            ret += std::string( rem, _symbols[quot + 1] );
            // Less-filled cells may follow
            if( _width > rem ) {
                ret += std::string( _width - rem, _symbols[quot] );
            }
        } else {
            // All cells at the same level
            ret += std::string( _width, _symbols[quot] );
        }
    } else {
        debugmsg( "Unknown widget fill type %s", _fill );
        return ret;
    }
    return ret;
}

std::string widget::layout( const avatar &ava, const unsigned int max_width )
{
    std::string ret;
    if( _style == "layout" ) {
        // Divide max_width equally among all widgets
        int child_width = max_width / _widgets.size();
        int remainder = max_width % _widgets.size();
        for( const widget_id &wid : _widgets ) {
            widget cur_child = wid.obj();
            int cur_width = child_width;
            // Spread remainder over the first few columns
            if( remainder > 0 ) {
                cur_width += 1;
                remainder -= 1;
            }
            // Allow 2 spaces of padding after each column, except last column (full-justified)
            if( wid != _widgets.back() ) {
                ret += string_format( "%s  ", cur_child.layout( ava, cur_width - 2 ) );
            } else {
                ret += string_format( "%s", cur_child.layout( ava, cur_width ) );
            }
        }
    } else {
        // Get displayed value (colorized)
        std::string shown = show( ava );
        const std::string tlabel = _label.translated();
        // Width used by label, ": " and value, using utf8_width to ignore color tags
        unsigned int used_width = utf8_width( tlabel, true ) + 2 + utf8_width( shown, true );

        // Label and ": " first
        ret += tlabel + ": ";
        // then enough padding to fit max_width
        if( used_width < max_width ) {
            ret += std::string( max_width - used_width, ' ' );
        }
        // then colorized value
        ret += shown;
    }
    return ret;
}

