#include "condition.h"

#include <climits>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "action.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "coordinates.h"
#include "dialogue.h"
#include "debug.h"
#include "dialogue_helpers.h"
#include "enum_conversions.h"
#include "field.h"
#include "flag.h"
#include "game.h"
#include "generic_factory.h"
#include "global_vars.h"
#include "item.h"
#include "item_category.h"
#include "json.h"
#include "kill_tracker.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "martialarts.h"
#include "math_parser.h"
#include "math_parser_shim.h"
#include "mission.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "popup.h"
#include "profession.h"
#include "ranged.h"
#include "recipe_groups.h"
#include "talker.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "widget.h"
#include "worldfactory.h"

class basecamp;
class recipe;

static const efftype_id effect_currently_busy( "currently_busy" );

static const json_character_flag json_flag_MUTATION_THRESHOLD( "MUTATION_THRESHOLD" );

namespace
{
struct deferred_math {
    std::string str;
    bool assignment;
    std::shared_ptr<math_exp> exp;

    deferred_math( std::string_view str_, bool ass_ ) : str( str_ ), assignment( ass_ ),
        exp( std::make_shared<math_exp>() ) {}
};

struct condition_parser {
    using condition_func = conditional_t::func;
    using f_t = condition_func( * )( const JsonObject &, std::string_view );
    using f_t_beta = condition_func( * )( const JsonObject &, std::string_view, bool );
    using f_t_simple = condition_func( * )();
    using f_t_beta_simple = condition_func( * )( bool );

    condition_parser( std::string_view key_alpha_, jarg arg_, f_t f_ ) : key_alpha( key_alpha_ ),
        arg( arg_ ), f( f_ ) {}
    condition_parser( std::string_view key_alpha_, std::string_view key_beta_, jarg arg_,
                      f_t_beta f_ ) : key_alpha( key_alpha_ ), key_beta( key_beta_ ), arg( arg_ ), f_beta( f_ ) {
        has_beta = true;
    }
    condition_parser( std::string_view key_alpha_, f_t_simple f_ ) : key_alpha( key_alpha_ ),
        f_simple( f_ ) {}
    condition_parser( std::string_view key_alpha_, std::string_view key_beta_,
                      f_t_beta_simple f_ ) : key_alpha( key_alpha_ ), key_beta( key_beta_ ), f_beta_simple( f_ ) {
        has_beta = true;
    }

    bool check( const JsonObject &jo, bool beta = false ) const {
        std::string_view key = beta ? key_beta : key_alpha;
        if( ( ( arg & jarg::member ) && jo.has_member( key ) ) ||
            ( ( arg & jarg::object ) && jo.has_object( key ) ) ||
            ( ( arg & jarg::string ) && jo.has_string( key ) ) ||
            ( ( arg & jarg::array ) && jo.has_array( key ) ) ) {
            return true;
        }
        return false;
    }

    bool has_beta = false;
    std::string_view key_alpha;
    std::string_view key_beta;
    jarg arg;
    f_t f;
    f_t_beta f_beta;
    f_t_simple f_simple;
    f_t_beta_simple f_beta_simple;
};

std::queue<deferred_math> &get_deferred_math()
{
    static std::queue<deferred_math> dfr_math;
    return dfr_math;
}

std::shared_ptr<math_exp> &defer_math( std::string_view str, bool ass )
{
    get_deferred_math().emplace( str, ass );
    return get_deferred_math().back().exp;
}

}  // namespace

std::string get_talk_varname( const JsonObject &jo, std::string_view member,
                              bool check_value, dbl_or_var &default_val )
{
    if( check_value && !( jo.has_string( "value" ) ||
                          jo.has_array( "possible_values" ) ) ) {
        jo.throw_error( "invalid " + std::string( member ) + " condition in " + jo.str() );
    }
    const std::string &var_basename = jo.get_string( std::string( member ) );
    const std::string &type_var = jo.get_string( "type", "" );
    const std::string &var_context = jo.get_string( "context", "" );
    default_val = get_dbl_or_var( jo, "default", false );
    return "npctalk_var" + ( type_var.empty() ? "" : "_" + type_var ) + ( var_context.empty() ? "" : "_"
            + var_context ) + "_" + var_basename;
}

std::string get_talk_var_basename( const JsonObject &jo, std::string_view member,
                                   bool check_value )
{
    if( check_value && !( jo.has_string( "value" ) ||
                          jo.has_array( "possible_values" ) ) ) {
        jo.throw_error( "invalid " + std::string( member ) + " condition in " + jo.str() );
    }
    const std::string &var_basename = jo.get_string( std::string( member ) );
    return var_basename;
}

dbl_or_var_part get_dbl_or_var_part( const JsonValue &jv, std::string_view member, bool required,
                                     double default_val )
{
    dbl_or_var_part ret_val;
    if( jv.test_float() ) {
        ret_val.dbl_val = jv.get_float();
    } else if( jv.test_object() ) {
        JsonObject jo = jv.get_object();
        jo.allow_omitted_members();
        if( jo.has_array( "arithmetic" ) ) {
            ret_val.arithmetic_val = talk_effect_fun_t::from_arithmetic( jo, "arithmetic", true );
        } else if( jo.has_array( "math" ) ) {
            ret_val.math_val.emplace();
            ret_val.math_val->from_json( jo, "math", eoc_math::type_t::ret );
        } else {
            ret_val.var_val = read_var_info( jo );
        }
    } else if( required ) {
        jv.throw_error( "No valid value for " + std::string( member ) );
    } else {
        ret_val.dbl_val = default_val;
    }
    return ret_val;
}

dbl_or_var get_dbl_or_var( const JsonObject &jo, std::string_view member, bool required,
                           double default_val )
{
    dbl_or_var ret_val;
    if( jo.has_array( member ) ) {
        JsonArray ja = jo.get_array( member );
        ret_val.min = get_dbl_or_var_part( ja.next_value(), member );
        ret_val.max = get_dbl_or_var_part( ja.next_value(), member );
        ret_val.pair = true;
    } else if( required ) {
        ret_val.min = get_dbl_or_var_part( jo.get_member( member ), member, required, default_val );
    } else {
        if( jo.has_member( member ) ) {
            ret_val.min = get_dbl_or_var_part( jo.get_member( member ), member, required, default_val );
        } else {
            ret_val.min.dbl_val = default_val;
        }
    }
    return ret_val;
}

duration_or_var_part get_duration_or_var_part( const JsonValue &jv, const std::string_view &member,
        bool required, time_duration default_val )
{
    duration_or_var_part ret_val;
    if( jv.test_string() ) {
        if( jv.get_string() == "infinite" ) {
            ret_val.dur_val = time_duration::from_turns( calendar::INDEFINITELY_LONG );
        } else {
            ret_val.dur_val = read_from_json_string<time_duration>( jv, time_duration::units );
        }
    } else if( jv.test_int() ) {
        ret_val.dur_val = time_duration::from_turns( jv.get_float() );
    } else if( jv.test_object() ) {
        JsonObject jo = jv.get_object();
        jo.allow_omitted_members();
        if( jo.has_array( "arithmetic" ) ) {
            ret_val.arithmetic_val = talk_effect_fun_t::from_arithmetic( jo, "arithmetic", true );
        } else if( jo.has_array( "math" ) ) {
            ret_val.math_val.emplace();
            ret_val.math_val->from_json( jo, "math", eoc_math::type_t::ret );
        } else {
            ret_val.var_val = read_var_info( jo );
        }
    } else if( required ) {
        jv.throw_error( "No valid value for " + std::string( member ) );
    } else {
        ret_val.dur_val = default_val;
    }
    return ret_val;
}

duration_or_var get_duration_or_var( const JsonObject &jo, const std::string_view &member,
                                     bool required,
                                     time_duration default_val )
{
    duration_or_var ret_val;
    if( jo.has_array( member ) ) {
        JsonArray ja = jo.get_array( member );
        ret_val.min = get_duration_or_var_part( ja.next_value(), member );
        ret_val.max = get_duration_or_var_part( ja.next_value(), member );
        ret_val.pair = true;
    } else if( required ) {
        ret_val.min = get_duration_or_var_part( jo.get_member( member ), member, required, default_val );
    } else {
        if( jo.has_member( member ) ) {
            ret_val.min = get_duration_or_var_part( jo.get_member( member ), member, required, default_val );
        } else {
            ret_val.min.dur_val = default_val;
        }
    }
    return ret_val;
}

str_or_var get_str_or_var( const JsonValue &jv, std::string_view member, bool required,
                           std::string_view default_val )
{
    str_or_var ret_val;
    if( jv.test_string() ) {
        ret_val.str_val = jv.get_string();
    } else if( jv.test_object() ) {
        const JsonObject &jo = jv.get_object();
        if( jo.has_member( "mutator" ) ) {
            // if we have a mutator then process that here.
            ret_val.function = conditional_t::get_get_string( jo );
        } else {
            ret_val.var_val = read_var_info( jo );
            ret_val.default_val = default_val;
        }
    } else if( required ) {
        jv.throw_error( "No valid value for " + std::string( member ) );
    } else {
        ret_val.str_val = default_val;
    }
    return ret_val;
}

translation_or_var get_translation_or_var( const JsonValue &jv, std::string_view member,
        bool required, const translation &default_val )
{
    translation_or_var ret_val;
    translation str_val;
    if( jv.read( str_val ) ) {
        ret_val.str_val = str_val;
    } else if( jv.test_object() ) {
        const JsonObject &jo = jv.get_object();
        if( jo.has_member( "mutator" ) ) {
            // if we have a mutator then process that here.
            ret_val.function = conditional_t::get_get_translation( jo );
        } else {
            ret_val.var_val = read_var_info( jo );
            ret_val.default_val = default_val;
        }
    } else if( required ) {
        jv.throw_error( "No valid value for " + std::string( member ) );
    } else {
        ret_val.str_val = default_val;
    }
    return ret_val;
}

tripoint_abs_ms get_tripoint_from_var( std::optional<var_info> var, dialogue const &d )
{
    tripoint_abs_ms target_pos = get_map().getglobal( d.actor( false )->pos() );
    if( var.has_value() ) {
        std::string value = read_var_value( var.value(), d );
        if( !value.empty() ) {
            target_pos = tripoint_abs_ms( tripoint::from_string( value ) );
        }
    }
    return target_pos;
}

var_info read_var_info( const JsonObject &jo )
{
    std::string default_val;
    dbl_or_var empty;
    var_type type;
    std::string name;
    if( jo.has_string( "default_str" ) ) {
        default_val = jo.get_string( "default_str" );
    } else if( jo.has_string( "default" ) ) {
        default_val = std::to_string( to_turns<int>( read_from_json_string<time_duration>
                                      ( jo.get_member( "default" ), time_duration::units ) ) );
    } else if( jo.has_float( "default" ) ) {
        default_val = std::to_string( jo.get_float( "default" ) );
    }

    if( jo.has_string( "var_name" ) ) {
        const std::string &type_var = jo.get_string( "type", "" );
        const std::string &var_context = jo.get_string( "context", "" );
        name = "npctalk_var_" + type_var + ( type_var.empty() ? "" : "_" ) + var_context +
               ( var_context.empty() ? "" : "_" )
               + jo.get_string( "var_name" );
    }
    if( jo.has_member( "u_val" ) ) {
        type = var_type::u;
        if( name.empty() ) {
            name = get_talk_varname( jo, "u_val", false, empty );
        }
    } else if( jo.has_member( "npc_val" ) ) {
        type = var_type::npc;
        if( name.empty() ) {
            name = get_talk_varname( jo, "npc_val", false, empty );
        }
    } else if( jo.has_member( "global_val" ) ) {
        type = var_type::global;
        if( name.empty() ) {
            name = get_talk_varname( jo, "global_val", false, empty );
        }
    } else if( jo.has_member( "var_val" ) ) {
        type = var_type::var;
        if( name.empty() ) {
            name = get_talk_varname( jo, "var_val", false, empty );
        }
    } else if( jo.has_member( "context_val" ) ) {
        type = var_type::context;
        if( name.empty() ) {
            name = get_talk_varname( jo, "context_val", false, empty );
        }
    } else if( jo.has_member( "faction_val" ) ) {
        type = var_type::faction;
        if( name.empty() ) {
            name = get_talk_varname( jo, "faction_val", false, empty );
        }
    } else if( jo.has_member( "party_val" ) ) {
        type = var_type::party;
        if( name.empty() ) {
            name = get_talk_varname( jo, "party_val", false, empty );
        }
    } else {
        jo.throw_error( "Invalid variable type." );
    }
    return var_info( type, name, default_val );
}

void write_var_value( var_type type, const std::string &name, talker *talk, dialogue *d,
                      const std::string &value )
{
    global_variables &globvars = get_globals();
    std::string ret;
    var_info vinfo( var_type::global, "" );
    switch( type ) {
        case var_type::global:
            globvars.set_global_value( name, value );
            break;
        case var_type::var:
            ret = d->get_value( name );
            vinfo = process_variable( ret );
            write_var_value( vinfo.type, vinfo.name, talk, d, value );
            break;
        case var_type::u:
        case var_type::npc:
            talk->set_value( name, value );
            break;
        case var_type::faction:
            debugmsg( "Not implemented yet." );
            break;
        case var_type::party:
            debugmsg( "Not implemented yet." );
            break;
        case var_type::context:
            d->set_value( name, value );
            break;
        default:
            debugmsg( "Invalid type." );
            break;
    }
}

void write_var_value( var_type type, const std::string &name, talker *talk, dialogue *d,
                      double value )
{
    // NOLINTNEXTLINE(cata-translate-string-literal)
    write_var_value( type, name, talk, d, string_format( "%g", value ) );
}

static bodypart_id get_bp_from_str( const std::string &ctxt )
{
    bodypart_id bid = bodypart_str_id::NULL_ID();
    if( !ctxt.empty() ) {
        bid = bodypart_id( ctxt );
        if( !bid.is_valid() ) {
            bid = bodypart_str_id::NULL_ID();
        }
    }
    return bid;
}

void read_condition( const JsonObject &jo, const std::string &member_name,
                     conditional_t::func &condition, bool default_val )
{
    const auto null_function = [default_val]( dialogue const & ) {
        return default_val;
    };

    if( !jo.has_member( member_name ) ) {
        condition = null_function;
    } else if( jo.has_string( member_name ) ) {
        const std::string type = jo.get_string( member_name );
        conditional_t sub_condition( type );
        condition = [sub_condition]( dialogue & d ) {
            return sub_condition( d );
        };
    } else if( jo.has_object( member_name ) ) {
        JsonObject con_obj = jo.get_object( member_name );
        conditional_t sub_condition( con_obj );
        condition = [sub_condition]( dialogue & d ) {
            return sub_condition( d );
        };
    } else {
        jo.throw_error_at( member_name, "invalid condition syntax" );
    }
}

void finalize_conditions()
{
    std::queue<deferred_math> &dfr = get_deferred_math();
    while( !dfr.empty() ) {
        deferred_math &math = dfr.front();
        math.exp->parse( math.str, math.assignment );
        dfr.pop();
    }
}

static std::string get_string_from_input( const JsonArray &objects, int index )
{
    if( objects.has_string( index ) ) {
        std::string type = objects.get_string( index );
        if( type == "u" || type == "npc" ) {
            return type;
        }
    }
    dbl_or_var empty;
    JsonObject object = objects.get_object( index );
    if( object.has_string( "u_val" ) ) {
        return "u_" + get_talk_varname( object, "u_val", false, empty );
    } else if( object.has_string( "npc_val" ) ) {
        return "npc_" + get_talk_varname( object, "npc_val", false, empty );
    } else if( object.has_string( "global_val" ) ) {
        return "global_" + get_talk_varname( object, "global_val", false, empty );
    } else if( object.has_string( "context_val" ) ) {
        return "context_" + get_talk_varname( object, "context_val", false, empty );
    } else if( object.has_string( "faction_val" ) ) {
        return "faction_" + get_talk_varname( object, "faction_val", false, empty );
    } else if( object.has_string( "party_val" ) ) {
        return "party_" + get_talk_varname( object, "party_val", false, empty );
    }
    object.throw_error( "Invalid input type." );
    return "";
}

static tripoint_abs_ms get_tripoint_from_string( const std::string &type, dialogue const &d )
{
    if( type == "u" ) {
        return d.actor( false )->global_pos();
    } else if( type == "npc" ) {
        return d.actor( true )->global_pos();
    } else if( type.find( "u_" ) == 0 ) {
        var_info var = var_info( var_type::u, type.substr( 2, type.size() - 2 ) );
        return get_tripoint_from_var( var, d );
    } else if( type.find( "npc_" ) == 0 ) {
        var_info var = var_info( var_type::npc, type.substr( 4, type.size() - 4 ) );
        return get_tripoint_from_var( var, d );
    } else if( type.find( "global_" ) == 0 ) {
        var_info var = var_info( var_type::global, type.substr( 7, type.size() - 7 ) );
        return get_tripoint_from_var( var, d );
    } else if( type.find( "faction_" ) == 0 ) {
        var_info var = var_info( var_type::faction, type.substr( 8, type.size() - 8 ) );
        return get_tripoint_from_var( var, d );
    } else if( type.find( "party_" ) == 0 ) {
        var_info var = var_info( var_type::party, type.substr( 6, type.size() - 6 ) );
        return get_tripoint_from_var( var, d );
    } else if( type.find( "context_" ) == 0 ) {
        var_info var = var_info( var_type::context, type.substr( 8, type.size() - 8 ) );
        return get_tripoint_from_var( var, d );
    }
    return tripoint_abs_ms();
}


namespace conditional_fun
{
namespace
{

conditional_t::func f_has_any_trait( const JsonObject &jo, std::string_view member, bool is_npc )
{
    std::vector<str_or_var> traits_to_check;
    for( JsonValue jv : jo.get_array( member ) ) {
        traits_to_check.emplace_back( get_str_or_var( jv, member ) );
    }
    return [traits_to_check, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        for( const str_or_var &trait : traits_to_check ) {
            if( actor->has_trait( trait_id( trait.evaluate( d ) ) ) ) {
                return true;
            }
        }
        return false;
    };
}

conditional_t::func f_has_trait( const JsonObject &jo, std::string_view member, bool is_npc )
{
    str_or_var trait_to_check = get_str_or_var( jo.get_member( member ), member, true );
    return [trait_to_check, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_trait( trait_id( trait_to_check.evaluate( d ) ) );
    };
}

conditional_t::func f_has_visible_trait( const JsonObject &jo, std::string_view member,
        bool is_npc )
{
    str_or_var trait_to_check = get_str_or_var( jo.get_member( member ), member, true );
    return [trait_to_check, is_npc]( dialogue const & d ) {
        const talker *observer = d.actor( !is_npc );
        const talker *observed = d.actor( is_npc );
        int visibility_cap = observer->get_character()->get_mutation_visibility_cap(
                                 observed->get_character() );
        bool observed_has = observed->has_trait( trait_id( trait_to_check.evaluate( d ) ) );
        const mutation_branch &mut_branch = trait_id( trait_to_check.evaluate( d ) ).obj();
        bool is_visible = mut_branch.visibility > 0 && mut_branch.visibility >= visibility_cap;
        return observed_has && is_visible;
    };
}

conditional_t::func f_has_martial_art( const JsonObject &jo, std::string_view member,
                                       bool is_npc )
{
    str_or_var style_to_check = get_str_or_var( jo.get_member( member ), member, true );
    return [style_to_check, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->knows_martial_art( matype_id( style_to_check.evaluate( d ) ) );
    };
}

conditional_t::func f_has_flag( const JsonObject &jo, std::string_view member,
                                bool is_npc )
{
    str_or_var trait_flag_to_check = get_str_or_var( jo.get_member( member ), member, true );
    return [trait_flag_to_check, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        if( json_character_flag( trait_flag_to_check.evaluate( d ) ) == json_flag_MUTATION_THRESHOLD ) {
            return actor->crossed_threshold();
        }
        return actor->has_flag( json_character_flag( trait_flag_to_check.evaluate( d ) ) );
    };
}

conditional_t::func f_has_species( const JsonObject &jo, std::string_view member,
                                   bool is_npc )
{
    str_or_var species_to_check = get_str_or_var( jo.get_member( member ), member, true );
    return [species_to_check, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        return actor->has_species( species_id( species_to_check.evaluate( d ) ) );
    };
}

conditional_t::func f_bodytype( const JsonObject &jo, std::string_view member,
                                bool is_npc )
{
    str_or_var bt_to_check = get_str_or_var( jo.get_member( member ), member, true );
    return [bt_to_check, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        return actor->bodytype( bodytype_id( bt_to_check.evaluate( d ) ) );
    };
}

conditional_t::func f_has_activity( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_activity();
    };
}

conditional_t::func f_has_activity( const JsonObject &, std::string_view, bool is_npc )
{
    return f_has_activity( is_npc );
}

conditional_t::func f_has_proficiency( const JsonObject &jo, std::string_view member,
                                       bool is_npc )
{
    str_or_var proficiency_to_check = get_str_or_var( jo.get_member( member ), member, true );
    return [proficiency_to_check, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->knows_proficiency( proficiency_id( proficiency_to_check.evaluate( d ) ) );
    };
}

conditional_t::func f_is_riding( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_mounted();
    };
}

conditional_t::func f_is_riding( const JsonObject &, std::string_view, bool is_npc )
{
    return f_is_riding( is_npc );
}

conditional_t::func f_npc_has_class( const JsonObject &jo, std::string_view member,
                                     bool is_npc )
{
    str_or_var class_to_check = get_str_or_var( jo.get_member( member ), member, true );
    return [class_to_check, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_myclass( npc_class_id( class_to_check.evaluate( d ) ) );
    };
}

conditional_t::func f_u_has_mission( const JsonObject &jo, std::string_view member )
{
    str_or_var u_mission = get_str_or_var( jo.get_member( member ), member, true );
    return [u_mission]( dialogue const & d ) {
        for( mission *miss_it : get_avatar().get_active_missions() ) {
            if( miss_it->mission_id() == mission_type_id( u_mission.evaluate( d ) ) ) {
                return true;
            }
        }
        return false;
    };
}

conditional_t::func f_u_monsters_in_direction( const JsonObject &jo,
        std::string_view member )
{
    str_or_var dir = get_str_or_var( jo.get_member( member ), member, true );
    return [dir]( dialogue const & d ) {
        //This string_to_enum function is defined in widget.h. Should it be moved?
        const int card_dir = static_cast<int>( io::string_to_enum<cardinal_direction>( dir.evaluate(
                d ) ) );
        int monster_count = get_avatar().get_mon_visible().unique_mons[card_dir].size();
        return monster_count > 0;
    };
}

conditional_t::func f_u_safe_mode_trigger( const JsonObject &jo, std::string_view member )
{
    str_or_var dir = get_str_or_var( jo.get_member( member ), member, true );
    return [dir]( dialogue const & d ) {
        //This string_to_enum function is defined in widget.h. Should it be moved?
        const int card_dir = static_cast<int>( io::string_to_enum<cardinal_direction>( dir.evaluate(
                d ) ) );
        return get_avatar().get_mon_visible().dangerous[card_dir];
    };
}

conditional_t::func f_u_profession( const JsonObject &jo, std::string_view member )
{
    str_or_var u_profession = get_str_or_var( jo.get_member( member ), member, true );
    return [u_profession]( dialogue const & d ) {
        const profession *prof = get_player_character().get_profession();
        std::set<const profession *> hobbies = get_player_character().get_hobbies();
        if( prof->get_profession_id() == profession_id( u_profession.evaluate( d ) ) ) {
            return true;
        } else if( profession_id( u_profession.evaluate( d ) )->is_hobby() ) {
            for( const profession *hob : hobbies ) {
                if( hob->get_profession_id() == profession_id( u_profession.evaluate( d ) ) ) {
                    return true;
                }
                break;
            }
            return false;
        } else {
            return false;
        }
    };
}

conditional_t::func f_has_strength( const JsonObject &jo, std::string_view member,
                                    bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [dov, is_npc]( dialogue & d ) {
        return d.actor( is_npc )->str_cur() >= dov.evaluate( d );
    };
}

conditional_t::func f_has_dexterity( const JsonObject &jo, std::string_view member,
                                     bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [dov, is_npc]( dialogue & d ) {
        return d.actor( is_npc )->dex_cur() >= dov.evaluate( d );
    };
}

conditional_t::func f_has_intelligence( const JsonObject &jo, std::string_view member,
                                        bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [dov, is_npc]( dialogue & d ) {
        return d.actor( is_npc )->int_cur() >= dov.evaluate( d );
    };
}

conditional_t::func f_has_perception( const JsonObject &jo, std::string_view member,
                                      bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [dov, is_npc]( dialogue & d ) {
        return d.actor( is_npc )->per_cur() >= dov.evaluate( d );
    };
}

conditional_t::func f_has_hp( const JsonObject &jo, std::string_view member, bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    std::optional<bodypart_id> bp;
    optional( jo, false, "bodypart", bp );
    return [dov, bp, is_npc]( dialogue & d ) {
        bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
        return d.actor( is_npc )->get_cur_hp( bid ) >= dov.evaluate( d );
    };
}

conditional_t::func f_has_part_temp( const JsonObject &jo, std::string_view member,
                                     bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    std::optional<bodypart_id> bp;
    optional( jo, false, "bodypart", bp );
    return [dov, bp, is_npc]( dialogue & d ) {
        bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
        return units::to_legacy_bodypart_temp( d.actor( is_npc )->get_cur_part_temp(
                bid ) ) >= dov.evaluate( d );
    };
}

conditional_t::func f_is_wearing( const JsonObject &jo, std::string_view member,
                                  bool is_npc )
{
    str_or_var item_id = get_str_or_var( jo.get_member( member ), member, true );
    return [item_id, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_wearing( itype_id( item_id.evaluate( d ) ) );
    };
}

conditional_t::func f_has_item( const JsonObject &jo, std::string_view member, bool is_npc )
{
    str_or_var item_id = get_str_or_var( jo.get_member( member ), member, true );
    return [item_id, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        return actor->charges_of( itype_id( item_id.evaluate( d ) ) ) > 0 ||
               actor->has_amount( itype_id( item_id.evaluate( d ) ), 1 );
    };
}

conditional_t::func f_has_items( const JsonObject &jo, const std::string_view member,
                                 bool is_npc )
{
    JsonObject has_items = jo.get_object( member );
    if( !has_items.has_member( "item" ) || ( !has_items.has_member( "count" ) &&
            !has_items.has_member( "charges" ) ) ) {
        return []( dialogue const & ) {
            return false;
        };
    } else {
        str_or_var item_id = get_str_or_var( has_items.get_member( "item" ), "item", true );
        dbl_or_var count = get_dbl_or_var( has_items, "count", false );
        dbl_or_var charges = get_dbl_or_var( has_items, "charges", false );
        return [item_id, count, charges, is_npc]( dialogue & d ) {
            const talker *actor = d.actor( is_npc );
            itype_id id = itype_id( item_id.evaluate( d ) );
            if( charges.evaluate( d ) == 0 && item::count_by_charges( id ) ) {
                return actor->has_charges( id, count.evaluate( d ), true );
            }
            if( charges.evaluate( d ) > 0 && count.evaluate( d ) == 0 ) {
                return actor->has_charges( id, charges.evaluate( d ), true );
            }
            bool has_enough_charges = true;
            if( charges.evaluate( d ) > 0 ) {
                has_enough_charges = actor->has_charges( id, charges.evaluate( d ), true );
            }
            return has_enough_charges && actor->has_amount( id, count.evaluate( d ) );
        };
    }
}

conditional_t::func f_has_item_with_flag( const JsonObject &jo, std::string_view member,
        bool is_npc )
{
    str_or_var flag = get_str_or_var( jo.get_member( member ), member, true );
    return [flag, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_item_with_flag( flag_id( flag.evaluate( d ) ) );
    };
}

conditional_t::func f_has_item_category( const JsonObject &jo, std::string_view member,
        bool is_npc )
{
    str_or_var category_id = get_str_or_var( jo.get_member( member ), member, true );
    size_t count = 1;
    if( jo.has_int( "count" ) ) {
        int tcount = jo.get_int( "count" );
        if( tcount > 1 && tcount < INT_MAX ) {
            count = static_cast<size_t>( tcount );
        }
    }

    return [category_id, count, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        const item_category_id cat_id = item_category_id( category_id.evaluate( d ) );
        const auto items_with = actor->const_items_with( [cat_id]( const item & it ) {
            return it.get_category_shallow().get_id() == cat_id;
        } );
        return items_with.size() >= count;
    };
}

conditional_t::func f_has_bionics( const JsonObject &jo, std::string_view member,
                                   bool is_npc )
{
    str_or_var bionics_id = get_str_or_var( jo.get_member( member ), member, true );
    return [bionics_id, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        if( bionics_id.evaluate( d ) == "ANY" ) {
            return actor->num_bionics() > 0 || actor->has_max_power();
        }
        return actor->has_bionic( bionic_id( bionics_id.evaluate( d ) ) );
    };
}

conditional_t::func f_has_any_effect( const JsonObject &jo, std::string_view member,
                                      bool is_npc )
{
    std::vector<str_or_var> effects_to_check;
    for( JsonValue jv : jo.get_array( member ) ) {
        effects_to_check.emplace_back( get_str_or_var( jv, member ) );
    }
    dbl_or_var intensity = get_dbl_or_var( jo, "intensity", false, -1 );
    str_or_var bp;
    if( jo.has_member( "bodypart" ) ) {
        bp = get_str_or_var( jo.get_member( "bodypart" ), "bodypart", true );
    } else {
        bp.str_val = "";
    }
    return [effects_to_check, intensity, bp, is_npc]( dialogue & d ) {
        bodypart_id bid = bp.evaluate( d ).empty() ? get_bp_from_str( d.reason ) :
                          bodypart_id( bp.evaluate( d ) );
        for( const str_or_var &effect_id : effects_to_check ) {
            effect target = d.actor( is_npc )->get_effect( efftype_id( effect_id.evaluate( d ) ), bid );
            if( !target.is_null() && intensity.evaluate( d ) <= target.get_intensity() ) {
                return true;
            }
        }
        return false;
    };
}

conditional_t::func f_has_effect( const JsonObject &jo, std::string_view member,
                                  bool is_npc )
{
    str_or_var effect_id = get_str_or_var( jo.get_member( member ), member, true );
    dbl_or_var intensity = get_dbl_or_var( jo, "intensity", false, -1 );
    str_or_var bp;
    if( jo.has_member( "bodypart" ) ) {
        bp = get_str_or_var( jo.get_member( "bodypart" ), "bodypart", true );
    } else {
        bp.str_val = "";
    }
    return [effect_id, intensity, bp, is_npc]( dialogue & d ) {
        bodypart_id bid = bp.evaluate( d ).empty() ? get_bp_from_str( d.reason ) :
                          bodypart_id( bp.evaluate( d ) );
        effect target = d.actor( is_npc )->get_effect( efftype_id( effect_id.evaluate( d ) ), bid );
        return !target.is_null() && intensity.evaluate( d ) <= target.get_intensity();
    };
}

conditional_t::func f_need( const JsonObject &jo, std::string_view member, bool is_npc )
{
    str_or_var need = get_str_or_var( jo.get_member( member ), member, true );
    dbl_or_var dov;
    if( jo.has_int( "amount" ) ) {
        dov.min.dbl_val = jo.get_int( "amount" );
    } else if( jo.has_object( "amount" ) ) {
        dov = get_dbl_or_var( jo, "amount" );
    } else if( jo.has_string( "level" ) ) {
        const std::string &level = jo.get_string( "level" );
        auto flevel = fatigue_level_strs.find( level );
        if( flevel != fatigue_level_strs.end() ) {
            dov.min.dbl_val = static_cast<int>( flevel->second );
        }
    }
    return [need, dov, is_npc]( dialogue & d ) {
        const talker *actor = d.actor( is_npc );
        int amount = dov.evaluate( d );
        return ( actor->get_fatigue() > amount && need.evaluate( d ) == "fatigue" ) ||
               ( actor->get_hunger() > amount && need.evaluate( d ) == "hunger" ) ||
               ( actor->get_thirst() > amount && need.evaluate( d ) == "thirst" );
    };
}

conditional_t::func f_at_om_location( const JsonObject &jo, std::string_view member,
                                      bool is_npc )
{
    str_or_var location = get_str_or_var( jo.get_member( member ), member, true );
    return [location, is_npc]( dialogue const & d ) {
        const tripoint_abs_omt omt_pos = d.actor( is_npc )->global_omt_location();
        const oter_id &omt_ter = overmap_buffer.ter( omt_pos );
        const std::string &omt_str = omt_ter.id().str();
        std::string location_value = location.evaluate( d );

        if( location_value == "FACTION_CAMP_ANY" ) {
            std::optional<basecamp *> bcp = overmap_buffer.find_camp( omt_pos.xy() );
            if( bcp ) {
                return true;
            }
            // TODO: legacy check to be removed once primitive field camp OMTs have been purged
            return omt_str.find( "faction_base_camp" ) != std::string::npos;
        } else if( location_value == "FACTION_CAMP_START" ) {
            return !recipe_group::get_recipes_by_id( "all_faction_base_types", omt_str ).empty();
        } else {
            return oter_no_dir( omt_ter ) == location_value;
        }
    };
}

conditional_t::func f_near_om_location( const JsonObject &jo, std::string_view member,
                                        bool is_npc )
{
    str_or_var location = get_str_or_var( jo.get_member( member ), member, true );
    const dbl_or_var range = get_dbl_or_var( jo, "range", false, 1 );
    return [location, range, is_npc]( dialogue & d ) {
        const tripoint_abs_omt omt_pos = d.actor( is_npc )->global_omt_location();
        for( const tripoint_abs_omt &curr_pos : points_in_radius( omt_pos,
                range.evaluate( d ) ) ) {
            const oter_id &omt_ter = overmap_buffer.ter( curr_pos );
            const std::string &omt_str = omt_ter.id().str();
            std::string location_value = location.evaluate( d );

            if( location_value == "FACTION_CAMP_ANY" ) {
                std::optional<basecamp *> bcp = overmap_buffer.find_camp( curr_pos.xy() );
                if( bcp ) {
                    return true;
                }
                // TODO: legacy check to be removed once primitive field camp OMTs have been purged
                if( omt_str.find( "faction_base_camp" ) != std::string::npos ) {
                    return true;
                }
            } else if( location_value  == "FACTION_CAMP_START" &&
                       !recipe_group::get_recipes_by_id( "all_faction_base_types", omt_str ).empty() ) {
                return true;
            } else {
                if( oter_no_dir( omt_ter ) == location_value ) {
                    return true;
                }
            }
        }
        // should never get here this is for safety
        return false;
    };
}

conditional_t::func f_has_var( const JsonObject &jo, std::string_view member, bool is_npc )
{
    dbl_or_var empty;
    const std::string var_name = get_talk_varname( jo, member, false, empty );
    const std::string &value = jo.has_member( "value" ) ? jo.get_string( "value" ) : std::string();
    if( !jo.has_member( "value" ) ) {
        jo.throw_error( R"(Missing field: "value")" );
        return []( dialogue const & ) {
            return false;
        };
    }

    return [var_name, value, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        return actor->get_value( var_name ) == value;
    };
}

conditional_t::func f_expects_vars( const JsonObject &jo, std::string_view member )
{
    std::vector<str_or_var> to_check;
    if( jo.has_array( member ) ) {
        for( const JsonValue &jv : jo.get_array( member ) ) {
            to_check.push_back( get_str_or_var( jv, member, true ) );
        }
    }

    return [to_check]( dialogue const & d ) {
        std::string missing_variables;
        for( const str_or_var &val : to_check ) {
            if( d.get_context().find( "npctalk_var_" + val.evaluate( d ) ) == d.get_context().end() ) {
                missing_variables += val.evaluate( d ) + ", ";
            }
        }
        if( !missing_variables.empty() ) {
            debugmsg( string_format( "Missing required variables: %s", missing_variables ) );
            return false;
        }
        return true;
    };
}

conditional_t::func f_compare_var( const JsonObject &jo, std::string_view member,
                                   bool is_npc )
{
    dbl_or_var empty;
    const std::string var_name = get_talk_varname( jo, member, false, empty );
    const std::string &op = jo.get_string( "op" );

    dbl_or_var dov = get_dbl_or_var( jo, "value" );
    return [var_name, op, dov, is_npc]( dialogue & d ) {
        double stored_value = 0;
        double value = dov.evaluate( d );
        const std::string &var = d.actor( is_npc )->get_value( var_name );
        if( !var.empty() ) {
            stored_value = std::stof( var );
        }

        if( op == "==" ) {
            return stored_value == value;

        } else if( op == "!=" ) {
            return stored_value != value;

        } else if( op == "<=" ) {
            return stored_value <= value;

        } else if( op == ">=" ) {
            return stored_value >= value;

        } else if( op == "<" ) {
            return stored_value < value;

        } else if( op == ">" ) {
            return stored_value > value;
        }

        return false;
    };
}

conditional_t::func f_npc_role_nearby( const JsonObject &jo, std::string_view member )
{
    str_or_var role = get_str_or_var( jo.get_member( member ), member, true );
    return [role]( dialogue const & d ) {
        const std::vector<npc *> available = g->get_npcs_if( [&]( const npc & guy ) {
            return d.actor( false )->posz() == guy.posz() &&
                   guy.companion_mission_role_id == role.evaluate( d ) &&
                   ( rl_dist( d.actor( false )->pos(), guy.pos() ) <= 48 );
        } );
        return !available.empty();
    };
}

conditional_t::func f_npc_allies( const JsonObject &jo, std::string_view member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [dov]( dialogue & d ) {
        return g->allies().size() >= static_cast<std::vector<npc *>::size_type>( dov.evaluate( d ) );
    };
}

conditional_t::func f_npc_allies_global( const JsonObject &jo, std::string_view member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [dov]( dialogue & d ) {
        const auto all_npcs = overmap_buffer.get_overmap_npcs();
        const size_t count = std::count_if( all_npcs.begin(),
        all_npcs.end(), []( const shared_ptr_fast<npc> &ptr ) {
            return ptr.get()->is_player_ally() && !ptr.get()->hallucination && !ptr.get()->is_dead();
        } );

        return count >= static_cast<size_t>( dov.evaluate( d ) );
    };
}

conditional_t::func f_u_has_cash( const JsonObject &jo, std::string_view member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [dov]( dialogue & d ) {
        return d.actor( false )->cash() >= dov.evaluate( d );
    };
}

conditional_t::func f_u_are_owed( const JsonObject &jo, std::string_view member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [dov]( dialogue & d ) {
        return d.actor( true )->debt() >= dov.evaluate( d );
    };
}

conditional_t::func f_npc_aim_rule( const JsonObject &jo, std::string_view member,
                                    bool is_npc )
{
    str_or_var setting = get_str_or_var( jo.get_member( member ), member, true );
    return [setting, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "aim_rule", setting.evaluate( d ) );
    };
}

conditional_t::func f_npc_engagement_rule( const JsonObject &jo, std::string_view member,
        bool is_npc )
{
    str_or_var setting = get_str_or_var( jo.get_member( member ), member, true );
    return [setting, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "engagement_rule", setting.evaluate( d ) );
    };
}

conditional_t::func f_npc_cbm_reserve_rule( const JsonObject &jo, std::string_view member,
        bool is_npc )
{
    str_or_var setting = get_str_or_var( jo.get_member( member ), member, true );
    return [setting, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "cbm_reserve_rule", setting.evaluate( d ) );
    };
}

conditional_t::func f_npc_cbm_recharge_rule( const JsonObject &jo, std::string_view member,
        bool is_npc )
{
    str_or_var setting = get_str_or_var( jo.get_member( member ), member, true );
    return [setting, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "cbm_recharge_rule", setting.evaluate( d ) );
    };
}

conditional_t::func f_npc_rule( const JsonObject &jo, std::string_view member, bool is_npc )
{
    str_or_var rule = get_str_or_var( jo.get_member( member ), member, true );
    return [rule, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "ally_rule", rule.evaluate( d ) );
    };
}

conditional_t::func f_npc_override( const JsonObject &jo, std::string_view member,
                                    bool is_npc )
{
    str_or_var rule = get_str_or_var( jo.get_member( member ), member, true );
    return [rule, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "ally_override", rule.evaluate( d ) );
    };
}

conditional_t::func f_is_season( const JsonObject &jo, std::string_view member )
{
    str_or_var season_name = get_str_or_var( jo.get_member( member ), member, true );
    return [season_name]( dialogue const & d ) {
        const season_type season = season_of_year( calendar::turn );
        return ( season == SPRING && season_name.evaluate( d ) == "spring" ) ||
               ( season == SUMMER && season_name.evaluate( d ) == "summer" ) ||
               ( season == AUTUMN && season_name.evaluate( d ) == "autumn" ) ||
               ( season == WINTER && season_name.evaluate( d ) == "winter" );
    };
}

conditional_t::func f_mission_goal( const JsonObject &jo, std::string_view member,
                                    bool is_npc )
{
    str_or_var mission_goal_str = get_str_or_var( jo.get_member( member ), member, true );
    return [mission_goal_str, is_npc]( dialogue const & d ) {
        mission *miss = d.actor( is_npc )->selected_mission();
        if( !miss ) {
            return false;
        }
        const mission_goal mgoal = io::string_to_enum<mission_goal>( mission_goal_str.evaluate( d ) );
        return miss->get_type().goal == mgoal;
    };
}

conditional_t::func f_is_gender( bool is_male, bool is_npc )
{
    return [is_male, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_male() == is_male;
    };
}

conditional_t::func f_is_male( bool is_npc )
{
    return f_is_gender( true, is_npc );
}
conditional_t::func f_is_female( bool is_npc )
{
    return f_is_gender( false, is_npc );
}

conditional_t::func f_is_alive( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->get_is_alive();
    };
}

conditional_t::func f_is_avatar( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->get_character() && d.actor( is_npc )->get_character()->is_avatar();
    };
}

conditional_t::func f_is_npc( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->get_npc();
    };
}

conditional_t::func f_is_character( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->get_character();
    };
}

conditional_t::func f_is_monster( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->get_monster();
    };
}

conditional_t::func f_is_item( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->get_item();
    };
}

conditional_t::func f_is_furniture( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->get_computer();
    };
}

conditional_t::func f_player_see( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        const Creature *c = d.actor( is_npc )->get_creature();
        if( c ) {
            return get_player_view().sees( *c );
        } else {
            return get_player_view().sees( d.actor( is_npc )->pos() );
        }
    };
}

conditional_t::func f_no_assigned_mission()
{
    return []( dialogue const & d ) {
        return d.missions_assigned.empty();
    };
}

conditional_t::func f_has_assigned_mission()
{
    return []( dialogue const & d ) {
        return d.missions_assigned.size() == 1;
    };
}

conditional_t::func f_has_many_assigned_missions()
{
    return []( dialogue const & d ) {
        return d.missions_assigned.size() >= 2;
    };
}

conditional_t::func f_no_available_mission( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->available_missions().empty();
    };
}

conditional_t::func f_has_available_mission( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->available_missions().size() == 1;
    };
}

conditional_t::func f_has_many_available_missions( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->available_missions().size() >= 2;
    };
}

conditional_t::func f_mission_complete( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        mission *miss = d.actor( is_npc )->selected_mission();
        return miss && miss->is_complete( d.actor( is_npc )->getID() );
    };
}

conditional_t::func f_mission_incomplete( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        mission *miss = d.actor( is_npc )->selected_mission();
        return miss && !miss->is_complete( d.actor( is_npc )->getID() );
    };
}

conditional_t::func f_mission_failed( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        mission *miss = d.actor( is_npc )->selected_mission();
        return miss && miss->has_failed();
    };
}

conditional_t::func f_npc_service( const JsonObject &jo, std::string_view member, bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [is_npc, dov]( dialogue & d ) {
        return !d.actor( is_npc )->has_effect( effect_currently_busy, bodypart_str_id::NULL_ID() ) &&
               d.actor( false )->cash() >= dov.evaluate( d );
    };
}

conditional_t::func f_npc_available( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return !d.actor( is_npc )->has_effect( effect_currently_busy, bodypart_str_id::NULL_ID() );
    };
}

conditional_t::func f_npc_following( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_following();
    };
}

conditional_t::func f_npc_friend( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_friendly( get_player_character() );
    };
}

conditional_t::func f_npc_hostile( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_enemy();
    };
}

conditional_t::func f_npc_train_skills( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return !d.actor( is_npc )->skills_offered_to( *d.actor( !is_npc ) ).empty();
    };
}

conditional_t::func f_npc_train_styles( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return !d.actor( is_npc )->styles_offered_to( *d.actor( !is_npc ) ).empty();
    };
}

conditional_t::func f_npc_train_spells( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return !d.actor( is_npc )->spells_offered_to( *d.actor( !is_npc ) ).empty();
    };
}

conditional_t::func f_at_safe_space( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return overmap_buffer.is_safe( d.actor( is_npc )->global_omt_location() ) &&
               d.actor( is_npc )->is_safe();
    };
}

conditional_t::func f_can_stow_weapon( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        return !actor->unarmed_attack() && actor->can_stash_weapon();
    };
}

conditional_t::func f_can_drop_weapon( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        return !actor->unarmed_attack() && !actor->wielded_with_flag( flag_NO_UNWIELD );
    };
}

conditional_t::func f_has_weapon( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return !d.actor( is_npc )->unarmed_attack();
    };
}

conditional_t::func f_is_driving( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        if( const optional_vpart_position vp = get_map().veh_at( actor->pos() ) ) {
            return vp->vehicle().is_moving() && actor->is_in_control_of( vp->vehicle() );
        }
        return false;
    };
}

conditional_t::func f_has_stolen_item( bool /*is_npc*/ )
{
    return []( dialogue const & d ) {
        return d.actor( false )->has_stolen_item( *d.actor( true ) );
    };
}

conditional_t::func f_is_day()
{
    return []( dialogue const & ) {
        return !is_night( calendar::turn );
    };
}

conditional_t::func f_is_outside( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return is_creature_outside( *static_cast<talker const *>( d.actor( is_npc ) )->get_creature() );
    };
}

conditional_t::func f_is_underwater( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return get_map().is_divable( d.actor( is_npc )->pos() );
    };
}

conditional_t::func f_one_in_chance( const JsonObject &jo, std::string_view member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [dov]( dialogue & d ) {
        return one_in( dov.evaluate( d ) );
    };
}

conditional_t::func f_query( const JsonObject &jo, std::string_view member, bool is_npc )
{
    translation_or_var message = get_translation_or_var( jo.get_member( member ), member, true );
    bool default_val = jo.get_bool( "default" );
    return [message, default_val, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        if( actor->get_character() && actor->get_character()->is_avatar() ) {
            std::string translated_message = message.evaluate( d );
            return query_yn( translated_message );
        } else {
            return default_val;
        }
    };
}

conditional_t::func f_query_tile( const JsonObject &jo, std::string_view member, bool is_npc )
{
    std::string type = jo.get_string( member.data() );
    var_info target_var = read_var_info( jo.get_object( "target_var" ) );
    std::string message;
    if( jo.has_member( "message" ) ) {
        message = jo.get_string( "message" );
    }
    dbl_or_var range;
    if( jo.has_member( "range" ) ) {
        range = get_dbl_or_var( jo, "range" );
    }
    bool z_level = jo.get_bool( "z_level", false );
    return [type, target_var, message, range, z_level, is_npc]( dialogue & d ) {
        std::optional<tripoint> loc;
        Character *ch = d.actor( is_npc )->get_character();
        if( ch && ch->as_avatar() ) {
            avatar *you = ch->as_avatar();
            if( type == "anywhere" ) {
                if( !message.empty() ) {
                    static_popup popup;
                    popup.on_top( true );
                    popup.message( "%s", message );
                }
                tripoint center = d.actor( is_npc )->pos();
                const look_around_params looka_params = { true, center, center, false, true, true, z_level };
                loc = g->look_around( looka_params ).position;
            } else if( type == "line_of_sight" ) {
                if( !message.empty() ) {
                    static_popup popup;
                    popup.on_top( true );
                    popup.message( "%s", message );
                }
                target_handler::trajectory traj = target_handler::mode_select_only( *you, range.evaluate( d ) );
                if( !traj.empty() ) {
                    loc = traj.back();
                }
            } else if( type == "around" ) {
                if( !message.empty() ) {
                    loc = choose_adjacent( message );
                } else {
                    loc = choose_adjacent( _( "Choose direction" ) );
                }
            } else {
                debugmsg( string_format( "Invalid selection type: %s", type ) );
            }

        }
        if( loc.has_value() ) {
            tripoint_abs_ms pos_global = get_map().getglobal( *loc );
            write_var_value( target_var.type, target_var.name, d.actor( target_var.type == var_type::npc ), &d,
                             pos_global.to_string() );
        }
        return loc.has_value();
    };
}

conditional_t::func f_x_in_y_chance( const JsonObject &jo, const std::string_view member )
{
    const JsonObject &var_obj = jo.get_object( member );
    dbl_or_var dovx = get_dbl_or_var( var_obj, "x" );
    dbl_or_var dovy = get_dbl_or_var( var_obj, "y" );
    return [dovx, dovy]( dialogue & d ) {
        return x_in_y( dovx.evaluate( d ),
                       dovy.evaluate( d ) );
    };
}

conditional_t::func f_is_weather( const JsonObject &jo, std::string_view member )
{
    str_or_var weather = get_str_or_var( jo.get_member( member ), member, true );
    return [weather]( dialogue const & d ) {
        return get_weather().weather_id == weather_type_id( weather.evaluate( d ) );
    };
}

conditional_t::func f_map_ter_furn_with_flag( const JsonObject &jo, std::string_view member )
{
    str_or_var furn_type = get_str_or_var( jo.get_member( member ), member, true );
    var_info loc_var = read_var_info( jo.get_object( "loc" ) );
    bool terrain = true;
    if( member == "map_terrain_with_flag" ) {
        terrain = true;
    } else if( member == "map_furniture_with_flag" ) {
        terrain = false;
    }
    return [terrain, furn_type, loc_var]( dialogue const & d ) {
        tripoint loc = get_map().getlocal( get_tripoint_from_var( loc_var, d ) );
        if( terrain ) {
            return get_map().ter( loc )->has_flag( furn_type.evaluate( d ) );
        } else {
            return get_map().furn( loc )->has_flag( furn_type.evaluate( d ) );
        }
    };
}

conditional_t::func f_map_in_city( const JsonObject &jo, std::string_view member )
{
    str_or_var target = get_str_or_var( jo.get_member( member ), member, true );
    return [target]( dialogue const & d ) {
        tripoint_abs_ms target_pos = tripoint_abs_ms( tripoint::from_string( target.evaluate( d ) ) );
        city_reference c = overmap_buffer.closest_city( project_to<coords::sm>( target_pos ) );
        c.distance = rl_dist( c.abs_sm_pos, project_to<coords::sm>( target_pos ) );
        return c && c.get_distance_from_bounds() <= 0;
    };
}

conditional_t::func f_mod_is_loaded( const JsonObject &jo, std::string_view member )
{
    str_or_var compared_mod = get_str_or_var( jo.get_member( member ), member, true );
    return [compared_mod]( dialogue const & d ) {
        mod_id comp_mod = mod_id( compared_mod.evaluate( d ) );
        for( const mod_id &mod : world_generator->active_world->active_mod_order ) {
            if( comp_mod == mod ) {
                return true;
            }
        }
        return false;
    };
}

conditional_t::func f_has_faction_trust( const JsonObject &jo, std::string_view member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    return [dov]( dialogue & d ) {
        return d.actor( true )->get_faction()->trusts_u >= dov.evaluate( d );
    };
}

conditional_t::func f_compare_string( const JsonObject &jo, std::string_view member )
{
    str_or_var first;
    str_or_var second;
    JsonArray objects = jo.get_array( member );
    if( objects.size() != 2 ) {
        jo.throw_error( "incorrect number of values.  Expected 2 in " + jo.str() );
        return []( dialogue const & ) {
            return false;
        };
    }

    if( objects.has_object( 0 ) ) {
        first = get_str_or_var( objects.next_value(), member, true );
    } else {
        first.str_val = objects.next_string();
    }
    if( objects.has_object( 1 ) ) {
        second = get_str_or_var( objects.next_value(), member, true );
    } else {
        second.str_val = objects.next_string();
    }

    return [first, second]( dialogue const & d ) {
        return first.evaluate( d ) == second.evaluate( d );
    };
}

conditional_t::func f_get_condition( const JsonObject &jo, std::string_view member )
{
    str_or_var conditionalToGet = get_str_or_var( jo.get_member( member ), member, true );
    return [conditionalToGet]( dialogue & d ) {
        return d.evaluate_conditional( conditionalToGet.evaluate( d ), d );
    };
}

conditional_t::func f_has_ammo()
{
    return []( dialogue & d ) {
        item_location *it = d.actor( true )->get_item();
        if( it ) {
            return ( *it )->ammo_sufficient( d.actor( false )->get_character() );
        } else {
            debugmsg( "beta talker must be Item" );
            return false;
        }
    };
}

conditional_t::func f_compare_num( const JsonObject &jo, const std::string_view member )
{
    JsonArray objects = jo.get_array( member );
    if( objects.size() != 3 ) {
        jo.throw_error( "incorrect number of values.  Expected three in " + jo.str() );
        return []( dialogue const & ) {
            return false;
        };
    }
    std::function<double( dialogue & )> get_first_dbl = objects.has_object(
                0 ) ? conditional_t::get_get_dbl(
                objects.get_object( 0 ) ) : conditional_t::get_get_dbl( objects.get_string( 0 ), jo );
    std::function<double( dialogue & )> get_second_dbl = objects.has_object(
                2 ) ? conditional_t::get_get_dbl(
                objects.get_object( 2 ) ) : conditional_t::get_get_dbl( objects.get_string( 2 ), jo );
    const std::string &op = objects.get_string( 1 );

    if( op == "==" || op == "=" ) {
        return [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) == get_second_dbl( d );
        };
    } else if( op == "!=" ) {
        return [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) != get_second_dbl( d );
        };
    } else if( op == "<=" ) {
        return [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) <= get_second_dbl( d );
        };
    } else if( op == ">=" ) {
        return [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) >= get_second_dbl( d );
        };
    } else if( op == "<" ) {
        return [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) < get_second_dbl( d );
        };
    } else if( op == ">" ) {
        return [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) > get_second_dbl( d );
        };
    } else {
        jo.throw_error( "unexpected operator " + jo.get_string( "op" ) + " in " + jo.str() );
        return []( dialogue const & ) {
            return false;
        };
    }
}

conditional_t::func f_math( const JsonObject &jo, const std::string_view member )
{
    eoc_math math;
    math.from_json( jo, member, eoc_math::type_t::compare );
    return [math = std::move( math )]( dialogue & d ) {
        return math.act( d );
    };
}

conditional_t::func f_u_has_camp()
{
    return []( dialogue const & ) {
        return !get_player_character().camps.empty();
    };
}

conditional_t::func f_has_pickup_list( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "pickup_rule", "any" );
    };
}

conditional_t::func f_is_by_radio()
{
    return []( dialogue const & d ) {
        return d.by_radio;
    };
}

conditional_t::func f_has_reason()
{
    return []( dialogue const & d ) {
        return !d.reason.empty();
    };
}

conditional_t::func f_roll_contested( const JsonObject &jo, const std::string_view member )
{
    std::function<double( dialogue & )> get_check = conditional_t::get_get_dbl( jo.get_object(
                member ) );
    dbl_or_var difficulty = get_dbl_or_var( jo, "difficulty", true );
    dbl_or_var die_size = get_dbl_or_var( jo, "die_size", false, 10 );
    return [get_check, difficulty, die_size]( dialogue & d ) {
        return rng( 1, die_size.evaluate( d ) ) + get_check( d ) > difficulty.evaluate( d );
    };
}

conditional_t::func f_u_know_recipe( const JsonObject &jo, std::string_view member )
{
    str_or_var known_recipe_id = get_str_or_var( jo.get_member( member ), member, true );
    return [known_recipe_id]( dialogue & d ) {
        const recipe &rep = recipe_id( known_recipe_id.evaluate( d ) ).obj();
        // should be a talker function but recipes aren't in Character:: yet
        return get_player_character().knows_recipe( &rep );
    };
}

conditional_t::func f_mission_has_generic_rewards()
{
    return []( dialogue const & d ) {
        mission *miss = d.actor( true )->selected_mission();
        if( miss == nullptr ) {
            debugmsg( "mission_has_generic_rewards: mission_selected == nullptr" );
            return true;
        }
        return miss->has_generic_rewards();
    };
}

conditional_t::func f_has_worn_with_flag( const JsonObject &jo, std::string_view member,
        bool is_npc )
{
    str_or_var flag = get_str_or_var( jo.get_member( member ), member, true );
    std::optional<bodypart_id> bp;
    optional( jo, false, "bodypart", bp );
    return [flag, bp, is_npc]( dialogue const & d ) {
        bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
        return d.actor( is_npc )->worn_with_flag( flag_id( flag.evaluate( d ) ), bid );
    };
}

conditional_t::func f_has_wielded_with_flag( const JsonObject &jo, std::string_view member,
        bool is_npc )
{
    str_or_var flag = get_str_or_var( jo.get_member( member ), member, true );
    return [flag, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->wielded_with_flag( flag_id( flag.evaluate( d ) ) );
    };
}

conditional_t::func f_has_wielded_with_weapon_category( const JsonObject &jo,
        std::string_view member,
        bool is_npc )
{
    str_or_var w_cat = get_str_or_var( jo.get_member( member ), member, true );
    return [w_cat, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->wielded_with_weapon_category( weapon_category_id( w_cat.evaluate( d ) ) );
    };
}

conditional_t::func f_can_see( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->can_see();
    };
}

conditional_t::func f_is_deaf( bool is_npc )
{
    return [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_deaf();
    };
}

conditional_t::func f_is_on_terrain( const JsonObject &jo, std::string_view member,
                                     bool is_npc )
{
    str_or_var terrain_type = get_str_or_var( jo.get_member( member ), member, true );
    return [terrain_type, is_npc]( dialogue const & d ) {
        map &here = get_map();
        return here.ter( d.actor( is_npc )->pos() ) == ter_id( terrain_type.evaluate( d ) );
    };
}

conditional_t::func f_is_on_terrain_with_flag( const JsonObject &jo, std::string_view member,
        bool is_npc )
{
    str_or_var terrain_type = get_str_or_var( jo.get_member( member ), member, true );
    return [terrain_type, is_npc]( dialogue const & d ) {
        map &here = get_map();
        return here.ter( d.actor( is_npc )->pos() )->has_flag( terrain_type.evaluate( d ) );
    };
}

conditional_t::func f_is_in_field( const JsonObject &jo, std::string_view member,
                                   bool is_npc )
{
    str_or_var field_type = get_str_or_var( jo.get_member( member ), member, true );
    return [field_type, is_npc]( dialogue const & d ) {
        map &here = get_map();
        field_type_id ft = field_type_id( field_type.evaluate( d ) );
        for( const std::pair<const field_type_id, field_entry> &f : here.field_at( d.actor(
                    is_npc )->pos() ) ) {
            if( f.second.get_field_type() == ft ) {
                return true;
            }
        }
        return false;
    };
}

conditional_t::func f_has_move_mode( const JsonObject &jo, std::string_view member,
                                     bool is_npc )
{
    str_or_var mode = get_str_or_var( jo.get_member( member ), member, true );
    return [mode, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->get_move_mode() == move_mode_id( mode.evaluate( d ) );
    };
}

conditional_t::func f_can_see_location( const JsonObject &jo, std::string_view member,
                                        bool is_npc )
{
    str_or_var target = get_str_or_var( jo.get_member( member ), member, true );
    return [is_npc, target]( dialogue const & d ) {
        tripoint_abs_ms target_pos = tripoint_abs_ms( tripoint::from_string( target.evaluate( d ) ) );
        return d.actor( is_npc )->can_see_location( get_map().getlocal( target_pos ) );
    };
}

conditional_t::func f_using_martial_art( const JsonObject &jo, std::string_view member,
        bool is_npc )
{
    str_or_var style_to_check = get_str_or_var( jo.get_member( member ), member, true );
    return [style_to_check, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->using_martial_art( matype_id( style_to_check.evaluate( d ) ) );
    };
}

} // namespace
} // namespace conditional_fun

template<class T>
static std::function<T( const dialogue & )> get_get_str_( const JsonObject &jo,
        std::function<T( const std::string & )> ret_func )
{
    if( jo.get_string( "mutator" ) == "mon_faction" ) {
        str_or_var mtypeid = get_str_or_var( jo.get_member( "mtype_id" ), "mtype_id" );
        return [mtypeid, ret_func]( const dialogue & d ) {
            return ret_func( ( static_cast<mtype_id>( mtypeid.evaluate( d ) ) )->default_faction.str() );
        };
    } else if( jo.get_string( "mutator" ) == "game_option" ) {
        str_or_var option = get_str_or_var( jo.get_member( "option" ), "option" );
        return [option, ret_func]( const dialogue & d ) {
            return ret_func( get_option<std::string>( option.evaluate( d ) ) );
        };
    } else if( jo.get_string( "mutator" ) == "valid_technique" ) {
        std::vector<str_or_var> blacklist;
        if( jo.has_array( "blacklist" ) ) {
            for( const JsonValue &jv : jo.get_array( "blacklist" ) ) {
                blacklist.push_back( get_str_or_var( jv, "blacklist" ) );
            }
        }

        bool crit = jo.get_bool( "crit", false );
        bool dodge_counter = jo.get_bool( "dodge_counter", false );
        bool block_counter = jo.get_bool( "block_counter", false );

        return [blacklist, crit, dodge_counter, block_counter, ret_func]( const dialogue & d ) {
            std::vector<matec_id> bl;
            bl.reserve( blacklist.size() );
            for( const str_or_var &sv : blacklist ) {
                bl.emplace_back( sv.evaluate( d ) );
            }
            return ret_func( d.actor( false )->get_random_technique( *d.actor( true )->get_creature(),
                             crit, dodge_counter, block_counter, bl ).str() );
        };
    } else if( jo.get_string( "mutator" ) == "loc_relative_u" ) {
        str_or_var target = get_str_or_var( jo.get_member( "target" ), "target" );
        return [target, ret_func]( const dialogue & d ) {
            tripoint_abs_ms char_pos = get_map().getglobal( d.actor( false )->pos() );
            tripoint_abs_ms target_pos = char_pos + tripoint::from_string( target.evaluate( d ) );
            return ret_func( target_pos.to_string() );
        };
    } else if( jo.get_string( "mutator" ) == "topic_item" ) {
        return [ret_func]( const dialogue & d ) {
            return ret_func( d.cur_item.str() );
        };
    }

    return nullptr;
}

template<class T>
static std::function<T( const dialogue & )> get_get_translation_( const JsonObject &jo,
        std::function<T( const translation & )> ret_func )
{
    if( jo.get_string( "mutator" ) == "ma_technique_description" ) {
        str_or_var ma = get_str_or_var( jo.get_member( "matec_id" ), "matec_id" );

        return [ma, ret_func]( const dialogue & d ) {
            return ret_func( matec_id( ma.evaluate( d ) )->description );
        };
    } else if( jo.get_string( "mutator" ) == "ma_technique_name" ) {
        str_or_var ma = get_str_or_var( jo.get_member( "matec_id" ), "matec_id" );

        return [ma, ret_func]( const dialogue & d ) {
            return ret_func( matec_id( ma.evaluate( d ) )->name );
        };
    }

    return nullptr;
}

std::function<translation( const dialogue & )> conditional_t::get_get_translation(
    const JsonObject &jo )
{
    auto ret_func = get_get_str_<translation>( jo, []( const std::string & s ) {
        return no_translation( s );
    } );

    if( !ret_func ) {
        ret_func = get_get_translation_<translation>( jo, []( const translation & t ) {
            return t;
        } );
        if( !ret_func ) {
            jo.throw_error( "unrecognized string mutator in " + jo.str() );
            return []( const dialogue & ) {
                return translation();
            };
        }
    }

    return ret_func;
}

std::function<std::string( const dialogue & )> conditional_t::get_get_string( const JsonObject &jo )
{
    auto ret_func = get_get_str_<std::string>( jo, []( const std::string & s ) {
        return s;
    } );

    if( !ret_func ) {
        ret_func = get_get_translation_<std::string>( jo, []( const translation & t ) {
            return t.translated();
        } );
        if( !ret_func ) {
            jo.throw_error( "unrecognized string mutator in " + jo.str() );
            return []( const dialogue & ) {
                return "INVALID";
            };
        }
    }

    return ret_func;
}

template<class J>
// NOLINTNEXTLINE(readability-function-cognitive-complexity): not my problem!!
std::function<double( dialogue & )> conditional_t::get_get_dbl( J const &jo )
{
    if( jo.has_member( "const" ) ) {
        const double const_value = jo.get_float( "const" );
        return [const_value]( dialogue const & ) {
            return const_value;
        };
    } else if( jo.has_member( "rand" ) ) {
        int max_value = jo.get_int( "rand" );
        return [max_value]( dialogue const & ) {
            return rng( 0, max_value );
        };
    } else if( jo.has_member( "faction_trust" ) ) {
        str_or_var name = get_str_or_var( jo.get_member( "faction_trust" ), "faction_trust" );
        return [name]( dialogue const & d ) {
            faction *fac = g->faction_manager_ptr->get( faction_id( name.evaluate( d ) ) );
            return fac->trusts_u;
        };
    } else if( jo.has_member( "faction_like" ) ) {
        str_or_var name = get_str_or_var( jo.get_member( "faction_like" ), "faction_like" );
        return [name]( dialogue const & d ) {
            faction *fac = g->faction_manager_ptr->get( faction_id( name.evaluate( d ) ) );
            return fac->likes_u;
        };
    } else if( jo.has_member( "faction_respect" ) ) {
        str_or_var name = get_str_or_var( jo.get_member( "faction_respect" ), "faction_respect" );
        return [name]( dialogue const & d ) {
            faction *fac = g->faction_manager_ptr->get( faction_id( name.evaluate( d ) ) );
            return fac->respects_u;
        };
    } else if( jo.has_member( "u_val" ) || jo.has_member( "npc_val" ) ||
               jo.has_member( "global_val" ) || jo.has_member( "context_val" ) ) {
        const bool is_npc = jo.has_member( "npc_val" );
        const bool is_global = jo.has_member( "global_val" );
        const bool is_context = jo.has_member( "context_val" );
        const std::string checked_value = is_npc ? jo.get_string( "npc_val" ) :
                                          ( is_global ? jo.get_string( "global_val" ) : ( is_context ? jo.get_string( "context_val" ) :
                                                  jo.get_string( "u_val" ) ) );
        if( checked_value == "strength" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->str_cur();
            };
        } else if( checked_value == "dexterity" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->dex_cur();
            };
        } else if( checked_value == "intelligence" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->int_cur();
            };
        } else if( checked_value == "perception" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->per_cur();
            };
        } else if( checked_value == "strength_base" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_str_max();
            };
        } else if( checked_value == "dexterity_base" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_dex_max();
            };
        } else if( checked_value == "intelligence_base" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_int_max();
            };
        } else if( checked_value == "perception_base" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_per_max();
            };
        } else if( checked_value == "strength_bonus" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_str_bonus();
            };
        } else if( checked_value == "dexterity_bonus" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_dex_bonus();
            };
        } else if( checked_value == "intelligence_bonus" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_int_bonus();
            };
        } else if( checked_value == "perception_bonus" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_per_bonus();
            };
        } else if( checked_value == "warmth" ) {
            std::optional<bodypart_id> bp;
            if constexpr( std::is_same_v<JsonObject, J> ) {
                optional( jo, false, "bodypart", bp );
            }
            return [is_npc, bp]( dialogue const & d ) {
                bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
                return units::to_legacy_bodypart_temp( d.actor( is_npc )->get_cur_part_temp( bid ) );
            };
        } else if( checked_value == "dodge" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_character()->get_dodge();
            };
        } else if( checked_value == "effect_intensity" ) {
            const std::string &effect_id = jo.get_string( "effect" );
            std::optional<bodypart_id> bp;
            if constexpr( std::is_same_v<JsonObject, J> ) {
                optional( jo, false, "bodypart", bp );
            }
            return [effect_id, bp, is_npc]( dialogue const & d ) {
                bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
                effect target = d.actor( is_npc )->get_effect( efftype_id( effect_id ), bid );
                return target.is_null() ? -1 : target.get_intensity();
            };
        } else if( checked_value == "var" ) {
            var_info info( {}, {} );
            if constexpr( std::is_same_v<JsonObject, J> ) {
                info = read_var_info( jo );
            }
            return [info]( dialogue const & d ) {
                std::string var = read_var_value( info, d );
                if( !var.empty() ) {
                    // NOLINTNEXTLINE(cert-err34-c)
                    return std::atof( var.c_str() );
                } else if( !info.default_val.empty() ) {
                    // NOLINTNEXTLINE(cert-err34-c)
                    return std::atof( info.default_val.c_str() );
                }
                return 0.0;
            };
        } else if( checked_value == "allies" ) {
            if( is_npc ) {
                jo.throw_error( "allies count not supported for NPCs.  In " + jo.str() );
            } else {
                return []( dialogue const & ) {
                    return static_cast<int>( g->allies().size() );
                };
            }
        } else if( checked_value == "cash" ) {
            if( is_npc ) {
                jo.throw_error( "cash count not supported for NPCs.  In " + jo.str() );
            } else {
                return [is_npc]( dialogue const & d ) {
                    return d.actor( is_npc )->cash();
                };
            }
        } else if( checked_value == "owed" ) {
            if( is_npc ) {
                jo.throw_error( "owed amount not supported for NPCs.  In " + jo.str() );
            } else {
                return []( dialogue const & d ) {
                    return d.actor( true )->debt();
                };
            }
        } else if( checked_value == "sold" ) {
            if( is_npc ) {
                jo.throw_error( "owed amount not supported for NPCs.  In " + jo.str() );
            } else {
                return []( dialogue const & d ) {
                    return d.actor( true )->sold();
                };
            }
        } else if( checked_value == "pos_x" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->posx();
            };
        } else if( checked_value == "pos_y" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->posy();
            };
        } else if( checked_value == "pos_z" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->posz();
            };
        } else if( checked_value == "power" ) {
            return [is_npc]( dialogue const & d ) {
                // Energy in milijoule
                return static_cast<int>( d.actor( is_npc )->power_cur().value() );
            };
        } else if( checked_value == "power_max" ) {
            return [is_npc]( dialogue const & d ) {
                // Energy in milijoule
                return static_cast<int>( d.actor( is_npc )->power_max().value() );
            };
        } else if( checked_value == "power_percentage" ) {
            return [is_npc]( dialogue const & d ) {
                // Energy in milijoule
                int power_max = d.actor( is_npc )->power_max().value();
                if( power_max == 0 ) {
                    return 0; //Default value if character does not have power, avoids division with 0.
                } else {
                    return static_cast<int>( d.actor( is_npc )->power_cur().value() * 100 ) / power_max;
                }
            };
        } else if( checked_value == "morale" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->morale_cur();
            };
        } else if( checked_value == "focus" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->focus_cur();
            };
        } else if( checked_value == "mana" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->mana_cur();
            };
        } else if( checked_value == "mana_max" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->mana_max();
            };
        } else if( checked_value == "mana_percentage" ) {
            return [is_npc]( dialogue const & d ) {
                int mana_max = d.actor( is_npc )->mana_max();
                if( mana_max == 0 ) {
                    return 0; //Default value if character does not have mana, avoids division with 0.
                } else {
                    return ( d.actor( is_npc )->mana_cur() * 100 ) / mana_max;
                }
            };
        } else if( checked_value == "hunger" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_hunger();
            };
        } else if( checked_value == "thirst" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_thirst();
            };
        } else if( checked_value == "instant_thirst" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_instant_thirst();
            };
        } else if( checked_value == "stored_kcal" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_stored_kcal();
            };
        } else if( checked_value == "stored_kcal_percentage" ) {
            // 100% is 5 BMI's worth of kcal, which is considered healthy (this varies with height).
            return [is_npc]( dialogue const & d ) {
                int divisor = d.actor( is_npc )->get_healthy_kcal() / 100;
                //if no data default to default height of 175cm
                if( divisor == 0 ) {
                    divisor = 118169 / 100;
                }
                return d.actor( is_npc )->get_stored_kcal() / divisor;
            };
        } else if( checked_value == "item_count" ) {
            const itype_id item_id( jo.get_string( "item" ) );
            return [is_npc, item_id]( dialogue const & d ) {
                return d.actor( is_npc )->get_amount( item_id );
            };
        } else if( checked_value == "charge_count" ) {
            const itype_id item_id( jo.get_string( "item" ) );
            return [is_npc, item_id]( dialogue const & d ) {
                return d.actor( is_npc )->charges_of( item_id );
            };
        } else if( checked_value == "exp" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_kill_xp();
            };
        } else if( checked_value == "addiction_intensity" ) {
            const addiction_id add_id( jo.get_string( "addiction" ) );
            if( jo.has_object( "mod" ) ) {
                // final_value = (val / (val - step * intensity)) - 1
                JsonObject jobj = jo.get_object( "mod" );
                const int val = jobj.get_int( "val", 0 );
                const int step = jobj.get_int( "step", 0 );
                return [is_npc, add_id, val, step]( dialogue const & d ) {
                    int intens = d.actor( is_npc )->get_addiction_intensity( add_id );
                    int denom = val - step * intens;
                    return denom == 0 ? 0 : ( val / std::max( 1, denom ) - 1 );
                };
            }
            const int mod = jo.get_int( "mod", 1 );
            return [is_npc, add_id, mod]( dialogue const & d ) {
                return d.actor( is_npc )->get_addiction_intensity( add_id ) * mod;
            };
        } else if( checked_value == "addiction_turns" ) {
            const addiction_id add_id( jo.get_string( "addiction" ) );
            return [is_npc, add_id]( dialogue const & d ) {
                return d.actor( is_npc )->get_addiction_turns( add_id );
            };
        } else if( checked_value == "stim" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_stim();
            };
        } else if( checked_value == "pkill" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_pkill();
            };
        } else if( checked_value == "rad" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_rad();
            };
        } else if( checked_value == "focus" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->focus_cur();
            };
        } else if( checked_value == "activity_level" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_activity_level();
            };
        } else if( checked_value == "fatigue" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_fatigue();
            };
        } else if( checked_value == "stamina" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_stamina();
            };
        } else if( checked_value == "sleep_deprivation" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_sleep_deprivation();
            };
        } else if( checked_value == "anger" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_anger();
            };
        } else if( checked_value == "friendly" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_friendly();
            };
        } else if( checked_value == "vitamin" ) {
            std::string vitamin_name = jo.get_string( "name" );
            return [is_npc, vitamin_name]( dialogue const & d ) {
                Character const *you = static_cast<talker const *>( d.actor( is_npc ) )->get_character();
                if( you ) {
                    return you->vitamin_get( vitamin_id( vitamin_name ) );
                } else {
                    return 0;
                }
            };
        } else if( checked_value == "age" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_age();
            };
        } else if( checked_value == "height" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_height();
            };
        } else if( checked_value == "bmi_permil" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_bmi_permil();
            };
        } else if( checked_value == "size" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_size();
            };
        } else if( checked_value == "volume" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_volume();
            };
        } else if( checked_value == "weight" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_weight();
            };
        } else if( checked_value == "grab_strength" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_grab_strength();
            };
        } else if( checked_value == "fine_detail_vision_mod" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_fine_detail_vision_mod();
            };
        } else if( checked_value == "health" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_health();
            };
        } else if( checked_value == "body_temp" ) {
            return [is_npc]( dialogue const & d ) {
                return units::to_legacy_bodypart_temp( d.actor( is_npc )->get_body_temp() );
            };
        } else if( checked_value == "body_temp_delta" ) {
            return [is_npc]( dialogue const & d ) {
                return units::to_legacy_bodypart_temp_delta( d.actor( is_npc )->get_body_temp_delta() );
            };
        } else if( checked_value == "npc_trust" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_npc_trust();
            };
        } else if( checked_value == "npc_fear" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_npc_fear();
            };
        } else if( checked_value == "npc_value" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_npc_value();
            };
        } else if( checked_value == "npc_anger" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_npc_anger();
            };
        } else if( checked_value == "field_strength" ) {
            if( jo.has_member( "field" ) ) {
                field_type_id ft = field_type_id( jo.get_string( "field" ) );
                return [is_npc, ft]( dialogue const & d ) {
                    map &here = get_map();
                    for( const std::pair<const field_type_id, field_entry> &f : here.field_at( d.actor(
                                is_npc )->pos() ) ) {
                        if( f.second.get_field_type() == ft ) {
                            return f.second.get_field_intensity();
                        }
                    }
                    return 0;
                };
            }
        } else if( checked_value == "spell_level" ) {
            if( jo.has_member( "school" ) ) {
                const std::string school_name = jo.get_string( "school" );
                const trait_id spell_school( school_name );
                return [is_npc, spell_school]( dialogue & d ) {
                    return d.actor( is_npc )->get_spell_level( spell_school );
                };
            } else if( jo.has_member( "spell" ) ) {
                const std::string spell_name = jo.get_string( "spell" );
                const spell_id this_spell_id( spell_name );
                return [is_npc, this_spell_id]( dialogue & d ) {
                    return d.actor( is_npc )->get_spell_level( this_spell_id );
                };
            } else {
                return [is_npc]( dialogue & d ) {
                    return d.actor( is_npc )->get_highest_spell_level();
                };
            }
        } else if( checked_value == "spell_level_adjustment" ) {
            if( jo.has_member( "school" ) ) {
                const std::string school_name = jo.get_string( "school" );
                const trait_id spell_school( school_name );
                return [is_npc, spell_school]( dialogue & d ) {
                    std::map<trait_id, double>::iterator it =
                        d.actor( is_npc )->get_character()->magic->caster_level_adjustment_by_school.find( spell_school );
                    if( it != d.actor( is_npc )->get_character()->magic->caster_level_adjustment_by_school.end() ) {
                        return it->second;
                    } else {
                        return 0.0;
                    }
                };
            } else if( jo.has_member( "spell" ) ) {
                const std::string spell_name = jo.get_string( "spell" );
                const spell_id this_spell_id( spell_name );
                return [is_npc, this_spell_id]( dialogue & d ) {
                    std::map<spell_id, double>::iterator it =
                        d.actor( is_npc )->get_character()->magic->caster_level_adjustment_by_spell.find( this_spell_id );
                    if( it != d.actor( is_npc )->get_character()->magic->caster_level_adjustment_by_spell.end() ) {
                        return it->second;
                    } else {
                        return 0.0;
                    }
                };
            } else {
                return [is_npc]( dialogue & d ) {
                    return d.actor( is_npc )->get_character()->magic->caster_level_adjustment;
                };
            }
        } else if( checked_value == "spell_exp" ) {
            const std::string spell_name = jo.get_string( "spell" );
            const spell_id this_spell_id( spell_name );
            return [is_npc, this_spell_id]( dialogue & d ) {
                return d.actor( is_npc )->get_spell_exp( this_spell_id );
            };
        } else if( checked_value == "spell_count" ) {
            trait_id school = trait_id::NULL_ID();
            if( jo.has_member( "school" ) ) {
                school = trait_id( jo.get_string( "school" ) );
            }
            return [is_npc, school]( dialogue & d ) {
                return d.actor( is_npc )->get_spell_count( school );
            };
        } else if( checked_value == "proficiency" ) {
            const std::string proficiency_name = jo.get_string( "proficiency_id" );
            const proficiency_id the_proficiency_id( proficiency_name );
            if( jo.has_int( "format" ) ) {
                const int format = jo.get_int( "format" );
                return [is_npc, format, the_proficiency_id]( dialogue & d ) {
                    return static_cast<int>( ( d.actor( is_npc )->proficiency_practiced_time(
                                                   the_proficiency_id ) * format ) /
                                             the_proficiency_id->time_to_learn() );
                };
            } else if( jo.has_member( "format" ) ) {
                const std::string format = jo.get_string( "format" );
                if( format == "time_spent" ) {
                    return [is_npc, the_proficiency_id]( dialogue & d ) {
                        return to_turns<int>( d.actor( is_npc )->proficiency_practiced_time( the_proficiency_id ) );
                    };
                } else if( format == "percent" ) {
                    return [is_npc, the_proficiency_id]( dialogue & d ) {
                        return static_cast<int>( ( d.actor( is_npc )->proficiency_practiced_time(
                                                       the_proficiency_id ) * 100 ) /
                                                 the_proficiency_id->time_to_learn() );
                    };
                } else if( format == "permille" ) {
                    return [is_npc, the_proficiency_id]( dialogue & d ) {
                        return static_cast<int>( ( d.actor( is_npc )->proficiency_practiced_time(
                                                       the_proficiency_id ) * 1000 ) /
                                                 the_proficiency_id->time_to_learn() );
                    };
                } else if( format == "total_time_required" ) {
                    return [the_proficiency_id]( dialogue & d ) {
                        static_cast<void>( d );
                        return to_turns<int>( the_proficiency_id->time_to_learn() );
                    };
                } else if( format == "time_left" ) {
                    return [is_npc, the_proficiency_id]( dialogue & d ) {
                        return to_turns<int>( the_proficiency_id->time_to_learn() - d.actor(
                                                  is_npc )->proficiency_practiced_time( the_proficiency_id ) );
                    };
                } else {
                    jo.throw_error( "unrecognized format in " + jo.str() );
                }
            }
        }
    } else if( jo.has_array( "distance" ) ) {
        JsonArray objects = jo.get_array( "distance" );
        if( objects.size() != 2 ) {
            objects.throw_error( "distance requires an array with 2 elements." );
        }
        std::string first = get_string_from_input( objects, 0 );
        std::string second = get_string_from_input( objects, 1 );
        return [first, second]( dialogue & d ) {
            tripoint_abs_ms first_point = get_tripoint_from_string( first, d );
            tripoint_abs_ms second_point = get_tripoint_from_string( second, d );
            return rl_dist( first_point, second_point );
        };
    } else if( jo.has_member( "mod_load_order" ) ) {
        const mod_id our_mod_id = mod_id( jo.get_string( "mod_load_order" ) );
        return [our_mod_id]( dialogue const & ) {
            int count = 0;
            for( const mod_id &mod : world_generator->active_world->active_mod_order ) {
                if( our_mod_id == mod ) {
                    return count;
                }
                count++;
            }
            return -1;
        };
    } else if( jo.has_array( "arithmetic" ) ) {
        talk_effect_fun_t arith;
        if constexpr( std::is_same_v<JsonObject, J> ) {
            arith = talk_effect_fun_t::from_arithmetic( jo, "arithmetic", true );
        }
        return [arith]( dialogue & d ) {
            arith( d );
            var_info info = var_info( var_type::global, "temp_var" );
            std::string val = read_var_value( info, d );
            if( !val.empty() ) {
                return std::stof( val );
            } else {
                debugmsg( "No valid value." );
                return 0.0f;
            }
        };
    } else if( jo.has_array( "math" ) ) {
        // no recursive math through shim
        if constexpr( std::is_same_v<JsonObject, J> ) {
            eoc_math math;
            math.from_json( jo, "math", eoc_math::type_t::ret );
            return [math = std::move( math )]( dialogue & d ) {
                return math.act( d );
            };
        }
    }
    jo.throw_error( "unrecognized number source in " + jo.str() );
    return []( dialogue const & ) {
        return 0.0;
    };
}

std::function<double( dialogue & )> conditional_t::get_get_dbl[[noreturn]](
    const std::string &value,
    const JsonObject &jo )
{
    jo.throw_error( "unrecognized number source in " + value );
}

static double handle_min_max( dialogue &d, double input, std::optional<dbl_or_var_part> min,
                              std::optional<dbl_or_var_part> max )
{
    if( min.has_value() ) {
        double min_val = min.value().evaluate( d );
        input = std::max( min_val, input );
    }
    if( max.has_value() ) {
        double max_val = max.value().evaluate( d );
        input = std::min( max_val, input );
    }
    return input;
}

template <class J>
std::function<void( dialogue &, double )>
// NOLINTNEXTLINE(readability-function-cognitive-complexity): not my problem!!
conditional_t::get_set_dbl( const J &jo, const std::optional<dbl_or_var_part> &min,
                            const std::optional<dbl_or_var_part> &max, bool temp_var )
{
    if( temp_var ) {
        jo.allow_omitted_members();
        return [min, max]( dialogue & d, double input ) {
            write_var_value( var_type::global, "temp_var", d.actor( false ), &d,
                             handle_min_max( d, input, min, max ) );
        };
    } else if( jo.has_member( "const" ) ) {
        jo.throw_error( "attempted to alter a constant value in " + jo.str() );
    } else if( jo.has_member( "rand" ) ) {
        jo.throw_error( "can not alter the random number generator, silly!  In " + jo.str() );
    } else if( jo.has_member( "faction_trust" ) ) {
        str_or_var name = get_str_or_var( jo.get_member( "faction_trust" ), "faction_trust" );
        return [name, min, max]( dialogue & d, double input ) {
            faction *fac = g->faction_manager_ptr->get( faction_id( name.evaluate( d ) ) );
            fac->trusts_u = handle_min_max( d, input, min, max );
        };
    } else if( jo.has_member( "faction_like" ) ) {
        str_or_var name = get_str_or_var( jo.get_member( "faction_like" ), "faction_like" );
        return [name, min, max]( dialogue & d, double input ) {
            faction *fac = g->faction_manager_ptr->get( faction_id( name.evaluate( d ) ) );
            fac->likes_u = handle_min_max( d, input, min, max );
        };
    } else if( jo.has_member( "faction_respect" ) ) {
        str_or_var name = get_str_or_var( jo.get_member( "faction_respect" ), "faction_respect" );
        return [name, min, max]( dialogue & d, double input ) {
            faction *fac = g->faction_manager_ptr->get( faction_id( name.evaluate( d ) ) );
            fac->respects_u = handle_min_max( d, input, min, max );
        };
    } else if( jo.has_member( "u_val" ) || jo.has_member( "npc_val" ) ||
               jo.has_member( "global_val" ) || jo.has_member( "faction_val" ) || jo.has_member( "party_val" ) ||
               jo.has_member( "context_val" ) ) {
        var_type type = var_type::u;
        std::string checked_value;
        if( jo.has_member( "u_val" ) ) {
            type = var_type::u;
            checked_value = jo.get_string( "u_val" );
        } else if( jo.has_member( "npc_val" ) ) {
            type = var_type::npc;
            checked_value = jo.get_string( "npc_val" );
        } else if( jo.has_member( "global_val" ) ) {
            type = var_type::global;
            checked_value = jo.get_string( "global_val" );
        } else if( jo.has_member( "var_val" ) ) {
            type = var_type::var;
            checked_value = jo.get_string( "var_val" );
        } else if( jo.has_member( "context_val" ) ) {
            type = var_type::context;
            checked_value = jo.get_string( "context_val" );
        } else if( jo.has_member( "faction_val" ) ) {
            type = var_type::faction;
            checked_value = jo.get_string( "faction_val" );
        } else if( jo.has_member( "party_val" ) ) {
            type = var_type::party;
            checked_value = jo.get_string( "party_val" );
        } else {
            jo.throw_error( "Invalid variable type." );
        }

        const bool is_npc = type == var_type::npc;
        if( checked_value == "strength_base" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_str_max( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "dexterity_base" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_dex_max( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "intelligence_base" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_int_max( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "perception_base" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_per_max( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "strength_bonus" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_str_max( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "dexterity_bonus" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_dex_max( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "intelligence_bonus" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_int_max( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "perception_bonus" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_per_max( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "var" ) {
            dbl_or_var empty;
            std::string var_name;
            if constexpr( std::is_same_v<JsonObject, J> ) {
                var_name = get_talk_varname( jo, "var_name", false, empty );
            }
            return [is_npc, var_name, type, min, max]( dialogue & d, double input ) {
                write_var_value( type, var_name, d.actor( is_npc ), &d,
                                 handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "allies" ) {
            // It would be possible to make this work by removing allies and spawning new ones as needed.
            // But why would you ever want to do it this way?
            jo.throw_error( "altering allies this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "cash" ) {
            // TODO: See if this can be handeled in a clever way.
            jo.throw_error( "altering cash this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "owed" ) {
            if( is_npc ) {
                jo.throw_error( "owed amount not supported for NPCs.  In " + jo.str() );
            } else {
                return [min, max]( dialogue & d, double input ) {
                    d.actor( true )->add_debt( handle_min_max( d, input, min, max ) - d.actor( true )->debt() );
                };
            }
        } else if( checked_value == "sold" ) {
            if( is_npc ) {
                jo.throw_error( "sold amount not supported for NPCs.  In " + jo.str() );
            } else {
                return [min, max]( dialogue & d, double input ) {
                    d.actor( true )->add_sold( handle_min_max( d, input, min, max ) - d.actor( true )->sold() );
                };
            }
        } else if( checked_value == "pos_x" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_pos( tripoint( handle_min_max( d, input, min, max ),
                                                      d.actor( is_npc )->posy(),
                                                      d.actor( is_npc )->posz() ) );
            };
        } else if( checked_value == "pos_y" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_pos( tripoint( d.actor( is_npc )->posx(), handle_min_max( d, input, min,
                                                      max ),
                                                      d.actor( is_npc )->posz() ) );
            };
        } else if( checked_value == "pos_z" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_pos( tripoint( d.actor( is_npc )->posx(), d.actor( is_npc )->posy(),
                                                      handle_min_max( d, input, min, max ) ) );
            };
        } else if( checked_value == "power" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                // Energy in milijoule
                d.actor( is_npc )->set_power_cur( 1_mJ * handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "power_max" ) {
            jo.throw_error( "altering max power this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "power_percentage" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                // Energy in milijoule
                d.actor( is_npc )->set_power_cur( ( d.actor( is_npc )->power_max() * handle_min_max( d, input,
                                                    min,
                                                    max ) ) / 100 );
            };
        } else if( checked_value == "focus" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->mod_focus( handle_min_max( d, input, min,
                                              max ) - d.actor( is_npc )->focus_cur() );
            };
        } else if( checked_value == "mana" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_mana_cur( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "mana_max" ) {
            jo.throw_error( "altering max mana this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "mana_percentage" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_mana_cur( ( d.actor( is_npc )->mana_max() * handle_min_max( d, input, min,
                                                   max ) ) / 100 );
            };
        } else if( checked_value == "hunger" ) {
            jo.throw_error( "altering hunger this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "thirst" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_thirst( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "stored_kcal" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_stored_kcal( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "stored_kcal_percentage" ) {
            // 100% is 55'000 kcal, which is considered healthy.
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_stored_kcal( handle_min_max( d, input, min, max ) * 5500 );
            };
        } else if( checked_value == "item_count" ) {
            jo.throw_error( "altering items this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "exp" ) {
            jo.throw_error( "altering max exp this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "addiction_turns" ) {
            const addiction_id add_id( jo.get_string( "addiction" ) );
            return [is_npc, min, max, add_id]( dialogue & d, double input ) {
                d.actor( is_npc )->set_addiction_turns( add_id, handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "stim" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_stim( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "pkill" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_pkill( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "rad" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_rad( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "fatigue" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_fatigue( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "stamina" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_stamina( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "sleep_deprivation" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_sleep_deprivation( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "anger" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_anger( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "morale" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_morale( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "friendly" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_friendly( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "exp" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_kill_xp( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "vitamin" ) {
            std::string vitamin_name = jo.get_string( "name" );
            return [is_npc, min, max, vitamin_name]( dialogue & d, double input ) {
                Character *you = d.actor( is_npc )->get_character();
                if( you ) {
                    you->vitamin_set( vitamin_id( vitamin_name ), handle_min_max( d, input, min, max ) );
                }
            };
        } else if( checked_value == "age" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_age( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "height" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_height( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "npc_trust" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_npc_trust( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "npc_fear" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_npc_fear( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "npc_value" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_npc_value( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "npc_anger" ) {
            return [is_npc, min, max]( dialogue & d, double input ) {
                d.actor( is_npc )->set_npc_anger( handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "spell_level" ) {
            const std::string spell_name = jo.get_string( "spell" );
            const spell_id this_spell_id( spell_name );
            return [is_npc, min, max, this_spell_id]( dialogue & d, double input ) {
                d.actor( is_npc )->set_spell_level( this_spell_id, handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "spell_level_adjustment" ) {
            if( jo.has_member( "school" ) ) {
                const std::string school_name = jo.get_string( "school" );
                const trait_id spell_school( school_name );
                return [is_npc, min, max, spell_school]( dialogue & d, double input ) {
                    std::map<trait_id, double>::iterator it =
                        d.actor( is_npc )->get_character()->magic->caster_level_adjustment_by_school.find( spell_school );
                    if( it != d.actor( is_npc )->get_character()->magic->caster_level_adjustment_by_school.end() ) {
                        it->second = handle_min_max( d, input, min, max );
                    } else {
                        d.actor( is_npc )->get_character()->magic->caster_level_adjustment_by_school.insert( { spell_school, handle_min_max( d, input, min, max ) } );
                    }
                };
            } else if( jo.has_member( "spell" ) ) {
                const std::string spell_name = jo.get_string( "spell" );
                const spell_id this_spell_id( spell_name );
                return [is_npc, min, max, this_spell_id]( dialogue & d, double input ) {
                    std::map<spell_id, double>::iterator it =
                        d.actor( is_npc )->get_character()->magic->caster_level_adjustment_by_spell.find( this_spell_id );
                    if( it != d.actor( is_npc )->get_character()->magic->caster_level_adjustment_by_spell.end() ) {
                        it->second = handle_min_max( d, input, min, max );
                    } else {
                        d.actor( is_npc )->get_character()->magic->caster_level_adjustment_by_spell.insert( { this_spell_id, handle_min_max( d, input, min, max ) } );
                    }
                };
            } else {
                return [is_npc, min, max]( dialogue & d, double input ) {
                    d.actor( is_npc )->get_character()->magic->caster_level_adjustment =
                        handle_min_max( d, input, min, max );
                };
            }
        } else if( checked_value == "spell_exp" ) {
            const std::string spell_name = jo.get_string( "spell" );
            const spell_id this_spell_id( spell_name );
            return [is_npc, min, max, this_spell_id]( dialogue & d, double input ) {
                d.actor( is_npc )->set_spell_exp( this_spell_id, handle_min_max( d, input, min, max ) );
            };
        } else if( checked_value == "proficiency" ) {
            const std::string proficiency_name = jo.get_string( "proficiency_id" );
            const proficiency_id the_proficiency_id( proficiency_name );
            if( jo.has_int( "format" ) ) {
                const int format = jo.get_int( "format" );
                return [is_npc, format, the_proficiency_id]( dialogue const & d, double input ) {
                    d.actor( is_npc )->set_proficiency_practiced_time( the_proficiency_id,
                            to_turns<int>( the_proficiency_id->time_to_learn() * input ) / format );
                };
            } else if( jo.has_member( "format" ) ) {
                const std::string format = jo.get_string( "format" );
                if( format == "time_spent" ) {
                    return [is_npc, the_proficiency_id]( dialogue const & d, double input ) {
                        d.actor( is_npc )->set_proficiency_practiced_time( the_proficiency_id, input );
                    };
                } else if( format == "percent" ) {
                    return [is_npc, the_proficiency_id]( dialogue const & d, double input ) {
                        d.actor( is_npc )->set_proficiency_practiced_time( the_proficiency_id,
                                to_turns<int>( the_proficiency_id->time_to_learn()* input ) / 100 );
                    };
                } else if( format == "permille" ) {
                    return [is_npc, the_proficiency_id]( dialogue const & d, double input ) {
                        d.actor( is_npc )->set_proficiency_practiced_time( the_proficiency_id,
                                to_turns<int>( the_proficiency_id->time_to_learn() * input ) / 1000 );
                    };
                } else if( format == "time_left" ) {
                    return [is_npc, the_proficiency_id]( dialogue const & d, double input ) {
                        d.actor( is_npc )->set_proficiency_practiced_time( the_proficiency_id,
                                to_turns<int>( the_proficiency_id->time_to_learn() ) - input );
                    };
                } else {
                    jo.throw_error( "unrecognized format in " + jo.str() );
                }
            }
        }
    }
    jo.throw_error( "error setting double destination in " + jo.str() );
    return []( dialogue const &, double ) {};
}

void eoc_math::_validate_type( JsonArray const &objects, type_t type_ ) const
{
    if( type_ != type_t::compare && action >= oper::equal ) {
        objects.throw_error( "Comparison operators can only be used in conditional statements" );
    } else if( type_ == type_t::compare && action < oper::equal ) {
        if( action == oper::assign ) {
            objects.throw_error(
                R"(Assignment operator "=" can't be used in a conditional statement.  Did you mean to use "=="? )" );
        } else if( action != oper::ret ) {
            objects.throw_error( "Only comparison operators can be used in conditional statements" );
        }
    } else if( type_ == type_t::ret && action > oper::ret ) {
        objects.throw_error( "Only return expressions are allowed in this context" );
    } else if( type_ != type_t::ret && action == oper::ret ) {
        objects.throw_error( "Return expression in assignment context has no effect" );
    }
}

void eoc_math::from_json( const JsonObject &jo, std::string_view member, type_t type_ )
{
    JsonArray const objects = jo.get_array( member );
    if( objects.size() > 3 ) {
        jo.throw_error( "Invalid number of args in " + jo.str() );
        return;
    }

    std::string const oper = objects.size() >= 2 ? objects.get_string( 1 ) : std::string{};

    if( objects.size() == 1 ) {
        action = oper::ret;
    } else if( objects.size() == 2 ) {
        if( oper == "++" ) {
            action = oper::increase;
        } else if( oper == "--" ) {
            action = oper::decrease;
        } else {
            jo.throw_error( "Invalid unary operator in " + jo.str() );
            return;
        }
    } else if( objects.size() == 3 ) {
        rhs = defer_math( objects.get_string( 2 ), false );
        if( oper == "=" ) {
            action = oper::assign;
        } else if( oper == "+=" ) {
            action = oper::plus_assign;
        } else if( oper == "-=" ) {
            action = oper::minus_assign;
        } else if( oper == "*=" ) {
            action = oper::mult_assign;
        } else if( oper == "/=" ) {
            action = oper::div_assign;
        } else if( oper == "%=" ) {
            action = oper::mod_assign;
        } else if( oper == "==" ) {
            action = oper::equal;
        } else if( oper == "!=" ) {
            action = oper::not_equal;
        } else if( oper == "<" ) {
            action = oper::less;
        } else if( oper == "<=" ) {
            action = oper::equal_or_less;
        } else if( oper == ">" ) {
            action = oper::greater;
        } else if( oper == ">=" ) {
            action = oper::equal_or_greater;
        } else {
            jo.throw_error( "Invalid binary operator in " + jo.str() );
            return;
        }
    }
    _validate_type( objects, type_ );
    bool const lhs_assign = action >= oper::assign && action <= oper::decrease;
    lhs = defer_math( objects.get_string( 0 ), lhs_assign );
    if( action >= oper::plus_assign && action <= oper::decrease ) {
        mhs = defer_math( objects.get_string( 0 ), false );
    }
}

double eoc_math::act( dialogue &d ) const
{
    switch( action ) {
        case oper::ret:
            return lhs->eval( d );
        case oper::assign:
            lhs->assign( d, rhs->eval( d ) );
            break;
        case oper::plus_assign:
            lhs->assign( d, mhs->eval( d ) + rhs->eval( d ) );
            break;
        case oper::minus_assign:
            lhs->assign( d, mhs->eval( d ) - rhs->eval( d ) );
            break;
        case oper::mult_assign:
            lhs->assign( d, mhs->eval( d ) * rhs->eval( d ) );
            break;
        case oper::div_assign:
            lhs->assign( d, mhs->eval( d ) / rhs->eval( d ) );
            break;
        case oper::mod_assign:
            lhs->assign( d, std::fmod( mhs->eval( d ), rhs->eval( d ) ) );
            break;
        case oper::increase:
            lhs->assign( d, mhs->eval( d ) + 1 );
            break;
        case oper::decrease:
            lhs->assign( d, mhs->eval( d ) - 1 );
            break;
        case oper::equal:
            return static_cast<double>( float_equals( lhs->eval( d ), rhs->eval( d ) ) );
        case oper::not_equal:
            return static_cast<double>( !float_equals( lhs->eval( d ), rhs->eval( d ) ) );
        case oper::less:
            return lhs->eval( d ) < rhs->eval( d );
        case oper::equal_or_less:
            return lhs->eval( d ) <= rhs->eval( d );
        case oper::greater:
            return lhs->eval( d ) > rhs->eval( d );
        case oper::equal_or_greater:
            return lhs->eval( d ) >= rhs->eval( d );
        case oper::invalid:
        default:
            debugmsg( "unknown eoc math operator %d %s", action, d.get_callstack() );
    }

    return 0;
}

static const
std::vector<condition_parser>
parsers = {
    {"u_has_any_trait", "npc_has_any_trait", jarg::array, &conditional_fun::f_has_any_trait },
    {"u_has_trait", "npc_has_trait", jarg::member, &conditional_fun::f_has_trait },
    {"u_has_visible_trait", "npc_has_visible_trait", jarg::member, &conditional_fun::f_has_visible_trait },
    {"u_has_martial_art", "npc_has_martial_art", jarg::member, &conditional_fun::f_has_martial_art },
    {"u_using_martial_art", "npc_using_martial_art", jarg::member, &conditional_fun::f_using_martial_art },
    {"u_has_proficiency", "npc_has_proficiency", jarg::member, &conditional_fun::f_has_proficiency },
    {"u_has_flag", "npc_has_flag", jarg::member, &conditional_fun::f_has_flag },
    {"u_has_species", "npc_has_species", jarg::member, &conditional_fun::f_has_species },
    {"u_bodytype", "npc_bodytype", jarg::member, &conditional_fun::f_bodytype },
    {"u_has_class", "npc_has_class", jarg::member, &conditional_fun::f_npc_has_class },
    {"u_has_activity", "npc_has_activity", jarg::string, &conditional_fun::f_has_activity },
    {"u_is_riding", "npc_is_riding", jarg::string, &conditional_fun::f_is_riding },
    {"u_has_mission", jarg::string, &conditional_fun::f_u_has_mission },
    {"u_monsters_in_direction", jarg::string, &conditional_fun::f_u_monsters_in_direction },
    {"u_safe_mode_trigger", jarg::member, &conditional_fun::f_u_safe_mode_trigger },
    {"u_profession", jarg::string, &conditional_fun::f_u_profession },
    {"u_has_strength", "npc_has_strength", jarg::member | jarg::array, &conditional_fun::f_has_strength },
    {"u_has_dexterity", "npc_has_dexterity", jarg::member | jarg::array, &conditional_fun::f_has_dexterity },
    {"u_has_intelligence", "npc_has_intelligence", jarg::member | jarg::array, &conditional_fun::f_has_intelligence },
    {"u_has_perception", "npc_has_perception", jarg::member | jarg::array, &conditional_fun::f_has_perception },
    {"u_has_hp", "npc_has_hp", jarg::member | jarg::array, &conditional_fun::f_has_hp },
    {"u_has_part_temp", "npc_has_part_temp", jarg::member | jarg::array, &conditional_fun::f_has_part_temp },
    {"u_is_wearing", "npc_is_wearing", jarg::member, &conditional_fun::f_is_wearing },
    {"u_has_item", "npc_has_item", jarg::member, &conditional_fun::f_has_item },
    {"u_has_item_with_flag", "npc_has_item_with_flag", jarg::member, &conditional_fun::f_has_item_with_flag },
    {"u_has_items", "npc_has_items", jarg::member, &conditional_fun::f_has_items },
    {"u_has_item_category", "npc_has_item_category", jarg::member, &conditional_fun::f_has_item_category },
    {"u_has_bionics", "npc_has_bionics", jarg::member, &conditional_fun::f_has_bionics },
    {"u_has_any_effect", "npc_has_any_effect", jarg::array, &conditional_fun::f_has_any_effect },
    {"u_has_effect", "npc_has_effect", jarg::member, &conditional_fun::f_has_effect },
    {"u_need", "npc_need", jarg::member, &conditional_fun::f_need },
    {"u_query", "npc_query", jarg::member, &conditional_fun::f_query },
    {"u_query_tile", "npc_query_tile", jarg::member, &conditional_fun::f_query_tile },
    {"u_at_om_location", "npc_at_om_location", jarg::member, &conditional_fun::f_at_om_location },
    {"u_near_om_location", "npc_near_om_location", jarg::member, &conditional_fun::f_near_om_location },
    {"u_has_var", "npc_has_var", jarg::string, &conditional_fun::f_has_var },
    {"expects_vars", jarg::member, &conditional_fun::f_expects_vars },
    {"u_compare_var", "npc_compare_var", jarg::string, &conditional_fun::f_compare_var },
    {"npc_role_nearby", jarg::string, &conditional_fun::f_npc_role_nearby },
    {"npc_allies", jarg::member | jarg::array, &conditional_fun::f_npc_allies },
    {"npc_allies_global", jarg::member | jarg::array, &conditional_fun::f_npc_allies_global },
    {"u_service", "npc_service", jarg::member, &conditional_fun::f_npc_service },
    {"u_has_cash", jarg::member | jarg::array, &conditional_fun::f_u_has_cash },
    {"u_are_owed", jarg::member | jarg::array, &conditional_fun::f_u_are_owed },
    {"u_aim_rule", "npc_aim_rule", jarg::member, &conditional_fun::f_npc_aim_rule },
    {"u_engagement_rule", "npc_engagement_rule", jarg::member, &conditional_fun::f_npc_engagement_rule },
    {"u_cbm_reserve_rule", "npc_cbm_reserve_rule", jarg::member, &conditional_fun::f_npc_cbm_reserve_rule },
    {"u_cbm_recharge_rule", "npc_cbm_recharge_rule", jarg::member, &conditional_fun::f_npc_cbm_recharge_rule },
    {"u_rule", "npc_rule", jarg::member, &conditional_fun::f_npc_rule },
    {"u_override", "npc_override", jarg::member, &conditional_fun::f_npc_override },
    {"is_season", jarg::member, &conditional_fun::f_is_season },
    {"u_mission_goal", "mission_goal", jarg::member, &conditional_fun::f_mission_goal },
    {"u_mission_goal", "npc_mission_goal", jarg::member, &conditional_fun::f_mission_goal },
    {"roll_contested", jarg::member, &conditional_fun::f_roll_contested },
    {"u_know_recipe", jarg::member, &conditional_fun::f_u_know_recipe },
    {"one_in_chance", jarg::member | jarg::array, &conditional_fun::f_one_in_chance },
    {"x_in_y_chance", jarg::object, &conditional_fun::f_x_in_y_chance },
    {"u_has_worn_with_flag", "npc_has_worn_with_flag", jarg::member, &conditional_fun::f_has_worn_with_flag },
    {"u_has_wielded_with_flag", "npc_has_wielded_with_flag", jarg::member, &conditional_fun::f_has_wielded_with_flag },
    {"u_has_wielded_with_weapon_category", "npc_has_wielded_with_weapon_category", jarg::member, &conditional_fun::f_has_wielded_with_weapon_category },
    {"u_is_on_terrain", "npc_is_on_terrain", jarg::member, &conditional_fun::f_is_on_terrain },
    {"u_is_on_terrain_with_flag", "npc_is_on_terrain_with_flag", jarg::member, &conditional_fun::f_is_on_terrain_with_flag },
    {"u_is_in_field", "npc_is_in_field", jarg::member, &conditional_fun::f_is_in_field },
    {"u_has_move_mode", "npc_has_move_mode", jarg::member, &conditional_fun::f_has_move_mode },
    {"u_can_see_location", "npc_can_see_location", jarg::member, &conditional_fun::f_can_see_location },
    {"is_weather", jarg::member, &conditional_fun::f_is_weather },
    {"map_terrain_with_flag", jarg::member, &conditional_fun::f_map_ter_furn_with_flag },
    {"map_furniture_with_flag", jarg::member, &conditional_fun::f_map_ter_furn_with_flag },
    {"map_in_city", jarg::member, &conditional_fun::f_map_in_city },
    {"mod_is_loaded", jarg::member, &conditional_fun::f_mod_is_loaded },
    {"u_has_faction_trust", jarg::member | jarg::array, &conditional_fun::f_has_faction_trust },
    {"compare_int", jarg::member, &conditional_fun::f_compare_num },
    {"compare_num", jarg::member, &conditional_fun::f_compare_num },
    {"math", jarg::member, &conditional_fun::f_math },
    {"compare_string", jarg::member, &conditional_fun::f_compare_string },
    {"get_condition", jarg::member, &conditional_fun::f_get_condition },
};

// When updating this, please also update `dynamic_line_string_keys` in
// `lang/string_extractor/parsers/talk_topic.py` so the lines are properly
// extracted for translation
static const
std::vector<condition_parser>
parsers_simple = {
    {"u_male", "npc_male", &conditional_fun::f_is_male },
    {"u_female", "npc_female", &conditional_fun::f_is_female },
    {"has_no_assigned_mission", &conditional_fun::f_no_assigned_mission },
    {"has_assigned_mission", &conditional_fun::f_has_assigned_mission },
    {"has_many_assigned_missions", &conditional_fun::f_has_many_assigned_missions },
    {"u_has_no_available_mission", "has_no_available_mission", &conditional_fun::f_no_available_mission },
    {"u_has_no_available_mission", "npc_has_no_available_mission", &conditional_fun::f_no_available_mission },
    {"u_has_available_mission", "has_available_mission", &conditional_fun::f_has_available_mission },
    {"u_has_available_mission", "npc_has_available_mission", &conditional_fun::f_has_available_mission },
    {"u_has_many_available_missions", "has_many_available_missions", &conditional_fun::f_has_many_available_missions },
    {"u_has_many_available_missions", "npc_has_many_available_missions", &conditional_fun::f_has_many_available_missions },
    {"u_mission_complete", "mission_complete", &conditional_fun::f_mission_complete },
    {"u_mission_complete", "npc_mission_complete", &conditional_fun::f_mission_complete },
    {"u_mission_incomplete", "mission_incomplete", &conditional_fun::f_mission_incomplete },
    {"u_mission_incomplete", "npc_mission_incomplete", &conditional_fun::f_mission_incomplete },
    {"u_mission_failed", "mission_failed", &conditional_fun::f_mission_failed },
    {"u_mission_failed", "npc_mission_failed", &conditional_fun::f_mission_failed },
    {"u_available", "npc_available", &conditional_fun::f_npc_available },
    {"u_following", "npc_following", &conditional_fun::f_npc_following },
    {"u_friend", "npc_friend", &conditional_fun::f_npc_friend },
    {"u_hostile", "npc_hostile", &conditional_fun::f_npc_hostile },
    {"u_train_skills", "npc_train_skills", &conditional_fun::f_npc_train_skills },
    {"u_train_styles", "npc_train_styles", &conditional_fun::f_npc_train_styles },
    {"u_train_spells", "npc_train_spells", &conditional_fun::f_npc_train_spells },
    {"u_at_safe_space", "at_safe_space", &conditional_fun::f_at_safe_space },
    {"u_at_safe_space", "npc_at_safe_space", &conditional_fun::f_at_safe_space },
    {"u_can_stow_weapon", "npc_can_stow_weapon", &conditional_fun::f_can_stow_weapon },
    {"u_can_drop_weapon", "npc_can_drop_weapon", &conditional_fun::f_can_drop_weapon },
    {"u_has_weapon", "npc_has_weapon", &conditional_fun::f_has_weapon },
    {"u_driving", "npc_driving", &conditional_fun::f_is_driving },
    {"u_has_activity", "npc_has_activity", &conditional_fun::f_has_activity },
    {"u_is_riding", "npc_is_riding", &conditional_fun::f_is_riding },
    {"is_day", &conditional_fun::f_is_day },
    {"u_has_stolen_item", "npc_has_stolen_item", &conditional_fun::f_has_stolen_item },
    {"u_is_outside", "is_outside", &conditional_fun::f_is_outside },
    {"u_is_outside", "npc_is_outside", &conditional_fun::f_is_outside },
    {"u_is_underwater", "npc_is_underwater", &conditional_fun::f_is_underwater },
    {"u_has_camp", &conditional_fun::f_u_has_camp },
    {"u_has_pickup_list", "has_pickup_list", &conditional_fun::f_has_pickup_list },
    {"u_has_pickup_list", "npc_has_pickup_list", &conditional_fun::f_has_pickup_list },
    {"is_by_radio", &conditional_fun::f_is_by_radio },
    {"has_reason", &conditional_fun::f_has_reason },
    {"mission_has_generic_rewards", &conditional_fun::f_mission_has_generic_rewards },
    {"u_can_see", "npc_can_see", &conditional_fun::f_can_see },
    {"u_is_deaf", "npc_is_deaf", &conditional_fun::f_is_deaf },
    {"u_is_alive", "npc_is_alive", &conditional_fun::f_is_alive },
    {"u_is_avatar", "npc_is_avatar", &conditional_fun::f_is_avatar },
    {"u_is_npc", "npc_is_npc", &conditional_fun::f_is_npc },
    {"u_is_character", "npc_is_character", &conditional_fun::f_is_character },
    {"u_is_monster", "npc_is_monster", &conditional_fun::f_is_monster },
    {"u_is_item", "npc_is_item", &conditional_fun::f_is_item },
    {"u_is_furniture", "npc_is_furniture", &conditional_fun::f_is_furniture },
    {"has_ammo", &conditional_fun::f_has_ammo },
    {"player_see_u", "player_see_npc", &conditional_fun::f_player_see },
};

conditional_t::conditional_t( const JsonObject &jo )
{
    // improve the clarity of NPC setter functions
    bool found_sub_member = false;
    const auto parse_array = []( const JsonObject & jo, const std::string_view type ) {
        std::vector<conditional_t> conditionals;
        for( const JsonValue entry : jo.get_array( type ) ) {
            if( entry.test_string() ) {
                conditional_t type_condition( entry.get_string() );
                conditionals.emplace_back( type_condition );
            } else {
                JsonObject cond = entry.get_object();
                conditional_t type_condition( cond );
                conditionals.emplace_back( type_condition );
            }
        }
        return conditionals;
    };
    if( jo.has_array( "and" ) ) {
        std::vector<conditional_t> and_conditionals = parse_array( jo, "and" );
        found_sub_member = true;
        condition = [acs = std::move( and_conditionals )]( dialogue & d ) {
            return std::all_of( acs.begin(), acs.end(), [&d]( conditional_t const & cond ) {
                return cond( d );
            } );
        };
    } else if( jo.has_array( "or" ) ) {
        std::vector<conditional_t> or_conditionals = parse_array( jo, "or" );
        found_sub_member = true;
        condition = [ocs = std::move( or_conditionals )]( dialogue & d ) {
            return std::any_of( ocs.begin(), ocs.end(), [&d]( conditional_t const & cond ) {
                return cond( d );
            } );
        };
    } else if( jo.has_object( "not" ) ) {
        JsonObject cond = jo.get_object( "not" );
        const conditional_t sub_condition = conditional_t( cond );
        found_sub_member = true;
        condition = [sub_condition]( dialogue & d ) {
            return !sub_condition( d );
        };
    } else if( jo.has_string( "not" ) ) {
        const conditional_t sub_condition = conditional_t( jo.get_string( "not" ) );
        found_sub_member = true;
        condition = [sub_condition]( dialogue & d ) {
            return !sub_condition( d );
        };
    }
    if( !found_sub_member ) {
        for( const std::string &sub_member : dialogue_data::complex_conds() ) {
            if( jo.has_member( sub_member ) ) {
                found_sub_member = true;
                break;
            }
        }
    }
    bool found = false;
    for( const condition_parser &p : parsers ) {
        if( p.has_beta ) {
            if( p.check( jo ) ) {
                condition = p.f_beta( jo, p.key_alpha, false );
                found = true;
            } else if( p.check( jo, true ) ) {
                condition = p.f_beta( jo, p.key_beta, true );
                found = true;
            }
        } else if( p.check( jo ) ) {
            condition = p.f( jo, p.key_alpha );
            if( jo.has_member( "math" ) ) {
                found_sub_member = true;
            }
            found = true;
        }
        if( found ) {
            break;
        }
    }
    if( !found ) {
        for( const std::string &sub_member : dialogue_data::simple_string_conds() ) {
            if( jo.has_string( sub_member ) ) {
                const conditional_t sub_condition( jo.get_string( sub_member ) );
                condition = [sub_condition]( dialogue & d ) {
                    return sub_condition( d );
                };
                found_sub_member = true;
                break;
            }
        }
    }
    if( !found_sub_member ) {
        jo.throw_error( "unrecognized condition in " + jo.str() );
    }
}

conditional_t::conditional_t( std::string_view type )
{
    bool found = false;
    for( const condition_parser &p : parsers_simple ) {
        if( p.has_beta ) {
            if( type == p.key_alpha ) {
                condition = p.f_beta_simple( false );
                found = true;
            } else if( type == p.key_beta ) {
                condition = p.f_beta_simple( true );
                found = true;
            }
        } else if( type == p.key_alpha ) {
            condition = p.f_simple();
            found = true;
        }
        if( found ) {
            break;
        }
    }
    if( !found ) {
        condition = []( dialogue const & ) {
            return false;
        };
    }
}

const std::unordered_set<std::string> &dialogue_data::simple_string_conds()
{
    static std::unordered_set<std::string> ret;
    if( ret.empty() ) {
        for( const condition_parser &p : parsers_simple ) {
            ret.emplace( p.key_alpha );
            if( p.has_beta ) {
                ret.emplace( p.key_beta );
            }
        }
    }
    return ret;
}

const std::unordered_set<std::string> &dialogue_data::complex_conds()
{
    static std::unordered_set<std::string> ret;
    if( ret.empty() ) {
        for( const condition_parser &p : parsers ) {
            ret.emplace( p.key_alpha );
            if( p.has_beta ) {
                ret.emplace( p.key_beta );
            }
        }
    }
    return ret;
}

template std::function<double( dialogue & )>
conditional_t::get_get_dbl<>( kwargs_shim const & );

template std::function<void( dialogue &, double )>
conditional_t::get_set_dbl<>( const kwargs_shim &,
                              const std::optional<dbl_or_var_part> &,
                              const std::optional<dbl_or_var_part> &, bool );

template std::function<void( dialogue &, double )>
conditional_t::get_set_dbl<>( const JsonObject &,
                              const std::optional<dbl_or_var_part> &,
                              const std::optional<dbl_or_var_part> &, bool );
