#include "widget.h"

#include "character_martial_arts.h"
#include "color.h"
#include "condition.h"
#include "display.h"
#include "generic_factory.h"
#include "json.h"
#include "output.h"
#include "overmapbuffer.h"
#include "npctalk.h"

const static flag_id json_flag_W_DISABLED_BY_DEFAULT( "W_DISABLED_BY_DEFAULT" );
const static flag_id json_flag_W_DISABLED_WHEN_EMPTY( "W_DISABLED_WHEN_EMPTY" );
const static flag_id json_flag_W_DYNAMIC_HEIGHT( "W_DYNAMIC_HEIGHT" );
const static flag_id json_flag_W_LABEL_NONE( "W_LABEL_NONE" );
const static flag_id json_flag_W_NO_PADDING( "W_NO_PADDING" );

// Default label separator for widgets.
const static std::string default_separator = ": ";

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

const widget_id &widget::getId() const
{
    return id;
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
        case widget_var::move:
            return "move";
        case widget_var::move_remainder:
            return "move_remainder";
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
        case widget_var::fatigue:
            return "fatigue";
        case widget_var::health:
            return "health";
        case widget_var::weariness_level:
            return "weariness_level";
        case widget_var::mana:
            return "mana";
        case widget_var::max_mana:
            return "max_mana";
        case widget_var::power_percentage:
            return "power_percentage";
        case widget_var::log_power_balance:
            return "log_power_balance";
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
        case widget_var::carry_weight:
            return "carry_weight";
        // Description functions
        case widget_var::activity_text:
            return "activity_text";
        case widget_var::body_graph:
            return "body_graph";
        case widget_var::body_graph_temp:
            return "body_graph_temp";
        case widget_var::body_graph_encumb:
            return "body_graph_encumb";
        case widget_var::body_graph_status:
            return "body_graph_status";
        case widget_var::body_graph_wet:
            return "body_graph_wet";
        case widget_var::bp_armor_outer_text:
            return "bp_armor_outer_text";
        case widget_var::carry_weight_text:
            return "carry_weight_text";
        case widget_var::date_text:
            return "date_text";
        case widget_var::env_temp_text:
            return "env_temp_text";
        case widget_var::mood_text:
            return "mood_text";
        case widget_var::move_count_mode_text:
            return "move_count_mode_text";
        case widget_var::pain_text:
            return "pain_text";
        case widget_var::overmap_loc_text:
            return "overmap_loc_text";
        case widget_var::overmap_text:
            return "overmap_text";
        case widget_var::place_text:
            return "place_text";
        case widget_var::power_text:
            return "power_text";
        case widget_var::power_balance_text:
            return "power_balance_text";
        case widget_var::safe_mode_text:
            return "safe_mode_text";
        case widget_var::safe_mode_classic_text:
            return "safe_mode_classic_text";
        case widget_var::style_text:
            return "style_text";
        case widget_var::sundial_text:
            return "sundial_text";
        case widget_var::sundial_time_text:
            return "sundial_time_text";
        case widget_var::time_text:
            return "time_text";
        case widget_var::veh_azimuth_text:
            return "veh_azimuth_text";
        case widget_var::veh_cruise_text:
            return "veh_cruise_text";
        case widget_var::veh_fuel_text:
            return "veh_fuel_text";
        case widget_var::weariness_text:
            return "weariness_text";
        case widget_var::weary_transition_level:
            return "weary_transition_level";
        case widget_var::weary_malus_text:
            return "weary_malus_text";
        case widget_var::weather_text:
            return "weather_text";
        case widget_var::wielding_text:
            return "wielding_text";
        case widget_var::wielding_simple_text:
            return "wielding_simple_text";
        case widget_var::wielding_mode_text:
            return "wielding_mode_text";
        case widget_var::wielding_ammo_text:
            return "wielding_ammo_text";
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

template<>
std::string enum_to_string<bodypart_status>( bodypart_status stat )
{
    switch( stat ) {
        case bodypart_status::BITTEN:
            return "bitten";
        case bodypart_status::INFECTED:
            return "infected";
        case bodypart_status::BROKEN:
            return "broken";
        case bodypart_status::SPLINTED:
            return "splinted";
        case bodypart_status::BANDAGED:
            return "bandaged";
        case bodypart_status::DISINFECTED:
            return "disinfected";
        case bodypart_status::BLEEDING:
            return "bleeding";
        case bodypart_status::num_bodypart_status:
        default:
            break;
    }
    cata_fatal( "Invalid bodypart_status" );
}

template<>
std::string enum_to_string<widget_alignment>( widget_alignment align )
{
    switch( align ) {
        case widget_alignment::LEFT:
            return "left";
        case widget_alignment::CENTER:
            return "center";
        case widget_alignment::RIGHT:
            return "right";
        case widget_alignment::num_widget_alignments:
        default:
            break;
    }
    cata_fatal( "Invalid widget_alignment" );
}
} // namespace io

void widget_clause::load( const JsonObject &jo )
{
    optional( jo, false, "id", id );
    optional( jo, false, "text", text );
    optional( jo, false, "sym", sym );
    optional( jo, false, "value", value, INT_MIN );

    std::string clr;
    optional( jo, false, "color", clr, "" );
    color = color_from_string( clr );

    if( jo.has_member( "condition" ) ) {
        read_condition( jo, "condition", condition, false );
        has_condition = true;
    }
    if( jo.has_bool( "parse_tags" ) ) {
        should_parse_tags = jo.get_bool( "parse_tags" );
    }

    optional( jo, false, "widgets", widgets, string_id_reader<::widget> {} );
}

bool widget_clause::meets_condition( const std::string &opt_var ) const
{
    dialogue d( get_talker_for( get_avatar() ), nullptr );
    d.reason = opt_var; // TODO: remove since it's replaced by context var
    write_var_value( var_type::context, "npctalk_var_widget", nullptr, &d, opt_var );
    return !has_condition || condition( d );
}

bool widget_clause::meets_condition( const std::set<bodypart_id> &bps ) const
{
    if( bps.empty() ) {
        return meets_condition();
    }
    for( const bodypart_id &bid : bps ) {
        if( meets_condition( bid.id().str() ) ) {
            return true;
        }
    }
    return false;
}

const widget_clause *widget_clause::get_clause_for_id( const std::string &clause_id,
        const widget_id &wgt, int thresh_val, bool skip_condition )
{
    std::map<int, const widget_clause *> vals;
    for( const widget_clause &wp : wgt->_clauses ) {
        if( clause_id != wp.id || ( !skip_condition && !wp.meets_condition( wgt->_bps ) ) ) {
            continue;
        }
        if( thresh_val == INT_MIN || !skip_condition ) {
            return &wp;
        }
        vals.emplace( wp.value, &wp );
    }
    if( vals.empty() ) {
        return nullptr;
    }

    int key = INT_MIN;
    for( const auto &v : vals ) {
        if( v.first == thresh_val ) {
            return v.second;
        } else if( v.first > thresh_val ) {
            break;
        }
        key = v.first;
    }
    return key == INT_MIN ? nullptr : vals[key];
}

int widget_clause::get_val_for_id( const std::string &clause_id, const widget_id &wgt,
                                   bool skip_condition )
{
    const widget_clause *wp = widget_clause::get_clause_for_id( clause_id, wgt, INT_MIN,
                              skip_condition );
    return wp == nullptr ? INT_MIN : wp->value;
}

const translation &widget_clause::get_text_for_id( const std::string &clause_id,
        const widget_id &wgt, bool skip_condition )
{
    static const translation none;
    const widget_clause *wp = widget_clause::get_clause_for_id( clause_id, wgt, INT_MIN,
                              skip_condition );
    return wp == nullptr ? none : wp->text;
}

const std::string &widget_clause::get_sym_for_id( const std::string &clause_id,
        const widget_id &wgt, bool skip_condition )
{
    static const std::string none;
    const widget_clause *wp = widget_clause::get_clause_for_id( clause_id, wgt, INT_MIN,
                              skip_condition );
    return wp == nullptr ? none : wp->sym;
}

nc_color widget_clause::get_color_for_id( const std::string &clause_id, const widget_id &wgt,
        bool skip_condition )
{
    const widget_clause *wp = widget_clause::get_clause_for_id( clause_id, wgt, INT_MIN,
                              skip_condition );
    return wp == nullptr ? c_white : wp->color;
}

void widget::load( const JsonObject &jo, const std::string_view )
{
    optional( jo, was_loaded, "width", _width, 0 );
    optional( jo, was_loaded, "height", _height_max, 1 );
    optional( jo, was_loaded, "symbols", _symbols, "-" );
    optional( jo, was_loaded, "fill", _fill, "bucket" );
    optional( jo, was_loaded, "label", _label, translation() );
    optional( jo, was_loaded, "description", _description, "" );
    optional( jo, was_loaded, "style", _style, "number" );
    optional( jo, was_loaded, "arrange", _arrange, "columns" );
    optional( jo, was_loaded, "body_graph", _body_graph, "full_body_widget" );
    optional( jo, was_loaded, "direction", _direction, cardinal_direction::num_cardinal_directions );
    optional( jo, was_loaded, "text_align", _text_align, widget_alignment::LEFT );
    optional( jo, was_loaded, "label_align", _label_align, widget_alignment::LEFT );
    // Row layouts and non-layout widgets default to true
    optional( jo, was_loaded, "pad_labels", _pad_labels, _style != "layout" || _arrange == "rows" );
    optional( jo, was_loaded, "flags", _flags );

    if( _style == "sidebar" ) {
        mandatory( jo, was_loaded, "separator", _separator );
        mandatory( jo, was_loaded, "padding", _padding );
        explicit_separator = true;
        explicit_padding = true;
    } else {
        explicit_separator = jo.has_string( "separator" );
        explicit_padding = jo.has_number( "padding" );
        optional( jo, was_loaded, "separator", _separator, default_separator );
        optional( jo, was_loaded, "padding", _padding, 2 );
    }
    _height = _height_max;
    _label_width = _label.empty() ||
                   has_flag( json_flag_W_LABEL_NONE ) ? 0 : utf8_width( _label.translated() );

    if( jo.has_string( "var" ) ) {
        _var = io::string_to_enum<widget_var>( jo.get_string( "var" ) );
    }

    if( jo.has_string( "bodypart" ) ) {
        _bps.clear();
        _bps.emplace( jo.get_string( "bodypart" ) );
    }

    if( jo.has_array( "bodyparts" ) ) {
        _bps.clear();
        for( const JsonValue val : jo.get_array( "bodyparts" ) ) {
            if( !val.test_string() ) {
                jo.throw_error_at( "bodyparts", "Invalid string value in bodyparts array" );
                continue;
            }
            _bps.emplace( val.get_string() );
        }
    }

    if( jo.has_array( "colors" ) ) {
        _colors.clear();
        _breaks.clear();
        for( const std::string color_name : jo.get_array( "colors" ) ) {
            _colors.emplace_back( get_all_colors().name_to_color( color_name ) );
        }
        if( jo.has_array( "breaks" ) ) {
            for( const int value : jo.get_array( "breaks" ) ) {
                _breaks.emplace_back( value );
            }
            if( _breaks.size() != _colors.size() - 1 ) {
                debugmsg( "Widget property 'breaks' must have one element less than 'colors'" );
            }
        }
    }

    if( jo.has_array( "clauses" ) ) {
        _clauses.clear();
        for( JsonObject jobj : jo.get_array( "clauses" ) ) {
            widget_clause phs;
            phs.load( jobj );
            _clauses.emplace_back( phs );
        }
    }

    if( jo.has_object( "default_clause" ) ) {
        _default_clause.load( jo.get_object( "default_clause" ) );
    }
    if( _style == "text" && _clauses.empty() && _var == widget_var::last ) {
        mandatory( jo, was_loaded, "string", _string );
    } else {
        optional( jo, was_loaded, "string", _string );
    }
    optional( jo, was_loaded, "widgets", _widgets, string_id_reader<::widget> {} );
}

// Returns the derived label width for this widget/layout
int widget::finalize_label_width_recursive( const widget_id &id )
{
    widget *w = nullptr;
    // Get the original widget from the widget factory.
    for( const widget &wgt : widget::get_all() ) {
        if( wgt.getId() == id ) {
            w = const_cast<widget *>( &wgt );
            break;
        }
    }
    // None found, return 0 as the label width.
    if( w == nullptr ) {
        return 0;
    } else if( w->_widgets.empty() ) {
        // No more nested layouts, we've found an individual widget.
        // Return the widget's label width, or 0 if the label or pad_labels is disabled.
        return w->_label.empty() || w->has_flag( json_flag_W_LABEL_NONE ) ||
               ! w->_pad_labels ? 0 : w->_label_width;
    }
    // If we get here, we have a layout that contains nested widgets.

    // Find the longest label width within this layout.
    int width = 0;
    for( const widget_id &wid : w->_widgets ) {
        // Dive deeper to retrieve the nested element's label width.
        int tmpw = widget::finalize_label_width_recursive( wid );
        if( tmpw > width ) {
            width = tmpw;
        }
    }

    if( w->_pad_labels ) {
        // Update this layout's label width to reflect the longest label within.
        w->_label_width = width;
    } else {
        w->_label_width = 0;
    }
    return w->_label_width;
}

void widget::finalize_inherited_fields_recursive( const widget_id &id,
        const std::string &label_separator, const int col_padding )
{
    widget *w = nullptr;
    // Get the original widget from the widget factory.
    for( const widget &wgt : widget::get_all() ) {
        if( wgt.getId() == id ) {
            w = const_cast<widget *>( &wgt );
            break;
        }
    }
    if( w == nullptr ) {
        return;
    }
    if( !w->explicit_separator ) {
        w->_separator = label_separator;
    }
    if( !w->explicit_padding ) {
        w->_padding = col_padding;
    }

    // If we get here, we have a layout that contains nested widgets.
    for( const widget_id &wid : w->_widgets ) {
        widget::finalize_inherited_fields_recursive( wid,
                w->explicit_separator ? w->_separator : label_separator,
                w->explicit_padding ? w->_padding : col_padding );
    }

    // Also do it for widgets used in clauses
    for( const widget_clause &clause : w->_clauses ) {
        for( const widget_id &wid : clause.widgets ) {
            widget::finalize_inherited_fields_recursive( wid,
                    w->explicit_separator ? w->_separator : label_separator,
                    w->explicit_padding ? w->_padding : col_padding );
        }
    }
}

void widget::finalize()
{
    widget_factory.finalize();

    for( const widget &wgt : widget::get_all() ) {
        if( wgt._style == "sidebar" ) {
            widget::finalize_inherited_fields_recursive( wgt.getId(), wgt._separator, wgt._padding );
            widget::finalize_label_width_recursive( wgt.getId() );
        }
    }
}

const bodypart_id &widget::only_bp() const
{
    static const bodypart_id bp_null = bodypart_str_id::NULL_ID();
    // For widgets that rely on just one bodypart, assume there's only one in the set
    return _bps.empty() ? bp_null : *_bps.begin();
}

void widget::set_default_var_range( const avatar &ava )
{
    _var_min = INT_MIN;
    _var_max = INT_MAX;
    _var_norm = std::make_pair( INT_MIN, INT_MAX );

    switch( _var ) {
        case widget_var::cardio_acc:
            break; // TODO
        case widget_var::cardio_fit:
            _var_min = 0;
            // Same maximum used by get_cardiofit - 3 x BMR, adjusted for mutations
            _var_max = 3 * ava.base_bmr() * ava.mutation_value( "cardio_multiplier" );
            break;
        case widget_var::carry_weight:
            _var_min = 0;
            _var_max = 120;
            break;
        case widget_var::fatigue:
            _var_min = 0;
            _var_max = 1000;
            break;
        case widget_var::focus:
            _var_min = 0;
            _var_max = 200;
            // Small range of normal focus that won't be color-coded
            _var_norm = std::make_pair( 90, 110 );
            break;
        case widget_var::health:
            _var_min = -200;
            _var_max = 200;
            // Small range of normal health that won't be color-coded
            _var_norm = std::make_pair( -10, 10 );
            break;
        case widget_var::mana:
            _var_min = 0;
            _var_max = ava.magic->max_mana( ava );
            break;
        case widget_var::max_mana:
            _var_min = 0;
            // What could "max max mana" mean? Use 2x current max because why not
            _var_max = 2 * ava.magic->max_mana( ava );
            break;
        case widget_var::power_percentage:
            _var_min = 0;
            _var_max = 100;
            break;
        case widget_var::log_power_balance:
            _var_min = 0;
            _var_max = 1200;
            _var_norm = std::make_pair( 600, 600 );
            break;
        case widget_var::mood:
            break; // TODO
        case widget_var::morale_level:
            _var_min = -100;
            _var_max = 100;
            // Small range of morale that isn't worth crying about
            _var_norm = std::make_pair( -10, 10 );
            break;
        case widget_var::move:
            _var_min = 0;
            _var_max = 1000; // TODO: Determine better max
            // Move cost of last action
            break;
        case widget_var::move_remainder:
            _var_min = 0;
            _var_max = 9999; // TODO: Determine better max
            // remaining moves for the current turn
            break;
        case widget_var::move_cost:
            _var_min = 0;
            _var_max = 300; // Can go up to 500-600 while prone
            _var_norm = std::make_pair( 100, 100 );
            break;
        case widget_var::pain:
            _var_min = 0;
            _var_max = 80;
            // Zero pain isn't really normal but it's the state we prefer
            _var_norm = std::make_pair( 0, 0 );
            break;
        case widget_var::sound:
            _var_min = 0;
            _var_max = 200;
            // Quiet sounds are normal
            _var_norm = std::make_pair( 0, 5 );
            break;
        case widget_var::speed:
            _var_min = 0;
            _var_max = 200;
            _var_norm.first = ava.get_speed_base();
            _var_norm.second = ava.get_speed_base();
            break;
        case widget_var::stamina:
            _var_min = 0;
            _var_max = ava.get_stamina_max();
            // No normal defined, unless we want max stamina to be colored white? (maybe)
            break;
        case widget_var::weariness_level:
            _var_min = 0;
            _var_max = 10;
            break;
        case widget_var::weary_transition_level:
            _var_min = 0;
            _var_max = ava.weary_threshold();
            break;

        // Base stats
        // Normal is the base stat value only; min and max are -3 and +3 from base
        case widget_var::stat_str:
            _var_norm.first = ava.get_str_base();
            _var_norm.second = ava.get_str_base();
            _var_min = _var_norm.first - 3;
            _var_max = _var_norm.first + 3;
            break;
        case widget_var::stat_dex:
            _var_norm.first = ava.get_dex_base();
            _var_norm.second = ava.get_dex_base();
            _var_min = _var_norm.first - 3;
            _var_max = _var_norm.first + 3;
            break;
        case widget_var::stat_int:
            _var_norm.first = ava.get_int_base();
            _var_norm.second = ava.get_int_base();
            _var_min = _var_norm.first - 3;
            _var_max = _var_norm.first + 3;
            break;
        case widget_var::stat_per:
            _var_norm.first = ava.get_per_base();
            _var_norm.second = ava.get_per_base();
            _var_min = _var_norm.first - 3;
            _var_max = _var_norm.first + 3;
            break;

        // Bodypart attributes
        case widget_var::bp_hp:
            // HP for body part
            _var_min = 0;
            if( ava.has_part( only_bp(), body_part_filter::equivalent ) ) {
                _var_max = ava.get_part_hp_max( only_bp() );
            } else {
                _var_max = 0;
            }
            break;
        case widget_var::bp_encumb:
            _var_min = 0;
            _var_max = 100; // ???
            break;
        case widget_var::bp_warmth:
            // From weather.h: Body temperature is measured on a scale of 0u to 10000u,
            // where 10u = 0.02C and 5000u is 37C
            _var_min = 0;
            _var_max = 10000;
            break;
        case widget_var::bp_wetness:
            _var_min = 0;
            _var_max = 100; // ???
            break;
        default:
            break;
    }
}

int widget::get_var_value( const avatar &ava ) const
{
    // Numeric value to be rendered in the widget
    int value = 0;

    // Each "var" value refers to some attribute, typically of the avatar, that yields a numeric
    // value, and can be displayed as a numeric field, a graph, or a series of text clauses.
    switch( _var ) {
        // Vars with a known max val
        case widget_var::stamina:
            value = ava.get_stamina();
            break;
        case widget_var::mana:
            value = ava.magic->available_mana();
            break;
        case widget_var::max_mana:
            value = ava.magic->max_mana( ava );
            break;
        case widget_var::power_percentage:
            value = ava.has_max_power() ? ( 100 * ava.get_power_level().value() ) /
                    ava.get_max_power_level().value() : 0;
            break;
        case widget_var::log_power_balance: {
            int value_abs = std::abs( ava.power_balance.value() );
            if( value_abs < 500 ) {
                value = 0;
            } else {
                int sign = ava.power_balance.value() > 0 ? 1 : -1;
                value = ( sign * 100.0 * std::log10( value_abs / 500.0 ) );
            }
            value += 600;
            break;
        }
        case widget_var::morale_level:
            value = ava.get_morale_level();
            break;
        case widget_var::bp_hp:
            // HP for body part
            if( ava.has_part( only_bp(), body_part_filter::equivalent ) ) {
                value = ava.get_part_hp_cur( only_bp() );
            } else {
                value = 0;
            }
            break;
        case widget_var::bp_warmth:
            // Body part warmth/temperature
            if( ava.has_part( only_bp(), body_part_filter::equivalent ) ) {
                value = units::to_legacy_bodypart_temp( ava.get_part_temp_cur( only_bp() ) );
            } else {
                value = 0;
            }
            break;
        case widget_var::bp_wetness:
            // Body part wetness
            if( ava.has_part( only_bp(), body_part_filter::equivalent ) ) {
                value = ava.get_part_wetness( only_bp() );
            } else {
                value = 0;
            }
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
        case widget_var::move_remainder:
            value = ava.moves;
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
            value = ava.get_lifestyle();
            break;
        case widget_var::weariness_level:
            value = ava.weariness_level();
            break;
        case widget_var::weary_transition_level:
            value = ava.weariness_transition_level();
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
            value = ava.get_part_encumbrance_data( only_bp() ).encumbrance;
            break;
        case widget_var::cardio_fit:
            value = ava.get_cardiofit();
            break;
        case widget_var::cardio_acc:
            value = ava.get_cardio_acc();
            break;
        case widget_var::carry_weight:
            value = ( 100 * ava.weight_carried() ) / ava.weight_capacity();
            break;

        // TODO
        case widget_var::mood:
        // see morale_emotion
        default:
            value = 0;
    }
    return value;
}

bool widget::has_flag( const flag_id &flag ) const
{
    return _flags.count( flag ) != 0;
}

bool widget::has_flag( const std::string &flag ) const
{
    return has_flag( flag_id( flag ) );
}

std::string widget::show( const avatar &ava, const unsigned int max_width )
{
    set_default_var_range( ava );
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
        return color_value_string( value, max_width );
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
static int custom_draw_func( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;
    widget *wgt = args.get_widget();

    // Get full window width
    const int width = catacurses::getmaxx( w );
    // Leave 1 character space for margin on left and right
    const int margin = 1;
    const int widt = width - 2 * margin;
    // Whether to subtract height lines from the drawn panel space
    const bool disable_empty = wgt->has_flag( json_flag_W_DISABLED_WHEN_EMPTY );

    // Quit if there is nothing to draw or no space to draw it
    if( wgt == nullptr || width <= 0 ) {
        return 0;
    }

    int height_diff = 0;
    const bool skip_pad = wgt->has_flag( json_flag_W_NO_PADDING );

    werase( w );
    if( wgt->_style == "sidebar" ) {
        // noop
    } else if( wgt->_style == "layout" ) {
        if( wgt->_arrange == "rows" ) {
            // Layout widgets in rows
            std::vector<string_id<widget>> widgets = wgt->widgets( !wgt->_clauses.empty() );
            int row_num = 0;
            for( const widget_id &row_wid : widgets ) {
                widget row_widget = row_wid.obj();

                const std::string txt = row_widget.layout( u, widt, wgt->_label_width,
                                        skip_pad || row_wid->has_flag( json_flag_W_NO_PADDING ) );
                if( row_wid->has_flag( json_flag_W_DISABLED_WHEN_EMPTY ) && txt.empty() ) {
                    // reclaim the skipped height in the sidebar
                    height_diff -= row_widget._height;
                } else {
                    // draw normally
                    row_num = widget::custom_draw_multiline( txt, w, margin, widt, row_num );
                }
            }

            if( wgt->has_flag( json_flag_W_DYNAMIC_HEIGHT ) ) {
                wgt->_height = row_num;
            }
            if( disable_empty && widgets.empty() ) {
                // reclaim the skipped height in the sidebar
                height_diff -= wgt->_height;
            }
        } else {
            // Layout widgets in columns
            // For now, this is the default when calling layout()
            // So, just layout self on a single line

            const std::string txt = wgt->layout( u, widt, wgt->_label_width, skip_pad );
            if( disable_empty && txt.empty() ) {
                // reclaim the skipped height in the sidebar
                height_diff -= wgt->_height;
            } else {
                // draw normally
                widget::custom_draw_multiline( txt, w, margin, widt, 0 );
            }
        }
    } else {
        // No layout, just a widget
        const std::string txt = wgt->layout( u, widt, 0, skip_pad );
        if( disable_empty && txt.empty() ) {
            // reclaim the skipped height in the sidebar
            height_diff -= wgt->_height;
        } else {
            // draw normally
            widget::custom_draw_multiline( txt, w, margin, widt, 0 );
        }
    }
    wnoutrefresh( w );

    return height_diff;
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
        for( const widget_clause &clause : _clauses ) {
            int clause_height = 0;
            for( const widget_id &wid : clause.widgets ) {
                clause_height += wid->_height > 0 ? wid->_height : 1;
            }
            if( clause_height > height ) {
                height = clause_height;
            }
        }
    } else if( _style == "widget" || _style == "text" ) {
        height = _height > 1 ? _height : req_height;
    }
    // Minimap and log do not have a predetermined height
    // (or they should allow caller to customize height)

    window_panel win( _label.translated(), _label, height, width,
                      !has_flag( json_flag_W_DISABLED_BY_DEFAULT ) );
    win.set_widget( this->id );
    win.set_draw_func( custom_draw_func );
    return win;
}

bool widget::uses_text_function() const
{
    switch( _var ) {
        case widget_var::activity_text:
        case widget_var::body_graph:
        case widget_var::body_graph_temp:
        case widget_var::body_graph_encumb:
        case widget_var::body_graph_status:
        case widget_var::body_graph_wet:
        case widget_var::bp_armor_outer_text:
        case widget_var::carry_weight_text:
        case widget_var::compass_text:
        case widget_var::compass_legend_text:
        case widget_var::date_text:
        case widget_var::env_temp_text:
        case widget_var::mood_text:
        case widget_var::move_count_mode_text:
        case widget_var::pain_text:
        case widget_var::overmap_loc_text:
        case widget_var::overmap_text:
        case widget_var::place_text:
        case widget_var::power_text:
        case widget_var::power_balance_text:
        case widget_var::safe_mode_text:
        case widget_var::safe_mode_classic_text:
        case widget_var::style_text:
        case widget_var::sundial_text:
        case widget_var::sundial_time_text:
        case widget_var::time_text:
        case widget_var::veh_azimuth_text:
        case widget_var::veh_cruise_text:
        case widget_var::veh_fuel_text:
        case widget_var::weariness_text:
        case widget_var::weary_malus_text:
        case widget_var::weather_text:
        case widget_var::wielding_text:
        case widget_var::wielding_simple_text:
        case widget_var::wielding_mode_text:
        case widget_var::wielding_ammo_text:
        case widget_var::wind_text:
            return true;
        default:
            return false;
    }
}

// Simple workaround from the copied widget from the panel to set the widget's height globally
static void set_height_for_widget( const widget_id &id, int height )
{
    const std::vector<widget> &wlist = widget::get_all();
    auto iter = std::find_if( wlist.begin(), wlist.end(), [&id]( const widget & w ) {
        return w.getId() == id;
    } );
    if( iter != wlist.end() ) {
        const_cast<widget *>( &*iter )->_height = height;
    }
}

// NOTE: Use max_width to split multi-line widgets across lines
std::string widget::color_text_function_string( const avatar &ava, unsigned int max_width )
{
    std::string ret;
    // Most text variables have both a string and a color.
    // The string and color in `desc` will be converted to colorized text with markup.
    std::pair<std::string, nc_color> desc;
    // Set a default color
    desc.second = c_light_gray;
    // By default, colorize the string in desc.first with the color in desc.second.
    bool apply_color = true;
    // Don't bother updating the widget's height by default
    bool update_height = false;
    // Some helper display:: functions do their own internal colorization of the string.
    // For those, desc.first is the already-colorized string, and apply_color is set to false.
    switch( _var ) {
        case widget_var::activity_text:
            desc = display::activity_text_color( ava );
            break;
        case widget_var::body_graph:
            desc.first = display::colorized_bodygraph_text( ava, _body_graph,
                         bodygraph_var::hp, _width == 0 ? max_width : _width, _height_max, _height );
            update_height = true; // Dynamically adjusted height
            apply_color = false; // Already colorized
            break;
        case widget_var::body_graph_temp:
            desc.first = display::colorized_bodygraph_text( ava, _body_graph,
                         bodygraph_var::temp, _width == 0 ? max_width : _width, _height_max, _height );
            update_height = true; // Dynamically adjusted height
            apply_color = false; // Already colorized
            break;
        case widget_var::body_graph_encumb:
            desc.first = display::colorized_bodygraph_text( ava, _body_graph,
                         bodygraph_var::encumb, _width == 0 ? max_width : _width, _height_max, _height );
            update_height = true; // Dynamically adjusted height
            apply_color = false; // Already colorized
            break;
        case widget_var::body_graph_status:
            desc.first = display::colorized_bodygraph_text( ava, _body_graph,
                         bodygraph_var::status, _width == 0 ? max_width : _width, _height_max, _height );
            update_height = true; // Dynamically adjusted height
            apply_color = false; // Already colorized
            break;
        case widget_var::body_graph_wet:
            desc.first = display::colorized_bodygraph_text( ava, _body_graph,
                         bodygraph_var::wet, _width == 0 ? max_width : _width, _height_max, _height );
            update_height = true; // Dynamically adjusted height
            apply_color = false; // Already colorized
            break;
        case widget_var::bp_armor_outer_text:
            desc.first = ava.worn.get_armor_display( only_bp() );
            apply_color = false; // Item name already colorized by tname
            break;
        case widget_var::carry_weight_text:
            desc = display::carry_weight_text_color( ava );
            break;
        case widget_var::date_text:
            desc.first = display::date_string();
            break;
        case widget_var::env_temp_text:
            desc.first = display::get_temp( ava );
            break;
        case widget_var::mood_text:
            desc = display::morale_face_color( ava );
            break;
        case widget_var::move_count_mode_text:
            desc = display::move_count_and_mode_text_color( ava );
            break;
        case widget_var::pain_text:
            desc = display::pain_text_color( ava );
            break;
        case widget_var::overmap_loc_text:
            desc.first = display::overmap_position_text( ava.global_omt_location() );
            break;
        case widget_var::overmap_text:
            desc.first = display::colorized_overmap_text( ava, _width == 0 ? max_width : _width, _height );
            apply_color = false;
            break;
        case widget_var::place_text:
            desc.first = display::current_position_text( ava.global_omt_location() );
            break;
        case widget_var::power_text:
            desc = display::power_text_color( ava );
            break;
        case widget_var::power_balance_text:
            desc = display::power_balance_text_color( ava );
            break;
        case widget_var::safe_mode_text:
            desc = display::safe_mode_text_color( false );
            break;
        case widget_var::safe_mode_classic_text:
            desc = display::safe_mode_text_color( true );
            break;
        case widget_var::style_text:
            desc.first = ava.martial_arts_data->selected_style_name( ava );
            break;
        case widget_var::sundial_text:
            desc.first = display::sundial_text_color( ava, _width == 0 ? max_width : _width );
            apply_color = false; // Already colorized
            break;
        case widget_var::sundial_time_text:
            desc.first = display::sundial_time_text_color( ava, _width == 0 ? max_width : _width );
            apply_color = false; // Already colorized
            break;
        case widget_var::time_text:
            desc.first = display::time_string( ava );
            break;
        case widget_var::veh_azimuth_text:
            desc.first = display::vehicle_azimuth_text( ava );
            break;
        case widget_var::veh_cruise_text:
            desc = display::vehicle_cruise_text_color( ava );
            break;
        case widget_var::veh_fuel_text:
            desc = display::vehicle_fuel_percent_text_color( ava );
            break;
        case widget_var::weariness_text:
            desc = display::weariness_text_color( ava );
            break;
        case widget_var::weary_malus_text:
            desc = display::weary_malus_text_color( ava );
            break;
        case widget_var::weather_text:
            desc = display::weather_text_color( ava );
            break;
        case widget_var::wielding_text:
            desc.first = ava.weapname();
            break;
        case widget_var::wielding_simple_text:
            desc.first = ava.weapname_simple();
            break;
        case widget_var::wielding_mode_text:
            desc.first = ava.weapname_mode();
            break;
        case widget_var::wielding_ammo_text:
            desc.first = ava.weapname_ammo();
            break;
        case widget_var::wind_text:
            desc = display::wind_text_color( ava );
            break;
        case widget_var::compass_text:
            desc.first = display::colorized_compass_text( _direction, _width );
            apply_color = false; // Already colorized
            break;
        case widget_var::compass_legend_text:
            desc.first = display::colorized_compass_legend_text( _width == 0 ? max_width : _width, _height_max,
                         _height );
            update_height = true; // Dynamically adjusted height
            apply_color = false; // Already colorized
            break;
        default:
            debugmsg( "Unexpected widget_var %s - no text_color function defined",
                      io::enum_to_string<widget_var>( _var ) );
            return _( "???" );
    }
    // Update height dynamically for widgets that support it
    if( update_height && has_flag( json_flag_W_DYNAMIC_HEIGHT ) ) {
        set_height_for_widget( id, _height ); // Set within widget factory
    } else {
        _height = _height_max; // reset height
    }
    // Colorize if applicable
    if( !_colors.empty() ) {
        ret += colorize( desc.first, _colors.front() );
    } else if( apply_color ) {
        ret += colorize( desc.first, desc.second );
    } else {
        ret += desc.first;
    }
    return ret;
}

std::string widget::color_value_string( int value, int width_max )
{
    std::string val_string = value_string( value, width_max );
    const nc_color cur_color = value_color( value );
    if( cur_color == c_unset ) {
        return val_string;
    } else {
        return colorize( val_string, cur_color );
    }
}

std::string widget::value_string( int value, int width_max )
{
    std::string ret;
    // Use the available horizontal space unless widget has an explicit width
    const int w = _width > 0 ? _width : width_max;
    if( _style == "graph" ) {
        ret += graph( value );
    } else if( _style == "text" ) {
        ret += text( !_clauses.empty(), w );
    } else if( _style == "symbol" ) {
        ret += sym( !_clauses.empty() );
    } else if( _style == "legend" ) {
        ret += sym_text( !_clauses.empty(), w );
    } else if( _style == "number" ) {
        ret += number( value, !_clauses.empty() );
    } else {
        ret += "???";
    }
    return ret;
}

nc_color widget::value_color( int value )
{
    // Maps the range of values from min to max to the list of "colors", 0-indexed
    // [ _var_min ... ( _var_norm ) ... _var_max ]
    // [ 0, 1, 2, ...           ... num_colors-1 ]
    //
    if( _colors.empty() ) {
        return c_unset;
    }
    // Index of last color
    const int color_max = _colors.size() - 1;
    // Whether min, max, and normal are defined
    const bool min_max_defined = INT_MIN < _var_min && _var_max < INT_MAX;
    const bool normal_defined = INT_MIN < _var_norm.first && _var_norm.second < INT_MAX;

    // Get range of values from min to max
    const int var_range = _var_max - _var_min;

    if( ! _breaks.empty() ) {
        const int value_offset = ( 100 * ( value - _var_min ) ) / var_range;
        for( int i = 0; i < color_max; i++ ) {
            if( value_offset < _breaks[i] ) {
                return _colors[i];
            }
        }
        return _colors[color_max];
    }

    // Convert value to a positive offset within the range
    const int value_offset = std::max( value - _var_min, 0 );

    // If (min, max) is a valid nonzero range, fit value to a color using that range
    if( min_max_defined && var_range > 0 ) {
        // If value is within defined _var_norm range, the color is c_white
        if( normal_defined && _var_norm.first <= value && value <= _var_norm.second ) {
            return c_white;
        } else if( value <= _var_min ) {
            // If smaller than lower limit, return first color
            return _colors.front();
        } else if( value >= _var_max ) {
            // If larger than upper limit, return last color
            return _colors.back();
        } else {
            // If value is within the range, map it to an appropriate color
            // Scale value offset within range from [0, 1] to map color range
            const double scale = static_cast<double>( value_offset ) / var_range;
            // The +0.5 offset makes top and bottom color slots more equitable
            // (without the offset, only the max value gets the top color)
            const int color_index = std::floor( scale * color_max + 0.5 );
            return _colors[color_index];
        }
    }
    // No scaling by min-max range; assume colors map to 0, 1, 2 ...
    // Truncate below 0 and above color_max
    if( value < 0 ) {
        return _colors[0];
    } else if( value <= color_max ) {
        return _colors[value];
    } else {
        return _colors.back();
    }
}

std::string widget::number( int value, bool from_condition ) const
{
    return from_condition ? number_cond() : string_format( "%d", value );
}

std::string widget::text( bool from_condition, int width )
{
    if( from_condition ) {
        return text_cond( false, width );
    }
    return _string.translated();
}

std::string widget::sym( bool from_condition )
{
    return from_condition ? sym_cond() : text( from_condition, 0 );
}

std::string widget::sym_text( bool from_condition, int width )
{
    if( from_condition ) {
        return sym_text_cond( true, width );
    }
    return _string.translated();
}

std::vector<string_id<widget>> widget::widgets( bool from_condition )
{
    if( from_condition ) {
        return widgets_cond();
    }
    return _widgets;
}

std::string widget::number_cond( enumeration_conjunction join_type ) const
{
    std::vector<const widget_clause *> wplist = get_clauses();
    if( wplist.empty() ) {
        // All clauses returned false conditions, use default
        std::string txt = string_format( "%d", _default_clause.value );
        return _default_clause.value == INT_MIN ? "" :
               _default_clause.color == c_unset ? txt : colorize( txt, _default_clause.color );
    }
    // Get values as a comma-separated list
    return enumerate_as_string( wplist.begin(), wplist.end(), []( const widget_clause * wp ) {
        std::string txt = string_format( "%d", wp->value );
        return wp->color == c_unset ? txt : colorize( txt, wp->color );
    }, join_type );
}

std::string widget::text_cond( bool no_join, int width )
{
    std::vector<const widget_clause *> wplist = get_clauses();
    if( wplist.empty() ) {
        // All clauses returned false conditions, use default
        std::string txt = _default_clause.text.translated();
        return txt.empty() ? "" : _default_clause.color == c_unset ? txt : colorize( txt,
                _default_clause.color );
    }
    std::vector<std::string> strings;
    strings.reserve( wplist.size() );
    for( const widget_clause *wp : wplist ) {
        std::string txt = wp->text.translated();
        if( wp->should_parse_tags ) {
            parse_tags( txt, get_player_character(), get_player_character() );
        }
        txt = wp->color == c_unset ? txt : colorize(
                  txt, wp->color );
        strings.emplace_back( txt );
    }
    int h = 0;
    std::string ret = format_widget_multiline( strings, _height_max, width, h, !no_join );
    if( has_flag( json_flag_W_DYNAMIC_HEIGHT ) ) {
        _height = h;
        set_height_for_widget( id, h );
    }
    return ret;
}

std::string widget::sym_cond( bool no_join, enumeration_conjunction join_type ) const
{
    std::vector<const widget_clause *> wplist = get_clauses();
    if( wplist.empty() ) {
        // All clauses returned false conditions, use default
        std::string txt = _default_clause.sym;
        return txt.empty() ? "" : _default_clause.color == c_unset ? txt : colorize( txt,
                _default_clause.color );
    }
    // No string joining, just show symbols one-after-the-other
    if( no_join ) {
        std::string ret;
        for( const widget_clause *wp : wplist ) {
            ret += wp->color == c_unset ? wp->sym : colorize( wp->sym, wp->color );
        }
        return ret;
    }
    // Get values as a comma-separated list
    return enumerate_as_string( wplist.begin(), wplist.end(), []( const widget_clause * wp ) {
        return wp->color == c_unset ? wp->sym : colorize( wp->sym, wp->color );
    }, join_type );
}

std::string widget::sym_text_cond( bool no_join, int width )
{
    std::vector<const widget_clause *> wplist = get_clauses();
    if( wplist.empty() ) {
        // All clauses returned false conditions, use default
        std::string txt = _default_clause.sym;
        txt += ( txt.empty() ? "" : " " ) + _default_clause.text.translated();
        return txt.empty() ? "" : _default_clause.color == c_unset ? txt : colorize( txt,
                _default_clause.color );
    }
    std::vector<std::string> strings;
    strings.reserve( wplist.size() );
    for( const widget_clause *wp : wplist ) {
        std::string s = wp->color == c_unset ? wp->sym : colorize( wp->sym, wp->color );
        std::string txt_str = wp->text.translated();
        if( wp->should_parse_tags ) {
            parse_tags( txt_str, get_player_character(), get_player_character() );
        }
        std::string txt = string_format( "%s %s", s, txt_str );
        strings.emplace_back( txt );
    }
    int h = 0;
    std::string ret = format_widget_multiline( strings, _height_max, width, h, !no_join );
    if( has_flag( json_flag_W_DYNAMIC_HEIGHT ) ) {
        _height = h;
        set_height_for_widget( id, h );
    }
    return ret;
}

std::vector<string_id<widget>> widget::widgets_cond()
{
    std::vector<const widget_clause *> wplist = get_clauses();
    if( wplist.empty() ) {
        return _default_clause.widgets;
    }
    std::vector<string_id<widget>> widgets;
    for( const widget_clause *wp : wplist ) {
        widgets.insert( widgets.end(), wp->widgets.begin(), wp->widgets.end() );
    }
    return widgets;
}

const widget_clause *widget::get_clause( const std::string &clause_id ) const
{
    // Look for a clause with satisfied conditions
    for( const widget_clause &wp : _clauses ) {
        // If an id is given, only check conditions for that id
        if( !clause_id.empty() && clause_id != wp.id ) {
            continue;
        }
        // Return this clause if it has no condition or the condition is true
        if( !wp.has_condition || wp.meets_condition( only_bp().id().str() ) ) {
            return &wp;
        }
    }
    return nullptr;
}

std::vector<const widget_clause *> widget::get_clauses() const
{
    std::vector<const widget_clause *> ret;
    // Look for clauses with satisfied conditions
    for( const widget_clause &wp : _clauses ) {
        // Include this clause if it has no condition or the condition is true
        if( !wp.has_condition || wp.meets_condition( _bps ) ) {
            ret.emplace_back( &wp );
        }
    }
    return ret;
}

std::string widget::graph( int value ) const
{
    // graph "depth is equal to the number of nonzero symbols
    int depth = utf8_width( _symbols ) - 1;
    // Number of graph characters
    const int w = _arrange == "rows" ? _height : _width;
    // Max integer value this graph can show
    int max_graph_val = w * depth;
    // Scale value range to current graph resolution (width x depth)
    if( _var_max > 0 && _var_max != max_graph_val ) {
        // Scale max source value to max graph value
        value = max_graph_val * value / _var_max;
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

    const std::wstring syms = utf8_to_wstr( _symbols );
    std::wstring ret;
    if( _fill == "bucket" ) {
        quot = value / depth; // number of full cells/buckets
        rem = value % depth;  // partly full next cell, maybe
        // Full cells at the front
        ret += std::wstring( quot, syms.back() );
        // Any partly-full cells?
        if( w > quot ) {
            // Current partly-full cell
            ret += syms[rem];
            // Any more zero cells at the end
            if( w > quot + 1 ) {
                ret += std::wstring( w - quot - 1, syms[0] );
            }
        }
    } else if( _fill == "pool" ) {
        quot = value / w; // baseline depth of the pool
        rem = value % w;  // number of cells at next depth
        // Most-filled cells come first
        if( rem > 0 ) {
            ret += std::wstring( rem, syms[quot + 1] );
            // Less-filled cells may follow
            if( w > rem ) {
                ret += std::wstring( w - rem, syms[quot] );
            }
        } else {
            // All cells at the same level
            ret += std::wstring( w, syms[quot] );
        }
    } else {
        debugmsg( "Unknown widget fill type %s", _fill );
        return "";
    }

    // Re-arrange characters to a vertical bar graph
    if( _arrange == "rows" ) {
        std::wstring temp = ret;
        ret.clear();
        for( int i = temp.size() - 1; i >= 0; i-- ) {
            ret += temp[i];
            if( i > 0 ) {
                ret += '\n';
            }
        }
    }

    return wstr_to_utf8( ret );
}

// For widget::layout, process each row to append to the layout string
static std::string append_line( const std::string &line, bool first_row, int max_width,
                                const translation &label, int label_width, const std::string &_separator,
                                widget_alignment text_align, widget_alignment label_align, bool skip_pad )
{
    // utf8_width subtracts 1 for each newline; add it back for multiline widgets
    const int newline_fix = !line.empty() && line.back() == '\n' ? 1 : 0;

    // Prepare label
    int lbl_w = 0;
    std::string lbl;
    if( first_row && !label.empty() ) {
        lbl = label.translated();
        lbl_w = utf8_width( lbl, true );
        lbl.append( _separator );
    }
    // Don't process label width if label_width = 0 for empty labels
    if( !skip_pad && ( label_width > 0 || !label.empty() ) ) {
        lbl_w += _separator.length();
        label_width += _separator.length();
        // Use empty spaces in place of label if none exist
        if( label.empty() ) {
            lbl.append( label_width, ' ' );
            lbl_w = label_width;
        }
    }

    // Prepare text
    int txt_w = 0;
    std::string txt;
    if( !line.empty() ) {
        txt = line;
        txt_w = utf8_width( txt, true ) + newline_fix;
        if( line.back() == '\n' ) {
            txt.pop_back();
        }
    }

    // Label padding
    if( !skip_pad && label_width > lbl_w ) {
        const int lpad = label_width - lbl_w;
        // Left side
        int padding = 0;
        if( label_align != widget_alignment::LEFT ) {
            padding = label_align == widget_alignment::RIGHT ? lpad : ( lpad / 2 ) + lpad % 2;
        }
        lbl.insert( 0, padding, ' ' );
        lbl_w += padding;
        // Right side
        padding = 0;
        if( label_align != widget_alignment::RIGHT ) {
            padding = label_align == widget_alignment::LEFT ? lpad : lpad / 2;
        }
        lbl.append( padding, ' ' );
        lbl_w += padding;
    }

    // If the text is too long, start eating the free space next to the label.
    // This only works because labels are not colorized (no color tags).
    if( txt_w + lbl_w > max_width ) {
        std::wstring tmplbl = utf8_to_wstr( lbl );
        for( int i = tmplbl.size() - 1; txt_w + lbl_w > max_width && i > 0 && tmplbl[i] == ' ' &&
             tmplbl[i - 1] != ':'; i-- ) {
            tmplbl.pop_back();
            lbl_w--;
        }
        lbl = wstr_to_utf8( tmplbl );
    }

    // Text padding
    if( !skip_pad && max_width - lbl_w > txt_w ) {
        const int tpad = ( max_width - lbl_w ) - txt_w;
        // Left side
        int padding = 0;
        if( text_align != widget_alignment::LEFT &&
            // Don't pad left if the label is also right-aligned
            ( text_align == widget_alignment::CENTER || label_align != widget_alignment::RIGHT ) ) {
            padding = text_align == widget_alignment::RIGHT ? tpad : ( tpad / 2 ) + tpad % 2;
        }
        txt.insert( 0, padding, ' ' );
        txt_w += padding;
        // Right side
        padding = 0;
        if( text_align != widget_alignment::RIGHT ) {
            padding = text_align == widget_alignment::LEFT ? tpad : tpad / 2;
        }
        txt.append( padding, ' ' );
        txt_w += padding;
    }

    // Final assembly
    std::string ret = lbl + txt;
    if( !skip_pad && text_align == widget_alignment::RIGHT ) {
        const int leftover = max_width - ( lbl_w + txt_w );
        if( leftover > 0 ) {
            ret.insert( 0, leftover, ' ' );
        }
    }

    if( newline_fix == 1 ) {
        ret.append( 1, '\n' );
    }

    return ret;
}

std::string widget::layout( const avatar &ava, unsigned int max_width, int label_width,
                            bool skip_pad )
{
    std::string ret;
    if( _style == "layout" ) {
        std::vector<string_id<widget>> wgts = widgets( !_clauses.empty() );
        int layout_label_width = ( label_width == 0 || ! _pad_labels ) ? _label_width : label_width;

        if( _arrange == "rows" ) {
            std::string sep;
            int h = 0;
            // Stack rows vertically into a multiline widget
            for( const widget_id &wid : wgts ) {
                widget cur_child = wid.obj();
                ret += sep + cur_child.layout( ava, max_width, layout_label_width,
                                               skip_pad || wid->has_flag( json_flag_W_NO_PADDING ) );
                sep = "\n";
                h += wid->_height < 0 ? 0 : wid->_height;
            }
            // Set height for the final layout
            set_height_for_widget( id, h );
        } else { // columns
            const int num_widgets = wgts.size();
            if( num_widgets == 0 ) {
                debugmsg( "widget layout has no widgets" );
            }
            // Number of spaces between columns
            const int col_padding = _padding;
            // Subtract column padding to get space available for widgets
            const int avail_width = max_width - col_padding * ( num_widgets - 1 );
            // Divide available width equally among all widgets
            const int child_width = avail_width / num_widgets;
            // Total widget width w/o padding
            const int total_widget_width = std::accumulate( wgts.begin(), wgts.end(), 0,
            [child_width]( int sum, const widget_id & wid ) {
                widget cur_child = wid.obj();
                return sum + ( cur_child._style == "layout" &&
                               cur_child._width > 0 ? cur_child._width : child_width );
            } );
            // Total widget width with padding
            const int total_widget_padded_width = total_widget_width + col_padding * ( num_widgets - 1 );
            // Keep remainder to distribute
            int remainder = max_width - total_widget_padded_width;
            // Store the (potentially) multi-row text for each column
            std::vector<std::vector<std::string>> cols;
            std::vector<int> widths;
            unsigned int total_width = 0;
            std::string debug_widths;
            for( size_t i = 0; i < wgts.size(); i++ ) {
                const widget_id &wid = wgts[i];
                widget cur_child = wid.obj();
                int cur_width = child_width;
                // determine spacing based on type of column
                if( _arrange == "minimum_columns" ) {
                    if( cur_child._width > 0 ) {
                        cur_width = cur_child._width;
                    }
                    // if last widget make it take the remaining space
                    if( i == wgts.size() - 1 ) {
                        cur_width = std::max<int>( 0, avail_width - total_width );
                    }
                } else { //columns
                    if( cur_child._style == "layout" && cur_child._width > 0 ) {
                        cur_width = cur_child._width;
                    }
                    // Spread remainder over the first few columns
                    if( remainder > 0 ) {
                        cur_width += 1;
                        remainder -= 1;
                    }
                }

                // for debug keep track of each and width
                debug_widths.append( string_format( "%s: %d,", wid.str(), cur_width ) );

                if( cur_width > 0 ) {
                    total_width += cur_width;
                }
                if( total_width > max_width ) {
                    debugmsg( string_format( "widget layout is wider (%d) than sidebar allows (%d) for %s.",
                                             total_width, max_width, debug_widths ) );
                }
                const bool skip_pad_this = skip_pad || wid->has_flag( json_flag_W_NO_PADDING );
                // Layout child in this column
                const std::string txt = cur_child.layout( ava, skip_pad_this ? 0 : cur_width,
                                        layout_label_width, skip_pad_this );
                // Store the resulting text for this column
                cols.emplace_back( string_split( txt, '\n' ) );
                widths.emplace_back( cur_width );
            }

            int h_max = 0;
            std::string sep;
            // Line up each row of each column to form the whole multi-line layout
            for( size_t r = 0; ; r++ ) {
                bool any_val = false;
                std::string line;
                for( size_t c = 0; c < cols.size(); c++ ) {
                    if( r >= cols[c].size() ) {
                        if( !skip_pad ) {
                            // No row r for this column, pad with empty space
                            line += std::string( widths[c], ' ' );
                        }
                    } else {
                        any_val = true;
                        line += cols[c][r];
                    }
                    if( !skip_pad && c + 1 < cols.size() ) {
                        // Add padding between columns
                        line += std::string( col_padding, ' ' );
                    }
                }
                if( !any_val ) {
                    break;
                }
                ret += sep + line;
                sep = "\n";
                h_max++;
            }
            // Set height for the final layout
            set_height_for_widget( id, h_max );
        }
    } else {
        // Get displayed value (colorized)
        std::string shown = show( ava, max_width );
        // If nothing was printed, the widget never had a chance to adjust the height. Adjust it here.
        if( shown.empty() && has_flag( json_flag_W_DYNAMIC_HEIGHT ) ) {
            _height = 0;
        }
        // Let the calling func know that this widget should be skipped for rendering
        if( has_flag( json_flag_W_DISABLED_WHEN_EMPTY ) &&
            string_empty_or_whitespace( remove_color_tags( shown ) ) ) {
            return "";
        }
        size_t strpos = 0;
        int row_num = 0;
        // For multi-line widgets, each line is separated by a '\n' character
        while( ( strpos = shown.find( '\n' ) ) != std::string::npos && row_num < _height ) {
            // Process line, including '\n'
            ret += append_line( shown.substr( 0, strpos + 1 ), row_num == 0, max_width,
                                has_flag( json_flag_W_LABEL_NONE ) ? translation() : _label,
                                0, _separator, _text_align, _label_align, skip_pad );
            // Delete used token
            shown.erase( 0, strpos + 1 );
            row_num++;
        }
        if( row_num < _height ) {
            // Process last line, or first for single-line widgets
            ret += append_line( shown, row_num == 0, max_width,
                                has_flag( json_flag_W_LABEL_NONE ) ? translation() : _label,
                                row_num == 0 && _pad_labels ? label_width : 0, _separator, _text_align, _label_align, skip_pad );
        }
        if( !ret.empty() && ret.back() == '\n' ) {
            ret.pop_back();
        }
    }
    return ret.find( '\n' ) != std::string::npos || max_width == 0 ?
           ret : trim_by_length( ret, max_width );
}

std::string format_widget_multiline( const std::vector<std::string> &keys, int max_height,
                                     int width, int &height, bool join )
{
    std::string ret;
    height = 0;
    // For single-line text, just lay everything on the same line
    if( width <= 0 && max_height == 1 ) {
        width = INT_MAX;
    }
    const int h_max = max_height == 0 ? INT_MAX : max_height;
    const int nsize = keys.size();
    for( int row = 0, nidx = 0; row < h_max && nidx < nsize; row++ ) {
        int wavail = width;
        int nwidth = utf8_width( keys[nidx], true );
        bool startofline = true;
        while( nidx < nsize && ( wavail > nwidth || startofline ) ) {
            startofline = false;
            wavail -= nwidth;
            ret += keys[nidx];
            nidx++;
            if( nidx < nsize ) {
                nwidth = utf8_width( keys[nidx], true );
                if( wavail > nwidth ) {
                    ret += join ? ", " : "  ";
                    wavail -= 2;
                }
            }
        }
        // Newline, if not the last row
        if( row < h_max - 1 ) {
            ret += "\n";
        }
        height++;
    }
    return ret;
}
