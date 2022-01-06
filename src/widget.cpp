#include "widget.h"

#include "character_martial_arts.h"
#include "color.h"
#include "generic_factory.h"
#include "json.h"
#include "output.h"
#include "overmapbuffer.h"
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

const std::vector<widget> &widget::get_all()
{
    return widget_factory.get_all();
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
        case widget_var::move_cost:
            return "move_cost";
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
        case widget_var::health:
            return "health";
        case widget_var::weariness_level:
            return "weariness_level";
        case widget_var::mana:
            return "mana";
        case widget_var::morale_level:
            return "morale_level";
        // Compass
        case widget_var::compass_text:
            return "compass_text";
        case widget_var::compass_legend_text:
            return "compass_legend_text";
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
        case widget_var::cardio_fit:
            return "cardio_fit";
        case widget_var::cardio_acc:
            return "cardio_acc";
        // Description functions
        case widget_var::activity_text:
            return "activity_text";
        case widget_var::body_temp_text:
            return "body_temp_text";
        case widget_var::bp_status_text:
            return "bp_status_text";
        case widget_var::date_text:
            return "date_text";
        case widget_var::env_temp_text:
            return "env_temp_text";
        case widget_var::fatigue_text:
            return "fatigue_text";
        case widget_var::health_text:
            return "health_text";
        case widget_var::hunger_text:
            return "hunger_text";
        case widget_var::lighting_text:
            return "lighting_text";
        case widget_var::mood_text:
            return "mood_text";
        case widget_var::moon_phase_text:
            return "moon_phase_text";
        case widget_var::move_mode_letter:
            return "move_mode_letter";
        case widget_var::move_mode_text:
            return "move_mode_text";
        case widget_var::pain_text:
            return "pain_text";
        case widget_var::place_text:
            return "place_text";
        case widget_var::power_text:
            return "power_text";
        case widget_var::rad_badge_text:
            return "rad_badge_text";
        case widget_var::safe_mode_text:
            return "safe_mode_text";
        case widget_var::style_text:
            return "style_text";
        case widget_var::thirst_text:
            return "thirst_text";
        case widget_var::time_text:
            return "time_text";
        case widget_var::weather_text:
            return "weather_text";
        case widget_var::weariness_text:
            return "weariness_text";
        case widget_var::weary_malus_text:
            return "weary_malus_text";
        case widget_var::weight_text:
            return "weight_text";
        case widget_var::wielding_text:
            return "wielding_text";
        case widget_var::wind_text:
            return "wind_text";
        // Fall-through - invalid
        case widget_var::last:
            break;
    }
    cata_fatal( "Invalid widget_var" );
}

template<>
std::string enum_to_string<cardinal_direction>( cardinal_direction dir )
{
    switch( dir ) {
        case cardinal_direction::NORTH:
            return "N";
        case cardinal_direction::SOUTH:
            return "S";
        case cardinal_direction::EAST:
            return "E";
        case cardinal_direction::WEST:
            return "W";
        case cardinal_direction::NORTHEAST:
            return "NE";
        case cardinal_direction::NORTHWEST:
            return "NW";
        case cardinal_direction::SOUTHEAST:
            return "SE";
        case cardinal_direction::SOUTHWEST:
            return "SW";
        case cardinal_direction::LOCAL:
            return "L";
        case cardinal_direction::num_cardinal_directions:
        default:
            break;
    }
    cata_fatal( "Invalid cardinal_direction" );
}
} // namespace io

void widget::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "strings", _strings );
    optional( jo, was_loaded, "width", _width, 1 );
    optional( jo, was_loaded, "height", _height, 1 );
    optional( jo, was_loaded, "symbols", _symbols, "-" );
    optional( jo, was_loaded, "fill", _fill, "bucket" );
    optional( jo, was_loaded, "label", _label, translation() );
    optional( jo, was_loaded, "style", _style, "number" );
    optional( jo, was_loaded, "arrange", _arrange, "columns" );
    optional( jo, was_loaded, "var_min", _var_min );
    optional( jo, was_loaded, "var_max", _var_max );
    optional( jo, was_loaded, "direction", _direction, cardinal_direction::num_cardinal_directions );

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

void widget::finalize()
{
    // Nothing to do?
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
        case widget_var::move_cost:
            value = ava.run_cost( 100 );
            break;
        case widget_var::pain:
            value = ava.get_perceived_pain();
            break;
        case widget_var::fatigue:
            value = ava.get_fatigue();
            break;
        case widget_var::health:
            value = ava.get_healthy();
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
        case widget_var::cardio_fit:
            value = ava.get_cardiofit();
            break;
        case widget_var::cardio_acc:
            value = ava.get_cardio_acc();
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

std::string widget::show( const avatar &ava, const unsigned int max_width )
{
    if( uses_text_function() ) {
        // Text functions are a carry-over from before widgets, with existing functions generating
        // descriptive colorized text for avatar attributes.  The "value" for these is immaterial;
        // only the final color string is shown.  Bypass value calculation and call the
        // text-rendering function directly.
        return color_text_function_string( ava, max_width );
    } else {
        // For normal widgets, get current numeric value and potential maximum,
        // and return a color string rendering of that value in the appropriate style.
        int value = get_var_value( ava );
        int value_max = get_var_max( ava );
        return color_value_string( value, value_max );
    }
}

// Returns the new row index after drawing
int widget::custom_draw_multiline( const std::string &widget_string, const catacurses::window &w,
                                   const int margin, const int width, int row_num )
{
    std::string wgt_str = widget_string;
    size_t strpos = 0;
    // Split the widget string into lines (for height > 1)
    while( ( strpos = wgt_str.find( '\n' ) ) != std::string::npos ) {
        trim_and_print( w, point( margin, row_num ), width, c_light_gray, wgt_str.substr( 0, strpos ) );
        wgt_str.erase( 0, strpos + 1 );
        row_num++;
    }
    // Last line (or first line for single-line widgets)
    trim_and_print( w, point( margin, row_num ), width, c_light_gray, wgt_str );
    row_num++;
    return row_num;
}

// Drawing function, provided as a callback to the window_panel constructor.
// Handles rendering a widget's content into a window panel.
static void custom_draw_func( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;
    widget *wgt = args.get_widget();

    // Get full window width
    const int width = catacurses::getmaxx( w );
    // Leave 1 character space for margin on left and right
    const int margin = 1;
    const int widt = width - 2 * margin;

    // Quit if there is nothing to draw or no space to draw it
    if( wgt == nullptr || width <= 0 ) {
        return;
    }

    werase( w );
    if( wgt->_style == "sidebar" ) {
    } else if( wgt->_style == "layout" ) {
        if( wgt->_arrange == "rows" ) {
            // Layout widgets in rows
            // FIXME: Be able to handle rows that are themselves more than one line!
            // Could this be done in the layout() function somehow (by returning newlines?)
            int row_num = 0;
            for( const widget_id &row_wid : wgt->_widgets ) {
                widget row_widget = row_wid.obj();
                const std::string txt = row_widget.layout( u, widt );
                row_num = widget::custom_draw_multiline( txt, w, margin, widt, row_num );
            }
        } else {
            // Layout widgets in columns
            // For now, this is the default when calling layout()
            // So, just layout self on a single line
            widget::custom_draw_multiline( wgt->layout( u, widt ), w, margin, widt, 0 );
        }
    } else {
        // No layout, just a widget
        widget::custom_draw_multiline( wgt->layout( u, widt ), w, margin, widt, 0 );
    }
    wnoutrefresh( w );
}

window_panel widget::get_window_panel( const int width, const int req_height )
{
    // Width is fixed, but height may vary depending on child widgets
    int height = req_height;

    // For layout with rows, height will be the combined
    // height of all child widgets.
    if( _style == "layout" && _arrange == "rows" ) {
        height = 0;
        for( const widget_id &wid : _widgets ) {
            height += wid->_height > 0 ? wid->_height : 1;
        }
    } else if( _style == "widget" ) {
        height = _height > 1 ? _height : req_height;
    }
    // Minimap and log do not have a predetermined height
    // (or they should allow caller to customize height)

    window_panel win( custom_draw_func, _label.translated(), _label, height, width, true );
    win.set_widget( this->id );
    return win;
}

bool widget::uses_text_function()
{
    switch( _var ) {
        case widget_var::activity_text:
        case widget_var::body_temp_text:
        case widget_var::bp_status_text:
        case widget_var::compass_text:
        case widget_var::compass_legend_text:
        case widget_var::date_text:
        case widget_var::env_temp_text:
        case widget_var::fatigue_text:
        case widget_var::health_text:
        case widget_var::hunger_text:
        case widget_var::lighting_text:
        case widget_var::mood_text:
        case widget_var::moon_phase_text:
        case widget_var::move_mode_letter:
        case widget_var::move_mode_text:
        case widget_var::pain_text:
        case widget_var::place_text:
        case widget_var::power_text:
        case widget_var::rad_badge_text:
        case widget_var::safe_mode_text:
        case widget_var::style_text:
        case widget_var::thirst_text:
        case widget_var::time_text:
        case widget_var::weather_text:
        case widget_var::weariness_text:
        case widget_var::weary_malus_text:
        case widget_var::weight_text:
        case widget_var::wielding_text:
        case widget_var::wind_text:
            return true;
        default:
            return false;
    }
}

// NOTE: Use max_width to split multi-line widgets across lines
std::string widget::color_text_function_string( const avatar &ava, unsigned int max_width )
{
    std::string ret;
    bool apply_color = true;
    std::pair<std::string, nc_color> desc;
    // Give a default color (some widget_vars do not define one)
    desc.second = c_light_gray;
    switch( _var ) {
        case widget_var::activity_text:
            desc = display::activity_text_color( ava );
            break;
        case widget_var::body_temp_text:
            desc = display::temp_text_color( ava );
            break;
        case widget_var::bp_status_text:
            desc.first = display::colorized_bodypart_status_text( ava, _bp_id );
            apply_color = false; // Has embedded color already
            break;
        case widget_var::date_text:
            desc.first = display::date_string();
            break;
        case widget_var::env_temp_text:
            desc.first = display::get_temp( ava );
            break;
        case widget_var::fatigue_text:
            desc = display::fatigue_text_color( ava );
            break;
        case widget_var::health_text:
            desc = display::health_text_color( ava );
            break;
        case widget_var::hunger_text:
            desc = display::hunger_text_color( ava );
            break;
        case widget_var::lighting_text:
            desc = get_light_level( ava.fine_detail_vision_mod() );
            break;
        case widget_var::mood_text:
            desc = display::morale_face_color( ava );
            break;
        case widget_var::moon_phase_text:
            desc.first = display::get_moon();
            break;
        case widget_var::move_mode_letter:
            desc = display::move_mode_letter_color( ava );
            break;
        case widget_var::move_mode_text:
            desc = display::move_mode_text_color( ava );
            break;
        case widget_var::pain_text:
            desc = display::pain_text_color( ava );
            break;
        case widget_var::place_text:
            desc.first = overmap_buffer.ter( ava.global_omt_location() )->get_name();
            break;
        case widget_var::power_text:
            desc = display::power_text_color( ava );
            break;
        case widget_var::rad_badge_text:
            desc = display::rad_badge_text_color( ava );
            break;
        case widget_var::safe_mode_text:
            desc = display::safe_mode_text_color( false );
            break;
        case widget_var::style_text:
            desc.first = ava.martial_arts_data->selected_style_name( ava );
            break;
        case widget_var::thirst_text:
            desc = display::thirst_text_color( ava );
            break;
        case widget_var::time_text:
            desc.first = display::time_string( ava );
            break;
        case widget_var::weather_text:
            desc = display::weather_text_color( ava );
            break;
        case widget_var::weariness_text:
            desc = display::weariness_text_color( ava );
            break;
        case widget_var::weary_malus_text:
            desc = display::weary_malus_text_color( ava );
            break;
        case widget_var::weight_text:
            desc = display::weight_text_color( ava );
            break;
        case widget_var::wielding_text:
            desc.first = ava.weapname();
            break;
        case widget_var::wind_text:
            desc = display::wind_text_color( ava );
            break;
        case widget_var::compass_text:
            desc.first = display::colorized_compass_text( _direction, _width );
            apply_color = false; // Already colorized
            break;
        case widget_var::compass_legend_text:
            desc.first = display::colorized_compass_legend_text( max_width, _height );
            apply_color = false; // Already colorized
            break;
        default:
            debugmsg( "Unexpected widget_var %s - no text_color function defined",
                      io::enum_to_string<widget_var>( _var ) );
            return _( "???" );
    }
    ret += apply_color ? colorize( desc.first, desc.second ) : desc.first;
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

// For widget::layout, process each row to append to the layout string
static std::string append_line( const std::string &line, bool first_row, unsigned int max_width,
                                const translation &label, bool skip_padding )
{
    std::string ret;
    // Width used by label, ": " and value, using utf8_width to ignore color tags
    unsigned int used_width = utf8_width( line, true );
    if( first_row ) {
        const std::string tlabel = label.translated();
        // If label is empty or omitted, don't reserve space for it
        if( !tlabel.empty() ) {
            used_width += utf8_width( tlabel, true ) + 2;
            // Label and ": " first
            ret += tlabel + ": ";
        }
    }

    // then enough padding to fit max_width
    if( !skip_padding && used_width < max_width ) {
        ret += std::string( max_width - used_width, ' ' );
    }
    // then colorized value
    ret += line;
    return ret;
}

std::string widget::layout( const avatar &ava, const unsigned int max_width )
{
    std::string ret;
    if( _style == "layout" ) {
        // Widgets with "rows" arrangement must be laid out from window_panel
        if( _arrange == "rows" ) {
            debugmsg( "widget layout called with rows" );
        }
        const int num_widgets = _widgets.size();
        if( num_widgets == 0 ) {
            debugmsg( "widget layout has no widgets" );
        }
        // Number of spaces between columns
        const int col_padding = 2;
        // Subtract column padding to get space available for widgets
        const int avail_width = max_width - col_padding * ( num_widgets - 1 );
        // Divide available width equally among all widgets
        const int child_width = avail_width / num_widgets;
        // Keep remainder to distribute
        int remainder = avail_width % num_widgets;
        for( const widget_id &wid : _widgets ) {
            widget cur_child = wid.obj();
            int cur_width = child_width;
            // Spread remainder over the first few columns
            if( remainder > 0 ) {
                cur_width += 1;
                remainder -= 1;
            }
            // Layout child in this column
            ret += string_format( "%s", cur_child.layout( ava, cur_width ) );
            // Add column padding until we reach the last column
            if( wid != _widgets.back() ) {
                ret += std::string( col_padding, ' ' );
            }
        }
    } else {
        // Get displayed value (colorized)
        std::string shown = show( ava, max_width );
        size_t strpos = 0;
        int row_num = 0;
        // For multi-line widgets, each line is separated by a '\n' character
        while( ( strpos = shown.find( '\n' ) ) != std::string::npos && row_num < _height ) {
            // Process line, including '\n'
            ret += append_line( shown.substr( 0, strpos + 1 ), row_num == 0, max_width, _label, _height > 1 );
            // Delete used token
            shown.erase( 0, strpos + 1 );
            row_num++;
        }
        if( row_num < _height ) {
            // Process last line, or first for single-line widgets
            ret += append_line( shown, row_num == 0, max_width, _label, _height > 1 );
        }
    }
    return ret;
}

