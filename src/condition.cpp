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

#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "coordinates.h"
#include "dialogue.h"
#include "debug.h"
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
#include "math_parser.h"
#include "math_parser_shim.h"
#include "mission.h"
#include "mtype.h"
#include "npc.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
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

} // namespace

std::string get_talk_varname( const JsonObject &jo, const std::string &member,
                              bool check_value, dbl_or_var &default_val )
{
    if( check_value && !( jo.has_string( "value" ) || jo.has_member( "time" ) ||
                          jo.has_array( "possible_values" ) ) ) {
        jo.throw_error( "invalid " + member + " condition in " + jo.str() );
    }
    const std::string &var_basename = jo.get_string( member );
    const std::string &type_var = jo.get_string( "type", "" );
    const std::string &var_context = jo.get_string( "context", "" );
    default_val = get_dbl_or_var( jo, "default", false );
    if( jo.has_member( "default_time" ) ) {
        dbl_or_var value;
        time_duration max_time;
        mandatory( jo, false, "default_time", max_time );
        value.min.dbl_val = to_turns<int>( max_time );
        default_val = std::move( value );
    }
    return "npctalk_var" + ( type_var.empty() ? "" : "_" + type_var ) + ( var_context.empty() ? "" : "_"
            + var_context ) + "_" + var_basename;
}

std::string get_talk_var_basename( const JsonObject &jo, const std::string &member,
                                   bool check_value )
{
    if( check_value && !( jo.has_string( "value" ) || jo.has_member( "time" ) ||
                          jo.has_array( "possible_values" ) ) ) {
        jo.throw_error( "invalid " + member + " condition in " + jo.str() );
    }
    const std::string &var_basename = jo.get_string( member );
    return var_basename;
}

dbl_or_var_part get_dbl_or_var_part( const JsonValue &jv, const std::string &member,
                                     bool required,
                                     double default_val )
{
    dbl_or_var_part ret_val;
    if( jv.test_float() ) {
        ret_val.dbl_val = jv.get_float();
    } else if( jv.test_object() ) {
        JsonObject jo = jv.get_object();
        jo.allow_omitted_members();
        if( jo.has_array( "arithmetic" ) ) {
            talk_effect_fun_t arith;
            arith.set_arithmetic( jo, "arithmetic", true );
            ret_val.arithmetic_val = arith;
        } else if( jo.has_array( "math" ) ) {
            ret_val.math_val.emplace();
            ret_val.math_val->from_json( jo, "math" );
        } else {
            ret_val.var_val = read_var_info( jo );
        }
    } else if( required ) {
        jv.throw_error( "No valid value for " + member );
    } else {
        ret_val.dbl_val = default_val;
    }
    return ret_val;
}

dbl_or_var get_dbl_or_var( const JsonObject &jo, const std::string &member, bool required,
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

duration_or_var_part get_duration_or_var_part( const JsonValue &jv, const std::string &member,
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
        ret_val.dur_val = time_duration::from_turns( jv.get_int() );
    } else if( jv.test_object() ) {
        JsonObject jo = jv.get_object();
        jo.allow_omitted_members();
        if( jo.has_array( "arithmetic" ) ) {
            talk_effect_fun_t arith;
            arith.set_arithmetic( jo, "arithmetic", true );
            ret_val.arithmetic_val = arith;
        } else if( jo.has_array( "math" ) ) {
            ret_val.math_val.emplace();
            ret_val.math_val->from_json( jo, "math" );
        } else {
            ret_val.var_val = read_var_info( jo );
        }
    } else if( required ) {
        jv.throw_error( "No valid value for " + member );
    } else {
        ret_val.dur_val = default_val;
    }
    return ret_val;
}

duration_or_var get_duration_or_var( const JsonObject &jo, const std::string &member, bool required,
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

str_or_var get_str_or_var( const JsonValue &jv, const std::string &member, bool required,
                           const std::string &default_val )
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
        jv.throw_error( "No valid value for " + member );
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
    } else if( jo.has_member( "default_time" ) ) {
        time_duration max_time;
        mandatory( jo, false, "default_time", max_time );
        default_val = std::to_string( to_turns<int>( max_time ) );
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
    switch( type ) {
        case var_type::global:
            globvars.set_global_value( name, value );
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
                     std::function<bool( dialogue & )> &condition, bool default_val )
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

void conditional_t::set_has_any_trait( const JsonObject &jo, const std::string &member,
                                       bool is_npc )
{
    std::vector<str_or_var> traits_to_check;
    for( JsonValue jv : jo.get_array( member ) ) {
        traits_to_check.emplace_back( get_str_or_var( jv, member ) );
    }
    condition = [traits_to_check, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        for( const str_or_var &trait : traits_to_check ) {
            if( actor->has_trait( trait_id( trait.evaluate( d ) ) ) ) {
                return true;
            }
        }
        return false;
    };
}

void conditional_t::set_has_trait( const JsonObject &jo, const std::string &member, bool is_npc )
{
    str_or_var trait_to_check = get_str_or_var( jo.get_member( member ), member, true );
    condition = [trait_to_check, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_trait( trait_id( trait_to_check.evaluate( d ) ) );
    };
}

void conditional_t::set_has_martial_art( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    str_or_var style_to_check = get_str_or_var( jo.get_member( member ), member, true );
    condition = [style_to_check, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->knows_martial_art( matype_id( style_to_check.evaluate( d ) ) );
    };
}

void conditional_t::set_has_flag( const JsonObject &jo, const std::string &member,
                                  bool is_npc )
{
    str_or_var trait_flag_to_check = get_str_or_var( jo.get_member( member ), member, true );
    condition = [trait_flag_to_check, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        if( json_character_flag( trait_flag_to_check.evaluate( d ) ) == json_flag_MUTATION_THRESHOLD ) {
            return actor->crossed_threshold();
        }
        return actor->has_flag( json_character_flag( trait_flag_to_check.evaluate( d ) ) );
    };
}

void conditional_t::set_has_activity( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_activity();
    };
}

void conditional_t::set_is_riding( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_mounted();
    };
}

void conditional_t::set_npc_has_class( const JsonObject &jo, const std::string &member,
                                       bool is_npc )
{
    str_or_var class_to_check = get_str_or_var( jo.get_member( member ), member, true );
    condition = [class_to_check, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_myclass( npc_class_id( class_to_check.evaluate( d ) ) );
    };
}

void conditional_t::set_u_has_mission( const JsonObject &jo, const std::string &member )
{
    str_or_var u_mission = get_str_or_var( jo.get_member( member ), member, true );
    condition = [u_mission]( dialogue const & d ) {
        for( mission *miss_it : get_avatar().get_active_missions() ) {
            if( miss_it->mission_id() == mission_type_id( u_mission.evaluate( d ) ) ) {
                return true;
            }
        }
        return false;
    };
}

void conditional_t::set_u_monsters_in_direction( const JsonObject &jo,
        const std::string &member )
{
    str_or_var dir = get_str_or_var( jo.get_member( member ), member, true );
    condition = [dir]( dialogue const & d ) {
        //This string_to_enum function is defined in widget.h. Should it be moved?
        const int card_dir = static_cast<int>( io::string_to_enum<cardinal_direction>( dir.evaluate(
                d ) ) );
        int monster_count = get_avatar().get_mon_visible().unique_mons[card_dir].size();
        return monster_count > 0;
    };
}

void conditional_t::set_u_safe_mode_trigger( const JsonObject &jo, const std::string &member )
{
    str_or_var dir = get_str_or_var( jo.get_member( member ), member, true );
    condition = [dir]( dialogue const & d ) {
        //This string_to_enum function is defined in widget.h. Should it be moved?
        const int card_dir = static_cast<int>( io::string_to_enum<cardinal_direction>( dir.evaluate(
                d ) ) );
        return get_avatar().get_mon_visible().dangerous[card_dir];
    };
}

void conditional_t::set_has_strength( const JsonObject &jo, const std::string &member,
                                      bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    condition = [dov, is_npc]( dialogue & d ) {
        return d.actor( is_npc )->str_cur() >= dov.evaluate( d );
    };
}

void conditional_t::set_has_dexterity( const JsonObject &jo, const std::string &member,
                                       bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    condition = [dov, is_npc]( dialogue & d ) {
        return d.actor( is_npc )->dex_cur() >= dov.evaluate( d );
    };
}

void conditional_t::set_has_intelligence( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    condition = [dov, is_npc]( dialogue & d ) {
        return d.actor( is_npc )->int_cur() >= dov.evaluate( d );
    };
}

void conditional_t::set_has_perception( const JsonObject &jo, const std::string &member,
                                        bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    condition = [dov, is_npc]( dialogue & d ) {
        return d.actor( is_npc )->per_cur() >= dov.evaluate( d );
    };
}

void conditional_t::set_has_hp( const JsonObject &jo, const std::string &member, bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    std::optional<bodypart_id> bp;
    optional( jo, false, "bodypart", bp );
    condition = [dov, bp, is_npc]( dialogue & d ) {
        bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
        return d.actor( is_npc )->get_cur_hp( bid ) >= dov.evaluate( d );
    };
}

void conditional_t::set_has_part_temp( const JsonObject &jo, const std::string &member,
                                       bool is_npc )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    std::optional<bodypart_id> bp;
    optional( jo, false, "bodypart", bp );
    condition = [dov, bp, is_npc]( dialogue & d ) {
        bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
        return d.actor( is_npc )->get_cur_part_temp( bid ) >= dov.evaluate( d );
    };
}

void conditional_t::set_is_wearing( const JsonObject &jo, const std::string &member,
                                    bool is_npc )
{
    str_or_var item_id = get_str_or_var( jo.get_member( member ), member, true );
    condition = [item_id, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_wearing( itype_id( item_id.evaluate( d ) ) );
    };
}

void conditional_t::set_has_item( const JsonObject &jo, const std::string &member, bool is_npc )
{
    str_or_var item_id = get_str_or_var( jo.get_member( member ), member, true );
    condition = [item_id, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        return actor->charges_of( itype_id( item_id.evaluate( d ) ) ) > 0 ||
               actor->has_amount( itype_id( item_id.evaluate( d ) ), 1 );
    };
}

void conditional_t::set_has_items( const JsonObject &jo, const std::string_view member,
                                   bool is_npc )
{
    JsonObject has_items = jo.get_object( member );
    if( !has_items.has_string( "item" ) || ( !has_items.has_int( "count" ) &&
            !has_items.has_int( "charges" ) ) ) {
        condition = []( dialogue const & ) {
            return false;
        };
    } else {
        str_or_var item_id = get_str_or_var( has_items.get_member( "item" ), "item", true );
        dbl_or_var count = get_dbl_or_var( has_items, "count", false );
        dbl_or_var charges = get_dbl_or_var( has_items, "charges", false );
        condition = [item_id, count, charges, is_npc]( dialogue & d ) {
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

void conditional_t::set_has_item_with_flag( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    str_or_var flag = get_str_or_var( jo.get_member( member ), member, true );
    condition = [flag, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_item_with_flag( flag_id( flag.evaluate( d ) ) );
    };
}

void conditional_t::set_has_item_category( const JsonObject &jo, const std::string &member,
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

    condition = [category_id, count, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        const item_category_id cat_id = item_category_id( category_id.evaluate( d ) );
        const auto items_with = actor->const_items_with( [cat_id]( const item & it ) {
            return it.get_category_shallow().get_id() == cat_id;
        } );
        return items_with.size() >= count;
    };
}

void conditional_t::set_has_bionics( const JsonObject &jo, const std::string &member,
                                     bool is_npc )
{
    str_or_var bionics_id = get_str_or_var( jo.get_member( member ), member, true );
    condition = [bionics_id, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        if( bionics_id.evaluate( d ) == "ANY" ) {
            return actor->num_bionics() > 0 || actor->has_max_power();
        }
        return actor->has_bionic( bionic_id( bionics_id.evaluate( d ) ) );
    };
}

void conditional_t::set_has_effect( const JsonObject &jo, const std::string &member,
                                    bool is_npc )
{
    str_or_var effect_id = get_str_or_var( jo.get_member( member ), member, true );
    dbl_or_var intensity = get_dbl_or_var( jo, "intensity", false, -1 );
    str_or_var bp;
    if( jo.has_member( "bodypart" ) ) {
        bp = get_str_or_var( jo.get_member( "target_part" ), "target_part", true );
    } else {
        bp.str_val = "";
    }
    condition = [effect_id, intensity, bp, is_npc]( dialogue & d ) {
        bodypart_id bid = bp.evaluate( d ).empty() ? get_bp_from_str( d.reason ) : bodypart_id( bp.evaluate(
                              d ) );
        effect target = d.actor( is_npc )->get_effect( efftype_id( effect_id.evaluate( d ) ), bid );
        return !target.is_null() && intensity.evaluate( d ) <= target.get_intensity();
    };
}

void conditional_t::set_need( const JsonObject &jo, const std::string &member, bool is_npc )
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
    condition = [need, dov, is_npc]( dialogue & d ) {
        const talker *actor = d.actor( is_npc );
        int amount = dov.evaluate( d );
        return ( actor->get_fatigue() > amount && need.evaluate( d ) == "fatigue" ) ||
               ( actor->get_hunger() > amount && need.evaluate( d ) == "hunger" ) ||
               ( actor->get_thirst() > amount && need.evaluate( d ) == "thirst" );
    };
}

void conditional_t::set_at_om_location( const JsonObject &jo, const std::string &member,
                                        bool is_npc )
{
    str_or_var location = get_str_or_var( jo.get_member( member ), member, true );
    condition = [location, is_npc]( dialogue const & d ) {
        const tripoint_abs_omt omt_pos = d.actor( is_npc )->global_omt_location();
        const oter_id &omt_ter = overmap_buffer.ter( omt_pos );
        const std::string &omt_str = omt_ter.id().c_str();

        if( location.evaluate( d ) == "FACTION_CAMP_ANY" ) {
            std::optional<basecamp *> bcp = overmap_buffer.find_camp( omt_pos.xy() );
            if( bcp ) {
                return true;
            }
            // TODO: legacy check to be removed once primitive field camp OMTs have been purged
            return omt_str.find( "faction_base_camp" ) != std::string::npos;
        } else if( location.evaluate( d ) == "FACTION_CAMP_START" ) {
            return !recipe_group::get_recipes_by_id( "all_faction_base_types", omt_str ).empty();
        } else {
            return oter_no_dir( omt_ter ) == location.evaluate( d );
        }
    };
}

void conditional_t::set_near_om_location( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    str_or_var location = get_str_or_var( jo.get_member( member ), member, true );
    const dbl_or_var range = get_dbl_or_var( jo, "range", false, 1 );
    condition = [location, range, is_npc]( dialogue & d ) {
        const tripoint_abs_omt omt_pos = d.actor( is_npc )->global_omt_location();
        for( const tripoint_abs_omt &curr_pos : points_in_radius( omt_pos,
                range.evaluate( d ) ) ) {
            const oter_id &omt_ter = overmap_buffer.ter( curr_pos );
            const std::string &omt_str = omt_ter.id().c_str();

            if( location.evaluate( d ) == "FACTION_CAMP_ANY" ) {
                std::optional<basecamp *> bcp = overmap_buffer.find_camp( curr_pos.xy() );
                if( bcp ) {
                    return true;
                }
                // TODO: legacy check to be removed once primitive field camp OMTs have been purged
                if( omt_str.find( "faction_base_camp" ) != std::string::npos ) {
                    return true;
                }
            } else if( location.evaluate( d ) == "FACTION_CAMP_START" &&
                       !recipe_group::get_recipes_by_id( "all_faction_base_types", omt_str ).empty() ) {
                return true;
            } else {
                if( oter_no_dir( omt_ter ) == location.evaluate( d ) ) {
                    return true;
                }
            }
        }
        // should never get here this is for safety
        return false;
    };
}

void conditional_t::set_has_var( const JsonObject &jo, const std::string &member, bool is_npc )
{
    dbl_or_var empty;
    const std::string var_name = get_talk_varname( jo, member, false, empty );
    const std::string &value = jo.has_member( "value" ) ? jo.get_string( "value" ) : std::string();
    const bool time_check = jo.has_member( "time" ) && jo.get_bool( "time" );
    condition = [var_name, value, time_check, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        if( time_check ) {
            return !actor->get_value( var_name ).empty();
        }
        return actor->get_value( var_name ) == value;
    };
}

void conditional_t::set_expects_vars( const JsonObject &jo, const std::string &member )
{
    std::vector<str_or_var> to_check;
    if( jo.has_array( member ) ) {
        for( const JsonValue &jv : jo.get_array( member ) ) {
            to_check.push_back( get_str_or_var( jv, member, true ) );
        }
    }

    condition = [to_check]( dialogue const & d ) {
        std::string missing_variables;
        for( const str_or_var &val : to_check ) {
            if( d.get_context().find( val.evaluate( d ) ) == d.get_context().end() ) {
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

void conditional_t::set_compare_var( const JsonObject &jo, const std::string &member,
                                     bool is_npc )
{
    dbl_or_var empty;
    const std::string var_name = get_talk_varname( jo, member, false, empty );
    const std::string &op = jo.get_string( "op" );

    dbl_or_var dov = get_dbl_or_var( jo, "value" );
    condition = [var_name, op, dov, is_npc]( dialogue & d ) {
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

void conditional_t::set_compare_time_since_var( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    dbl_or_var empty;
    const std::string var_name = get_talk_varname( jo, member, false, empty );
    const std::string &op = jo.get_string( "op" );
    const int value = to_turns<int>( read_from_json_string<time_duration>( jo.get_member( "time" ),
                                     time_duration::units ) );
    condition = [var_name, op, value, is_npc]( dialogue const & d ) {
        int stored_value = 0;
        const std::string &var = d.actor( is_npc )->get_value( var_name );
        if( var.empty() ) {
            return false;
        } else {
            stored_value = std::stof( var );
        }
        stored_value += value;
        int now = to_turn<int>( calendar::turn );

        if( op == "==" ) {
            return stored_value == now;

        } else if( op == "!=" ) {
            return stored_value != now;

        } else if( op == "<=" ) {
            return now <= stored_value;

        } else if( op == ">=" ) {
            return now >= stored_value;

        } else if( op == "<" ) {
            return now < stored_value;

        } else if( op == ">" ) {
            return now > stored_value;
        }

        return false;
    };
}

void conditional_t::set_npc_role_nearby( const JsonObject &jo, const std::string &member )
{
    str_or_var role = get_str_or_var( jo.get_member( member ), member, true );
    condition = [role]( dialogue const & d ) {
        const std::vector<npc *> available = g->get_npcs_if( [&]( const npc & guy ) {
            return d.actor( false )->posz() == guy.posz() &&
                   guy.companion_mission_role_id == role.evaluate( d ) &&
                   ( rl_dist( d.actor( false )->pos(), guy.pos() ) <= 48 );
        } );
        return !available.empty();
    };
}

void conditional_t::set_npc_allies( const JsonObject &jo, const std::string &member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    condition = [dov]( dialogue & d ) {
        return g->allies().size() >= static_cast<std::vector<npc *>::size_type>( dov.evaluate( d ) );
    };
}

void conditional_t::set_npc_allies_global( const JsonObject &jo, const std::string &member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    condition = [dov]( dialogue & d ) {
        const auto all_npcs = overmap_buffer.get_overmap_npcs();
        const size_t count = std::count_if( all_npcs.begin(),
        all_npcs.end(), []( const shared_ptr_fast<npc> &ptr ) {
            return ptr.get()->is_player_ally() && !ptr.get()->hallucination && !ptr.get()->is_dead();
        } );

        return count >= static_cast<size_t>( dov.evaluate( d ) );
    };
}

void conditional_t::set_u_has_cash( const JsonObject &jo, const std::string &member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    condition = [dov]( dialogue & d ) {
        return d.actor( false )->cash() >= dov.evaluate( d );
    };
}

void conditional_t::set_u_are_owed( const JsonObject &jo, const std::string &member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    condition = [dov]( dialogue & d ) {
        return d.actor( true )->debt() >= dov.evaluate( d );
    };
}

void conditional_t::set_npc_aim_rule( const JsonObject &jo, const std::string &member,
                                      bool is_npc )
{
    str_or_var setting = get_str_or_var( jo.get_member( member ), member, true );
    condition = [setting, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "aim_rule", setting.evaluate( d ) );
    };
}

void conditional_t::set_npc_engagement_rule( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    str_or_var setting = get_str_or_var( jo.get_member( member ), member, true );
    condition = [setting, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "engagement_rule", setting.evaluate( d ) );
    };
}

void conditional_t::set_npc_cbm_reserve_rule( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    str_or_var setting = get_str_or_var( jo.get_member( member ), member, true );
    condition = [setting, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "cbm_reserve_rule", setting.evaluate( d ) );
    };
}

void conditional_t::set_npc_cbm_recharge_rule( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    str_or_var setting = get_str_or_var( jo.get_member( member ), member, true );
    condition = [setting, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "cbm_recharge_rule", setting.evaluate( d ) );
    };
}

void conditional_t::set_npc_rule( const JsonObject &jo, const std::string &member, bool is_npc )
{
    str_or_var rule = get_str_or_var( jo.get_member( member ), member, true );
    condition = [rule, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "ally_rule", rule.evaluate( d ) );
    };
}

void conditional_t::set_npc_override( const JsonObject &jo, const std::string &member,
                                      bool is_npc )
{
    str_or_var rule = get_str_or_var( jo.get_member( member ), member, true );
    condition = [rule, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "ally_override", rule.evaluate( d ) );
    };
}

void conditional_t::set_days_since( const JsonObject &jo, const std::string &member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    condition = [dov]( dialogue & d ) {
        return calendar::turn >= calendar::start_of_cataclysm + 1_days * dov.evaluate( d );
    };
}

void conditional_t::set_is_season( const JsonObject &jo, const std::string &member )
{
    str_or_var season_name = get_str_or_var( jo.get_member( member ), member, true );
    condition = [season_name]( dialogue const & d ) {
        const season_type season = season_of_year( calendar::turn );
        return ( season == SPRING && season_name.evaluate( d ) == "spring" ) ||
               ( season == SUMMER && season_name.evaluate( d ) == "summer" ) ||
               ( season == AUTUMN && season_name.evaluate( d ) == "autumn" ) ||
               ( season == WINTER && season_name.evaluate( d ) == "winter" );
    };
}

void conditional_t::set_mission_goal( const JsonObject &jo, const std::string &member,
                                      bool is_npc )
{
    str_or_var mission_goal_str = get_str_or_var( jo.get_member( member ), member, true );
    condition = [mission_goal_str, is_npc]( dialogue const & d ) {
        mission *miss = d.actor( is_npc )->selected_mission();
        if( !miss ) {
            return false;
        }
        const mission_goal mgoal = io::string_to_enum<mission_goal>( mission_goal_str.evaluate( d ) );
        return miss->get_type().goal == mgoal;
    };
}

void conditional_t::set_is_gender( bool is_male, bool is_npc )
{
    condition = [is_male, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_male() == is_male;
    };
}

void conditional_t::set_no_assigned_mission()
{
    condition = []( dialogue const & d ) {
        return d.missions_assigned.empty();
    };
}

void conditional_t::set_has_assigned_mission()
{
    condition = []( dialogue const & d ) {
        return d.missions_assigned.size() == 1;
    };
}

void conditional_t::set_has_many_assigned_missions()
{
    condition = []( dialogue const & d ) {
        return d.missions_assigned.size() >= 2;
    };
}

void conditional_t::set_no_available_mission( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->available_missions().empty();
    };
}

void conditional_t::set_has_available_mission( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->available_missions().size() == 1;
    };
}

void conditional_t::set_has_many_available_missions( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->available_missions().size() >= 2;
    };
}

void conditional_t::set_mission_complete( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        mission *miss = d.actor( is_npc )->selected_mission();
        return miss && miss->is_complete( d.actor( is_npc )->getID() );
    };
}

void conditional_t::set_mission_incomplete( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        mission *miss = d.actor( is_npc )->selected_mission();
        return miss && !miss->is_complete( d.actor( is_npc )->getID() );
    };
}

void conditional_t::set_mission_failed( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        mission *miss = d.actor( is_npc )->selected_mission();
        return miss && miss->has_failed();
    };
}

void conditional_t::set_npc_available( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return !d.actor( is_npc )->has_effect( effect_currently_busy, bodypart_str_id::NULL_ID() );
    };
}

void conditional_t::set_npc_following( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_following();
    };
}

void conditional_t::set_npc_friend( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_friendly( get_player_character() );
    };
}

void conditional_t::set_npc_hostile( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_enemy();
    };
}

void conditional_t::set_npc_train_skills( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return !d.actor( is_npc )->skills_offered_to( *d.actor( !is_npc ) ).empty();
    };
}

void conditional_t::set_npc_train_styles( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return !d.actor( is_npc )->styles_offered_to( *d.actor( !is_npc ) ).empty();
    };
}

void conditional_t::set_npc_train_spells( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return !d.actor( is_npc )->spells_offered_to( *d.actor( !is_npc ) ).empty();
    };
}

void conditional_t::set_at_safe_space( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return overmap_buffer.is_safe( d.actor( is_npc )->global_omt_location() ) &&
               d.actor( is_npc )->is_safe();
    };
}

void conditional_t::set_can_stow_weapon( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        return !actor->unarmed_attack() && actor->can_stash_weapon();
    };
}

void conditional_t::set_can_drop_weapon( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        return !actor->unarmed_attack() && !actor->wielded_with_flag( flag_NO_UNWIELD );
    };
}

void conditional_t::set_has_weapon( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return !d.actor( is_npc )->unarmed_attack();
    };
}

void conditional_t::set_is_driving( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        if( const optional_vpart_position vp = get_map().veh_at( actor->pos() ) ) {
            return vp->vehicle().is_moving() && actor->is_in_control_of( vp->vehicle() );
        }
        return false;
    };
}

void conditional_t::set_has_stolen_item( bool /*is_npc*/ )
{
    condition = []( dialogue const & d ) {
        return d.actor( false )->has_stolen_item( *d.actor( true ) );
    };
}

void conditional_t::set_is_day()
{
    condition = []( dialogue const & ) {
        return !is_night( calendar::turn );
    };
}

void conditional_t::set_is_outside( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return is_creature_outside( *d.actor( is_npc )->get_creature() );
    };
}

void conditional_t::set_is_underwater( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return get_map().is_divable( d.actor( is_npc )->pos() );
    };
}

void conditional_t::set_one_in_chance( const JsonObject &jo, const std::string &member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    condition = [dov]( dialogue & d ) {
        return one_in( dov.evaluate( d ) );
    };
}

void conditional_t::set_query( const JsonObject &jo, const std::string &member, bool is_npc )
{
    str_or_var message = get_str_or_var( jo.get_member( member ), member, true );
    bool default_val = jo.get_bool( "default" );
    condition = [message, default_val, is_npc]( dialogue const & d ) {
        const talker *actor = d.actor( is_npc );
        if( actor->get_character() && actor->get_character()->is_avatar() ) {
            std::string translated_message = _( message.evaluate( d ) );
            return query_yn( translated_message );
        } else {
            return default_val;
        }
    };
}

void conditional_t::set_x_in_y_chance( const JsonObject &jo, const std::string_view member )
{
    const JsonObject &var_obj = jo.get_object( member );
    dbl_or_var dovx = get_dbl_or_var( var_obj, "x" );
    dbl_or_var dovy = get_dbl_or_var( var_obj, "y" );
    condition = [dovx, dovy]( dialogue & d ) {
        return x_in_y( dovx.evaluate( d ),
                       dovy.evaluate( d ) );
    };
}

void conditional_t::set_is_weather( const JsonObject &jo, const std::string &member )
{
    str_or_var weather = get_str_or_var( jo.get_member( member ), member, true );
    condition = [weather]( dialogue const & d ) {
        return get_weather().weather_id == weather_type_id( weather.evaluate( d ) );
    };
}

void conditional_t::set_mod_is_loaded( const JsonObject &jo, const std::string &member )
{
    str_or_var compared_mod = get_str_or_var( jo.get_member( member ), member, true );
    condition = [compared_mod]( dialogue const & d ) {
        mod_id comp_mod = mod_id( compared_mod.evaluate( d ) );
        for( const mod_id &mod : world_generator->active_world->active_mod_order ) {
            if( comp_mod == mod ) {
                return true;
            }
        }
        return false;
    };
}

void conditional_t::set_has_faction_trust( const JsonObject &jo, const std::string &member )
{
    dbl_or_var dov = get_dbl_or_var( jo, member );
    condition = [dov]( dialogue & d ) {
        return d.actor( true )->get_faction()->trusts_u >= dov.evaluate( d );
    };
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
        var_info var = var_info( var_type::faction, type.substr( 7, type.size() - 7 ) );
        return get_tripoint_from_var( var, d );
    } else if( type.find( "party_" ) == 0 ) {
        var_info var = var_info( var_type::party, type.substr( 7, type.size() - 7 ) );
        return get_tripoint_from_var( var, d );
    } else if( type.find( "context_" ) == 0 ) {
        var_info var = var_info( var_type::context, type.substr( 7, type.size() - 7 ) );
        return get_tripoint_from_var( var, d );
    }
    return tripoint_abs_ms();
}

void conditional_t::set_compare_string( const JsonObject &jo, const std::string &member )
{
    str_or_var first;
    str_or_var second;
    JsonArray objects = jo.get_array( member );
    if( objects.size() != 2 ) {
        jo.throw_error( "incorrect number of values.  Expected 2 in " + jo.str() );
        condition = []( dialogue const & ) {
            return false;
        };
        return;
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

    condition = [first, second]( dialogue const & d ) {
        return first.evaluate( d ) == second.evaluate( d );
    };
}

void conditional_t::set_get_condition( const JsonObject &jo, const std::string &member )
{
    str_or_var conditionalToGet = get_str_or_var( jo.get_member( member ), member, true );
    condition = [conditionalToGet]( dialogue & d ) {
        return d.evaluate_conditional( conditionalToGet.evaluate( d ), d );
    };
}

void conditional_t::set_get_option( const JsonObject &jo, const std::string &member )
{
    str_or_var optionToGet = get_str_or_var( jo.get_member( member ), member, true );
    condition = [optionToGet]( dialogue & d ) {
        return get_option<bool>( optionToGet.evaluate( d ) );
    };
}

void conditional_t::set_compare_num( const JsonObject &jo, const std::string_view member )
{
    JsonArray objects = jo.get_array( member );
    if( objects.size() != 3 ) {
        jo.throw_error( "incorrect number of values.  Expected three in " + jo.str() );
        condition = []( dialogue const & ) {
            return false;
        };
        return;
    }
    std::function<double( dialogue & )> get_first_dbl = objects.has_object( 0 ) ? get_get_dbl(
                objects.get_object( 0 ) ) : get_get_dbl( objects.get_string( 0 ), jo );
    std::function<double( dialogue & )> get_second_dbl = objects.has_object( 2 ) ? get_get_dbl(
                objects.get_object( 2 ) ) : get_get_dbl( objects.get_string( 2 ), jo );
    const std::string &op = objects.get_string( 1 );

    if( op == "==" || op == "=" ) {
        condition = [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) == get_second_dbl( d );
        };
    } else if( op == "!=" ) {
        condition = [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) != get_second_dbl( d );
        };
    } else if( op == "<=" ) {
        condition = [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) <= get_second_dbl( d );
        };
    } else if( op == ">=" ) {
        condition = [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) >= get_second_dbl( d );
        };
    } else if( op == "<" ) {
        condition = [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) < get_second_dbl( d );
        };
    } else if( op == ">" ) {
        condition = [get_first_dbl, get_second_dbl]( dialogue & d ) {
            return get_first_dbl( d ) > get_second_dbl( d );
        };
    } else {
        jo.throw_error( "unexpected operator " + jo.get_string( "op" ) + " in " + jo.str() );
        condition = []( dialogue const & ) {
            return false;
        };
    }
}

void conditional_t::set_math( const JsonObject &jo, const std::string_view member )
{
    eoc_math math;
    math.from_json( jo, member );
    condition = [math = std::move( math )]( dialogue & d ) {
        return math.act( d );
    };
}

template<class J>
std::function<std::string( const dialogue & )> conditional_t::get_get_string( J const &jo )
{
    if( jo.get_string( "mutator" ) == "mon_faction" ) {
        str_or_var mtypeid = get_str_or_var( jo.get_member( "mtype_id" ), "mtype_id" );
        return [mtypeid]( const dialogue & d ) {
            return ( static_cast<mtype_id>( mtypeid.evaluate( d ) ) )->default_faction.str();
        };
    } else if( jo.get_string( "mutator" ) == "game_option" ) {
        str_or_var option = get_str_or_var( jo.get_member( "option" ), "option" );
        return [option]( const dialogue & d ) {
            return get_option<std::string>( option.evaluate( d ) );
        };
    }

    jo.throw_error( "unrecognized string mutator in " + jo.str() );
    return []( const dialogue & ) {
        return "INVALID";
    };
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
    } else if( jo.has_member( "time" ) ) {
        const int value = to_turns<int>( read_from_json_string<time_duration>( jo.get_member( "time" ),
                                         time_duration::units ) );
        return [value]( dialogue const & ) {
            return value;
        };
    } else if( jo.has_member( "power" ) ) {
        units::energy power;
        if constexpr( std::is_same_v<JsonObject, J> ) {
            assign( jo, "power", power, false, 0_kJ );
        }
        const int power_value = units::to_millijoule( power );
        return [power_value]( dialogue const & ) {
            return power_value;
        };
    } else if( jo.has_member( "time_since_cataclysm" ) ) {
        time_duration given_unit = 1_turns;
        if( jo.has_string( "time_since_cataclysm" ) ) {
            std::string given_unit_str = jo.get_string( "time_since_cataclysm" );
            bool found = false;
            for( const auto &pair : time_duration::units ) {
                const std::string &unit = pair.first;
                if( unit == given_unit_str ) {
                    given_unit = pair.second;
                    found = true;
                    break;
                }
            }
            if( !found ) {
                jo.throw_error( "unrecognized time unit in " + jo.str() );
            }
        }
        return [given_unit]( dialogue const & ) {
            return ( to_turn<int>( calendar::turn ) - to_turn<int>( calendar::start_of_cataclysm ) ) /
                   to_turns<int>( given_unit );
        };
    } else if( jo.has_member( "time_until_eoc" ) ) {
        time_duration given_unit = 1_turns;
        effect_on_condition_id eoc_id = effect_on_condition_id( jo.get_string( "time_until_eoc" ) );
        if( jo.has_string( "unit" ) ) {
            std::string given_unit_str = jo.get_string( "unit" );
            bool found = false;
            for( const auto &pair : time_duration::units ) {
                const std::string &unit = pair.first;
                if( unit == given_unit_str ) {
                    given_unit = pair.second;
                    found = true;
                    break;
                }
            }
            if( !found ) {
                jo.throw_error( "unrecognized time unit in " + jo.str() );
            }
        }
        return [eoc_id, given_unit]( dialogue const & ) {
            std::priority_queue<queued_eoc, std::vector<queued_eoc>, eoc_compare> copy_queue =
                g->queued_global_effect_on_conditions;
            time_point turn;
            bool found = false;
            while( !copy_queue.empty() ) {
                if( copy_queue.top().eoc == eoc_id ) {
                    turn = copy_queue.top().time;
                    found = true;
                    break;
                }
                copy_queue.pop();
            }
            if( !found ) {
                return -1;
            } else {
                return to_turns<int>( turn - calendar::turn ) / to_turns<int>( given_unit );
            }
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
        } else if( checked_value == "hp" ) {
            std::optional<bodypart_id> bp;
            if constexpr( std::is_same_v<JsonObject, J> ) {
                optional( jo, false, "bodypart", bp );
            }
            return [is_npc, bp]( dialogue const & d ) {
                bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
                return d.actor( is_npc )->get_cur_hp( bid );
            };
        } else if( checked_value == "warmth" ) {
            std::optional<bodypart_id> bp;
            if constexpr( std::is_same_v<JsonObject, J> ) {
                optional( jo, false, "bodypart", bp );
            }
            return [is_npc, bp]( dialogue const & d ) {
                bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
                return d.actor( is_npc )->get_cur_part_temp( bid );
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
        } else if( checked_value == "time_since_var" ) {
            dbl_or_var empty;
            std::string var_name;
            if constexpr( std::is_same_v<JsonObject, J> ) {
                var_name = get_talk_varname( jo, "var_name", false, empty );
            }
            return [is_npc, var_name]( dialogue const & d ) {
                int stored_value = 0;
                const std::string &var = d.actor( is_npc )->get_value( var_name );
                if( !var.empty() ) {
                    stored_value = std::stof( var );
                }
                return to_turn<int>( calendar::turn ) - stored_value;
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
                return std::max( d.actor( is_npc )->charges_of( item_id ),
                                 d.actor( is_npc )->get_amount( item_id ) );
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
        } else if( checked_value == "item_rad" ) {
            if constexpr( std::is_same_v<JsonObject, J> ) {
                const std::string flag = jo.get_string( "flag" );
                const aggregate_type agg = jo.template get_enum_value<aggregate_type>( "aggregate",
                                           aggregate_type::FIRST );
                return [is_npc, flag, agg]( dialogue const & d ) {
                    return d.actor( is_npc )->item_rads( flag_id( flag ), agg );
                };
            }
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
                Character *you = d.actor( is_npc )->get_character();
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
                return d.actor( is_npc )->get_body_temp();
            };
        } else if( checked_value == "body_temp_delta" ) {
            return [is_npc]( dialogue const & d ) {
                return d.actor( is_npc )->get_body_temp_delta();
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
        } else if( checked_value == "monsters_nearby" ) {
            std::optional<var_info> target_var;
            if( jo.has_object( "target_var" ) ) {
                read_var_info( jo.get_member( "target_var" ) );
            }
            str_or_var id;
            if( jo.has_member( "id" ) ) {
                id = get_str_or_var( jo.get_member( "id" ), "id", false, "" );
            } else {
                id.str_val = "";
            }
            dbl_or_var radius_dov;
            dbl_or_var number_dov;
            if constexpr( std::is_same_v<JsonObject, J> ) {
                radius_dov = get_dbl_or_var( jo, "radius", false, 10000 );
                number_dov = get_dbl_or_var( jo, "number", false, 1 );
            }
            return [target_var, radius_dov, id, number_dov, is_npc]( dialogue & d ) {
                tripoint_abs_ms loc;
                if( target_var.has_value() ) {
                    loc = get_tripoint_from_var( target_var, d );
                } else {
                    loc = d.actor( is_npc )->global_pos();
                }

                int radius = radius_dov.evaluate( d );
                std::vector<Creature *> targets = g->get_creatures_if( [&radius, id, &d,
                         loc]( const Creature & critter ) {
                    if( critter.is_monster() ) {
                        // friendly to the player, not a target for us
                        return critter.as_monster()->friendly == 0 &&
                               radius >= rl_dist( critter.get_location(), loc ) &&
                               ( id.evaluate( d ).empty() ||
                                 critter.as_monster()->type->id == mtype_id( id.evaluate( d ) ) );
                    }
                    return false;
                } );
                return static_cast<int>( targets.size() );
            };
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
        } else if( checked_value == "spell_exp" ) {
            const std::string spell_name = jo.get_string( "spell" );
            const spell_id this_spell_id( spell_name );
            return [is_npc, this_spell_id]( dialogue & d ) {
                return d.actor( is_npc )->get_spell_exp( this_spell_id );
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
    } else if( jo.has_member( "moon" ) ) {
        return []( dialogue const & ) {
            return static_cast<int>( get_moon_phase( calendar::turn ) );
        };
    } else if( jo.has_member( "hour" ) ) {
        return []( dialogue const & ) {
            return to_hours<int>( time_past_midnight( calendar::turn ) );
        };
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
            arith.set_arithmetic( jo, "arithmetic", true );
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
            math.from_json( jo, "math" );
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

std::function<double( dialogue & )> conditional_t::get_get_dbl( const std::string &value,
        const JsonObject &jo )
{
    if( value == "moon" ) {
        return []( dialogue const & ) {
            return static_cast<int>( get_moon_phase( calendar::turn ) );
        };
    } else if( value == "hour" ) {
        return []( dialogue const & ) {
            return to_hours<int>( time_past_midnight( calendar::turn ) );
        };
    }
    jo.throw_error( "unrecognized number source in " + value );
    return []( dialogue const & ) {
        return 0.0;
    };
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
    } else if( jo.has_member( "time" ) ) {
        jo.throw_error( "can not alter a time constant.  Did you mean time_since_cataclysm or time_since_var?  In "
                        + jo.str() );
    } else if( jo.has_member( "time_since_cataclysm" ) ) {
        time_duration given_unit = 1_turns;
        if( jo.has_string( "time_since_cataclysm" ) ) {
            std::string given_unit_str = jo.get_string( "time_since_cataclysm" );
            bool found = false;
            for( const auto &pair : time_duration::units ) {
                const std::string &unit = pair.first;
                if( unit == given_unit_str ) {
                    given_unit = pair.second;
                    found = true;
                    break;
                }
            }
            if( !found ) {
                jo.throw_error( "unrecognized time unit in " + jo.str() );
            }
        }
        return [given_unit, min, max]( dialogue & d, double input ) {
            calendar::turn = time_point( handle_min_max( d, input, min,
                                         max ) * to_turns<int>( given_unit ) );
        };
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
        } else if( checked_value == "time_since_var" ) {
            // This is a strange thing to want to adjust. But we allow it nevertheless.
            dbl_or_var empty;
            std::string var_name;
            if constexpr( std::is_same_v<JsonObject, J> ) {
                var_name = get_talk_varname( jo, "var_name", false, empty );
            }
            return [is_npc, var_name, min, max]( dialogue & d, double input ) {
                int storing_value = to_turn<int>( calendar::turn ) - handle_min_max( d, input, min, max );
                d.actor( is_npc )->set_value( var_name, std::to_string( storing_value ) );
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

void talk_effect_fun_t::set_arithmetic( const JsonObject &jo, const std::string_view member,
                                        bool no_result )
{
    JsonArray objects = jo.get_array( member );
    std::optional<dbl_or_var_part> min;
    std::optional<dbl_or_var_part> max;
    if( jo.has_member( "min" ) ) {
        min = get_dbl_or_var_part( jo.get_member( "min" ), "min" );
    } else if( jo.has_member( "min_time" ) ) {
        dbl_or_var_part value;
        time_duration min_time;
        mandatory( jo, false, "min_time", min_time );
        value.dbl_val = to_turns<int>( min_time );
        min = value;
    }
    if( jo.has_member( "max" ) ) {
        max = get_dbl_or_var_part( jo.get_member( "max" ), "max" );
    } else if( jo.has_member( "max_time" ) ) {
        dbl_or_var_part value;
        time_duration max_time;
        mandatory( jo, false, "max_time", max_time );
        value.dbl_val = to_turns<int>( max_time );
        max = value;
    }
    std::string op = "none";
    std::string result = "none";
    std::function<void( dialogue &, double )> set_dbl = conditional_t::get_set_dbl(
                objects.get_object( 0 ), min,
                max, no_result );
    int no_result_mod = no_result ? 2 : 0; //In the case of a no result we have fewer terms.
    // Normal full version
    if( static_cast<int>( objects.size() ) == 5 - no_result_mod ) {
        op = objects.get_string( 3 - no_result_mod );
        if( !no_result ) {
            result = objects.get_string( 1 );
            if( result != "=" ) {
                jo.throw_error( "invalid result " + op + " in " + jo.str() );
                function = []( dialogue const & ) {
                    return false;
                };
            }
        }
        std::function<double( dialogue & )> get_first_dbl = conditional_t::get_get_dbl(
                    objects.get_object( 2 - no_result_mod ) );
        std::function<double( dialogue & )> get_second_dbl = conditional_t::get_get_dbl(
                    objects.get_object( 4 - no_result_mod ) );
        if( op == "*" ) {
            function = [get_first_dbl, get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, get_first_dbl( d ) * get_second_dbl( d ) );
            };
        } else if( op == "/" ) {
            function = [get_first_dbl, get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, get_first_dbl( d ) / get_second_dbl( d ) );
            };
        } else if( op == "+" ) {
            function = [get_first_dbl, get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, get_first_dbl( d ) + get_second_dbl( d ) );
            };
        } else if( op == "-" ) {
            function = [get_first_dbl, get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, get_first_dbl( d ) - get_second_dbl( d ) );
            };
        } else if( op == "%" ) {
            function = [get_first_dbl, get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, static_cast<int>( get_first_dbl( d ) ) % static_cast<int>( get_second_dbl( d ) ) );
            };
        } else if( op == "^" ) {
            function = [get_first_dbl, get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, pow( get_first_dbl( d ), get_second_dbl( d ) ) );
            };
        } else {
            jo.throw_error( "unexpected operator " + op + " in " + jo.str() );
            function = []( dialogue const & ) {
                return false;
            };
        }
        // ~
    } else if( objects.size() == 4 && !no_result ) {
        op = objects.get_string( 3 );
        result = objects.get_string( 1 );
        if( result != "=" ) {
            jo.throw_error( "invalid result " + op + " in " + jo.str() );
            function = []( dialogue const & ) {
                return false;
            };
        }
        std::function<double( dialogue & )> get_first_dbl = conditional_t::get_get_dbl(
                    objects.get_object( 2 ) );
        if( op == "~" ) {
            function = [get_first_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, ~static_cast<int>( get_first_dbl( d ) ) );
            };
        } else {
            jo.throw_error( "unexpected operator " + op + " in " + jo.str() );
            function = []( dialogue const & ) {
                return false;
            };
        }

        // =, -=, +=, *=, and /=
    } else if( objects.size() == 3 && !no_result ) {
        result = objects.get_string( 1 );
        std::function<double( dialogue & )> get_first_dbl = conditional_t::get_get_dbl(
                    objects.get_object( 0 ) );
        std::function<double( dialogue & )> get_second_dbl = conditional_t::get_get_dbl(
                    objects.get_object( 2 ) );
        if( result == "+=" ) {
            function = [get_first_dbl, get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, get_first_dbl( d ) + get_second_dbl( d ) );
            };
        } else if( result == "-=" ) {
            function = [get_first_dbl, get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, get_first_dbl( d ) - get_second_dbl( d ) );
            };
        } else if( result == "*=" ) {
            function = [get_first_dbl, get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, get_first_dbl( d ) * get_second_dbl( d ) );
            };
        } else if( result == "/=" ) {
            function = [get_first_dbl, get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, get_first_dbl( d ) / get_second_dbl( d ) );
            };
        } else if( result == "%=" ) {
            function = [get_first_dbl, get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, static_cast<int>( get_first_dbl( d ) ) % static_cast<int>( get_second_dbl( d ) ) );
            };
        } else if( result == "=" ) {
            function = [get_second_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, get_second_dbl( d ) );
            };
        } else {
            jo.throw_error( "unexpected result " + result + " in " + jo.str() );
            function = []( dialogue const & ) {
                return false;
            };
        }
        // ++ and --
    } else if( objects.size() == 2 && !no_result ) {
        op = objects.get_string( 1 );
        std::function<double( dialogue & )> get_first_dbl = conditional_t::get_get_dbl(
                    objects.get_object( 0 ) );
        if( op == "++" ) {
            function = [get_first_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, get_first_dbl( d ) + 1 );
            };
        } else if( op == "--" ) {
            function = [get_first_dbl, set_dbl]( dialogue & d ) {
                set_dbl( d, get_first_dbl( d ) - 1 );
            };
        } else {
            jo.throw_error( "unexpected operator " + op + " in " + jo.str() );
            function = []( dialogue const & ) {
                return false;
            };
        }
    } else if( objects.size() == 1 && no_result ) {
        std::function<double( dialogue & )> get_first_dbl = conditional_t::get_get_dbl(
                    objects.get_object( 0 ) );
        function = [get_first_dbl, set_dbl]( dialogue & d ) {
            set_dbl( d, get_first_dbl( d ) );
        };
    } else {
        jo.throw_error( "Invalid number of args in " + jo.str() );
        function = []( dialogue const & ) {
            return false;
        };
        return;
    }
}

void talk_effect_fun_t::set_math( const JsonObject &jo, const std::string_view member )
{
    eoc_math math;
    math.from_json( jo, member );
    function = [math = std::move( math )]( dialogue & d ) {
        return math.act( d );
    };
}

void eoc_math::from_json( const JsonObject &jo, std::string_view member )
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
            debugmsg( "unknown eoc math operator %d", action );
    }

    return 0;
}

void conditional_t::set_u_has_camp()
{
    condition = []( dialogue const & ) {
        return !get_player_character().camps.empty();
    };
}

void conditional_t::set_has_pickup_list( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->has_ai_rule( "pickup_rule", "any" );
    };
}

void conditional_t::set_is_by_radio()
{
    condition = []( dialogue const & d ) {
        return d.by_radio;
    };
}

void conditional_t::set_has_reason()
{
    condition = []( dialogue const & d ) {
        return !d.reason.empty();
    };
}

void conditional_t::set_roll_contested( const JsonObject &jo, const std::string_view member )
{
    std::function<double( dialogue & )> get_check = conditional_t::get_get_dbl( jo.get_object(
                member ) );
    dbl_or_var difficulty = get_dbl_or_var( jo, "difficulty", true );
    dbl_or_var die_size = get_dbl_or_var( jo, "die_size", false, 10 );
    condition = [get_check, difficulty, die_size]( dialogue & d ) {
        return rng( 1, die_size.evaluate( d ) ) + get_check( d ) > difficulty.evaluate( d );
    };
}

void conditional_t::set_u_know_recipe( const JsonObject &jo, const std::string &member )
{
    str_or_var known_recipe_id = get_str_or_var( jo.get_member( member ), member, true );
    condition = [known_recipe_id]( dialogue & d ) {
        const recipe &rep = recipe_id( known_recipe_id.evaluate( d ) ).obj();
        // should be a talker function but recipes aren't in Character:: yet
        return get_player_character().knows_recipe( &rep );
    };
}

void conditional_t::set_mission_has_generic_rewards()
{
    condition = []( dialogue const & d ) {
        mission *miss = d.actor( true )->selected_mission();
        if( miss == nullptr ) {
            debugmsg( "mission_has_generic_rewards: mission_selected == nullptr" );
            return true;
        }
        return miss->has_generic_rewards();
    };
}

void conditional_t::set_has_worn_with_flag( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    str_or_var flag = get_str_or_var( jo.get_member( member ), member, true );
    std::optional<bodypart_id> bp;
    optional( jo, false, "bodypart", bp );
    condition = [flag, bp, is_npc]( dialogue const & d ) {
        bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
        return d.actor( is_npc )->worn_with_flag( flag_id( flag.evaluate( d ) ), bid );
    };
}

void conditional_t::set_has_wielded_with_flag( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    str_or_var flag = get_str_or_var( jo.get_member( member ), member, true );
    condition = [flag, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->wielded_with_flag( flag_id( flag.evaluate( d ) ) );
    };
}

void conditional_t::set_can_see( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->can_see();
    };
}

void conditional_t::set_is_deaf( bool is_npc )
{
    condition = [is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->is_deaf();
    };
}

void conditional_t::set_is_on_terrain( const JsonObject &jo, const std::string &member,
                                       bool is_npc )
{
    str_or_var terrain_type = get_str_or_var( jo.get_member( member ), member, true );
    condition = [terrain_type, is_npc]( dialogue const & d ) {
        map &here = get_map();
        return here.ter( d.actor( is_npc )->pos() ) == ter_id( terrain_type.evaluate( d ) );
    };
}

void conditional_t::set_is_in_field( const JsonObject &jo, const std::string &member,
                                     bool is_npc )
{
    str_or_var field_type = get_str_or_var( jo.get_member( member ), member, true );
    condition = [field_type, is_npc]( dialogue const & d ) {
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

void conditional_t::set_has_move_mode( const JsonObject &jo, const std::string &member,
                                       bool is_npc )
{
    str_or_var mode = get_str_or_var( jo.get_member( member ), member, true );
    condition = [mode, is_npc]( dialogue const & d ) {
        return d.actor( is_npc )->get_move_mode() == move_mode_id( mode.evaluate( d ) );
    };
}

conditional_t::conditional_t( const JsonObject &jo )
{
    // improve the clarity of NPC setter functions
    const bool is_npc = true;
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
        for( const std::string &sub_member : dialogue_data::complex_conds ) {
            if( jo.has_member( sub_member ) ) {
                found_sub_member = true;
                break;
            }
        }
    }
    if( jo.has_array( "u_has_any_trait" ) ) {
        set_has_any_trait( jo, "u_has_any_trait" );
    } else if( jo.has_array( "npc_has_any_trait" ) ) {
        set_has_any_trait( jo, "npc_has_any_trait", true );
    } else if( jo.has_member( "u_has_trait" ) ) {
        set_has_trait( jo, "u_has_trait" );
    } else if( jo.has_member( "npc_has_trait" ) ) {
        set_has_trait( jo, "npc_has_trait", true );
    } else if( jo.has_member( "u_has_martial_art" ) ) {
        set_has_martial_art( jo, "u_has_martial_art" );
    } else if( jo.has_member( "npc_has_martial_art" ) ) {
        set_has_martial_art( jo, "npc_has_martial_art", true );
    } else if( jo.has_member( "u_has_flag" ) ) {
        set_has_flag( jo, "u_has_flag" );
    } else if( jo.has_member( "npc_has_flag" ) ) {
        set_has_flag( jo, "npc_has_flag", true );
    } else if( jo.has_member( "npc_has_class" ) ) {
        set_npc_has_class( jo, "npc_has_class", true );
    } else if( jo.has_member( "u_has_class" ) ) {
        set_npc_has_class( jo, "u_has_class", false );
    } else if( jo.has_string( "npc_has_activity" ) ) {
        set_has_activity( is_npc );
    } else if( jo.has_string( "npc_is_riding" ) ) {
        set_is_riding( is_npc );
    } else if( jo.has_member( "u_has_mission" ) ) {
        set_u_has_mission( jo, "u_has_mission" );
    } else if( jo.has_member( "u_monsters_in_direction" ) ) {
        set_u_monsters_in_direction( jo, "u_monsters_in_direction" );
    } else if( jo.has_member( "u_safe_mode_trigger" ) ) {
        set_u_safe_mode_trigger( jo, "u_safe_mode_trigger" );
    } else if( jo.has_member( "u_has_strength" ) || jo.has_array( "u_has_strength" ) ) {
        set_has_strength( jo, "u_has_strength" );
    } else if( jo.has_member( "npc_has_strength" ) || jo.has_array( "npc_has_strength" ) ) {
        set_has_strength( jo, "npc_has_strength", is_npc );
    } else if( jo.has_member( "u_has_dexterity" ) || jo.has_array( "u_has_dexterity" ) ) {
        set_has_dexterity( jo, "u_has_dexterity" );
    } else if( jo.has_member( "npc_has_dexterity" ) || jo.has_array( "npc_has_dexterity" ) ) {
        set_has_dexterity( jo, "npc_has_dexterity", is_npc );
    } else if( jo.has_member( "u_has_intelligence" ) || jo.has_array( "u_has_intelligence" ) ) {
        set_has_intelligence( jo, "u_has_intelligence" );
    } else if( jo.has_member( "npc_has_intelligence" ) || jo.has_array( "npc_has_intelligence" ) ) {
        set_has_intelligence( jo, "npc_has_intelligence", is_npc );
    } else if( jo.has_member( "u_has_perception" ) || jo.has_array( "u_has_perception" ) ) {
        set_has_perception( jo, "u_has_perception" );
    } else if( jo.has_member( "npc_has_perception" ) || jo.has_array( "npc_has_perception" ) ) {
        set_has_perception( jo, "npc_has_perception", is_npc );
    } else if( jo.has_member( "u_has_hp" ) || jo.has_array( "u_has_hp" ) ) {
        set_has_hp( jo, "u_has_hp" );
    } else if( jo.has_member( "npc_has_hp" ) || jo.has_array( "npc_has_hp" ) ) {
        set_has_hp( jo, "npc_has_hp", is_npc );
    } else if( jo.has_member( "u_has_part_temp" ) || jo.has_array( "u_has_part_temp" ) ) {
        set_has_part_temp( jo, "u_has_part_temp" );
    } else if( jo.has_member( "npc_has_part_temp" ) || jo.has_array( "npc_has_part_temp" ) ) {
        set_has_part_temp( jo, "npc_has_part_temp", is_npc );
    } else if( jo.has_member( "u_is_wearing" ) ) {
        set_is_wearing( jo, "u_is_wearing" );
    } else if( jo.has_member( "npc_is_wearing" ) ) {
        set_is_wearing( jo, "npc_is_wearing", is_npc );
    } else if( jo.has_member( "u_has_item" ) ) {
        set_has_item( jo, "u_has_item" );
    } else if( jo.has_member( "npc_has_item" ) ) {
        set_has_item( jo, "npc_has_item", is_npc );
    } else if( jo.has_member( "u_has_item_with_flag" ) ) {
        set_has_item_with_flag( jo, "u_has_item_with_flag" );
    } else if( jo.has_member( "npc_has_item_with_flag" ) ) {
        set_has_item_with_flag( jo, "npc_has_item_with_flag", is_npc );
    } else if( jo.has_member( "u_has_items" ) ) {
        set_has_items( jo, "u_has_items" );
    } else if( jo.has_member( "npc_has_items" ) ) {
        set_has_items( jo, "npc_has_items", is_npc );
    } else if( jo.has_member( "u_has_item_category" ) ) {
        set_has_item_category( jo, "u_has_item_category" );
    } else if( jo.has_member( "npc_has_item_category" ) ) {
        set_has_item_category( jo, "npc_has_item_category", is_npc );
    } else if( jo.has_member( "u_has_bionics" ) ) {
        set_has_bionics( jo, "u_has_bionics" );
    } else if( jo.has_member( "npc_has_bionics" ) ) {
        set_has_bionics( jo, "npc_has_bionics", is_npc );
    } else if( jo.has_member( "u_has_effect" ) ) {
        set_has_effect( jo, "u_has_effect" );
    } else if( jo.has_member( "npc_has_effect" ) ) {
        set_has_effect( jo, "npc_has_effect", is_npc );
    } else if( jo.has_member( "u_need" ) ) {
        set_need( jo, "u_need" );
    } else if( jo.has_member( "npc_need" ) ) {
        set_need( jo, "npc_need", is_npc );
    } else if( jo.has_member( "u_query" ) ) {
        set_query( jo, "u_query" );
    } else if( jo.has_member( "npc_query" ) ) {
        set_query( jo, "npc_query", is_npc );
    } else if( jo.has_member( "u_at_om_location" ) ) {
        set_at_om_location( jo, "u_at_om_location" );
    } else if( jo.has_member( "npc_at_om_location" ) ) {
        set_at_om_location( jo, "npc_at_om_location", is_npc );
    } else if( jo.has_member( "u_near_om_location" ) ) {
        set_near_om_location( jo, "u_near_om_location" );
    } else if( jo.has_member( "npc_near_om_location" ) ) {
        set_near_om_location( jo, "npc_near_om_location", is_npc );
    } else if( jo.has_string( "u_has_var" ) ) {
        set_has_var( jo, "u_has_var" );
    } else if( jo.has_string( "npc_has_var" ) ) {
        set_has_var( jo, "npc_has_var", is_npc );
    } else if( jo.has_member( "expects_vars" ) ) {
        set_expects_vars( jo, "expects_vars" );
    } else if( jo.has_string( "u_compare_var" ) ) {
        set_compare_var( jo, "u_compare_var" );
    } else if( jo.has_string( "npc_compare_var" ) ) {
        set_compare_var( jo, "npc_compare_var", is_npc );
    } else if( jo.has_string( "u_compare_time_since_var" ) ) {
        set_compare_time_since_var( jo, "u_compare_time_since_var" );
    } else if( jo.has_string( "npc_compare_time_since_var" ) ) {
        set_compare_time_since_var( jo, "npc_compare_time_since_var", is_npc );
    } else if( jo.has_string( "npc_role_nearby" ) ) {
        set_npc_role_nearby( jo, "npc_role_nearby" );
    } else if( jo.has_member( "npc_allies" ) || jo.has_array( "npc_allies" ) ) {
        set_npc_allies( jo, "npc_allies" );
    } else if( jo.has_member( "npc_allies_global" ) || jo.has_array( "npc_allies_global" ) ) {
        set_npc_allies_global( jo, "npc_allies_global" );
    } else if( jo.get_bool( "npc_service", false ) ) {
        set_npc_available( true );
    } else if( jo.get_bool( "u_service", false ) ) {
        set_npc_available( false );
    } else if( jo.has_member( "u_has_cash" ) || jo.has_array( "u_has_cash" ) ) {
        set_u_has_cash( jo, "u_has_cash" );
    } else if( jo.has_member( "u_are_owed" ) || jo.has_array( "u_are_owed" ) ) {
        set_u_are_owed( jo, "u_are_owed" );
    } else if( jo.has_member( "npc_aim_rule" ) ) {
        set_npc_aim_rule( jo, "npc_aim_rule", true );
    } else if( jo.has_member( "u_aim_rule" ) ) {
        set_npc_aim_rule( jo, "u_aim_rule", false );
    } else if( jo.has_member( "npc_engagement_rule" ) ) {
        set_npc_engagement_rule( jo, "npc_engagement_rule", true );
    } else if( jo.has_member( "u_engagement_rule" ) ) {
        set_npc_engagement_rule( jo, "u_engagement_rule", false );
    } else if( jo.has_member( "npc_cbm_reserve_rule" ) ) {
        set_npc_cbm_reserve_rule( jo, "npc_cbm_reserve_rule", true );
    } else if( jo.has_member( "u_cbm_reserve_rule" ) ) {
        set_npc_cbm_reserve_rule( jo, "u_cbm_reserve_rule", false );
    } else if( jo.has_member( "npc_cbm_recharge_rule" ) ) {
        set_npc_cbm_recharge_rule( jo, "npc_cbm_recharge_rule", true );
    } else if( jo.has_member( "u_cbm_recharge_rule" ) ) {
        set_npc_cbm_recharge_rule( jo, "u_cbm_recharge_rule", false );
    } else if( jo.has_member( "npc_rule" ) ) {
        set_npc_rule( jo, "npc_rule", true );
    } else if( jo.has_member( "u_rule" ) ) {
        set_npc_rule( jo, "npc_rule", false );
    } else if( jo.has_member( "npc_override" ) ) {
        set_npc_override( jo, "npc_override", true );
    } else if( jo.has_string( "u_override" ) ) {
        set_npc_override( jo, "u_override", false );
    } else if( jo.has_member( "days_since_cataclysm" ) || jo.has_array( "days_since_cataclysm" ) ) {
        set_days_since( jo, "days_since_cataclysm" );
    } else if( jo.has_member( "is_season" ) ) {
        set_is_season( jo, "is_season" );
    } else if( jo.has_member( "mission_goal" ) ) {
        set_mission_goal( jo, "mission_goal", true );
    } else if( jo.has_member( "npc_mission_goal" ) ) {
        set_mission_goal( jo, "npc_mission_goal", true );
    } else if( jo.has_member( "u_mission_goal" ) ) {
        set_mission_goal( jo, "u_mission_goal", false );
    } else if( jo.has_member( "roll_contested" ) ) {
        set_roll_contested( jo, "roll_contested" );
    } else if( jo.has_member( "u_know_recipe" ) ) {
        set_u_know_recipe( jo, "u_know_recipe" );
    } else if( jo.has_member( "one_in_chance" ) || jo.has_array( "one_in_chance" ) ) {
        set_one_in_chance( jo, "one_in_chance" );
    } else if( jo.has_object( "x_in_y_chance" ) ) {
        set_x_in_y_chance( jo, "x_in_y_chance" );
    } else if( jo.has_member( "u_has_worn_with_flag" ) ) {
        set_has_worn_with_flag( jo, "u_has_worn_with_flag" );
    } else if( jo.has_member( "npc_has_worn_with_flag" ) ) {
        set_has_worn_with_flag( jo, "npc_has_worn_with_flag", is_npc );
    } else if( jo.has_member( "u_has_wielded_with_flag" ) ) {
        set_has_wielded_with_flag( jo, "u_has_wielded_with_flag" );
    } else if( jo.has_member( "npc_has_wielded_with_flag" ) ) {
        set_has_wielded_with_flag( jo, "npc_has_wielded_with_flag", is_npc );
    } else if( jo.has_member( "u_is_on_terrain" ) ) {
        set_is_on_terrain( jo, "u_is_on_terrain" );
    } else if( jo.has_member( "npc_is_on_terrain" ) ) {
        set_is_on_terrain( jo, "npc_is_on_terrain", is_npc );
    } else if( jo.has_member( "u_is_in_field" ) ) {
        set_is_in_field( jo, "u_is_in_field" );
    } else if( jo.has_member( "npc_is_in_field" ) ) {
        set_is_in_field( jo, "npc_is_in_field", is_npc );
    } else if( jo.has_member( "u_has_move_mode" ) ) {
        set_has_move_mode( jo, "u_has_move_mode" );
    } else if( jo.has_member( "npc_has_move_mode" ) ) {
        set_has_move_mode( jo, "npc_has_move_mode", is_npc );
    } else if( jo.has_member( "is_weather" ) ) {
        set_is_weather( jo, "is_weather" );
    } else if( jo.has_member( "mod_is_loaded" ) ) {
        set_mod_is_loaded( jo, "mod_is_loaded" );
    } else if( jo.has_member( "u_has_faction_trust" ) || jo.has_array( "u_has_faction_trust" ) ) {
        set_has_faction_trust( jo, "u_has_faction_trust" );
    } else if( jo.has_member( "compare_int" ) ) {
        set_compare_num( jo, "compare_int" );
    } else if( jo.has_member( "compare_num" ) ) {
        set_compare_num( jo, "compare_num" );
    } else if( jo.has_member( "math" ) ) {
        set_math( jo, "math" );
        found_sub_member = true;
    } else if( jo.has_member( "compare_string" ) ) {
        set_compare_string( jo, "compare_string" );
    } else if( jo.has_member( "get_condition" ) ) {
        set_get_condition( jo, "get_condition" );
    } else if( jo.has_member( "get_game_option" ) ) {
        set_get_option( jo, "get_game_option" );
    } else {
        for( const std::string &sub_member : dialogue_data::simple_string_conds ) {
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

conditional_t::conditional_t( const std::string &type )
{
    const bool is_npc = true;
    if( type == "u_male" ) {
        set_is_gender( true );
    } else if( type == "npc_male" ) {
        set_is_gender( true, is_npc );
    } else if( type == "u_female" ) {
        set_is_gender( false );
    } else if( type == "npc_female" ) {
        set_is_gender( false, is_npc );
    } else if( type == "has_no_assigned_mission" ) {
        set_no_assigned_mission();
    } else if( type == "has_assigned_mission" ) {
        set_has_assigned_mission();
    } else if( type == "has_many_assigned_missions" ) {
        set_has_many_assigned_missions();
    } else if( type == "has_no_available_mission" || type == "npc_has_no_available_mission" ) {
        set_no_available_mission( true );
    } else if( type == "u_has_no_available_mission" ) {
        set_no_available_mission( false );
    } else if( type == "has_available_mission" || type == "npc_has_available_mission" ) {
        set_has_available_mission( true );
    } else if( type == "u_has_available_mission" ) {
        set_has_available_mission( false );
    } else if( type == "has_many_available_missions" || type == "npc_has_many_available_missions" ) {
        set_has_many_available_missions( true );
    } else if( type == "u_has_many_available_missions" ) {
        set_has_many_available_missions( false );
    } else if( type == "mission_complete" || type == "npc_mission_complete" ) {
        set_mission_complete( true );
    } else if( type == "u_mission_complete" ) {
        set_mission_complete( false );
    } else if( type == "mission_incomplete" || type == "npc_mission_incomplete" ) {
        set_mission_incomplete( true );
    } else if( type == "u_mission_incomplete" ) {
        set_mission_incomplete( false );
    } else if( type == "mission_failed" || type == "npc_mission_failed" ) {
        set_mission_failed( true );
    } else if( type == "u_mission_failed" ) {
        set_mission_failed( false );
    } else if( type == "npc_available" ) {
        set_npc_available( true );
    } else if( type == "u_available" ) {
        set_npc_available( false );
    } else if( type == "npc_following" ) {
        set_npc_following( true );
    } else if( type == "u_following" ) {
        set_npc_following( false );
    } else if( type == "npc_friend" ) {
        set_npc_friend( true );
    } else if( type == "u_friend" ) {
        set_npc_friend( false );
    } else if( type == "npc_hostile" ) {
        set_npc_hostile( true );
    } else if( type == "u_hostile" ) {
        set_npc_hostile( false );
    } else if( type == "npc_train_skills" ) {
        set_npc_train_skills( true );
    } else if( type == "u_train_skills" ) {
        set_npc_train_skills( false );
    } else if( type == "npc_train_styles" ) {
        set_npc_train_styles( true );
    } else if( type == "u_train_styles" ) {
        set_npc_train_styles( false );
    } else if( type == "npc_train_spells" ) {
        set_npc_train_spells( true );
    } else if( type == "u_train_spells" ) {
        set_npc_train_spells( false );
    } else if( type == "at_safe_space" || type == "npc_at_safe_space" ) {
        set_at_safe_space( true );
    } else if( type == "u_at_safe_space" ) {
        set_at_safe_space( false );
    } else if( type == "u_can_stow_weapon" ) {
        set_can_stow_weapon();
    } else if( type == "npc_can_stow_weapon" ) {
        set_can_stow_weapon( is_npc );
    } else if( type == "u_can_drop_weapon" ) {
        set_can_drop_weapon();
    } else if( type == "npc_can_drop_weapon" ) {
        set_can_drop_weapon( is_npc );
    } else if( type == "u_has_weapon" ) {
        set_has_weapon();
    } else if( type == "npc_has_weapon" ) {
        set_has_weapon( is_npc );
    } else if( type == "u_driving" ) {
        set_is_driving();
    } else if( type == "npc_driving" ) {
        set_is_driving( is_npc );
    } else if( type == "npc_has_activity" ) {
        set_has_activity( is_npc );
    } else if( type == "npc_is_riding" ) {
        set_is_riding( is_npc );
    } else if( type == "is_day" ) {
        set_is_day();
    } else if( type == "u_has_stolen_item" ) {
        set_has_stolen_item( is_npc );
    } else if( type == "u_is_outside" ) {
        set_is_outside();
    } else if( type == "is_outside" || type == "npc_is_outside" ) {
        set_is_outside( is_npc );
    } else if( type == "u_is_underwater" ) {
        set_is_underwater();
    } else if( type == "npc_is_underwater" ) {
        set_is_underwater( is_npc );
    } else if( type == "u_has_camp" ) {
        set_u_has_camp();
    } else if( type == "has_pickup_list" || type == "npc_has_pickup_list" ) {
        set_has_pickup_list( true );
    } else if( type == "u_has_pickup_list" ) {
        set_has_pickup_list( false );
    } else if( type == "is_by_radio" ) {
        set_is_by_radio();
    } else if( type == "has_reason" ) {
        set_has_reason();
    } else if( type == "mission_has_generic_rewards" ) {
        set_mission_has_generic_rewards();
    } else if( type == "u_can_see" ) {
        set_can_see();
    } else if( type == "npc_can_see" ) {
        set_can_see( is_npc );
    } else if( type == "u_is_deaf" ) {
        set_is_deaf();
    } else if( type == "npc_is_deaf" ) {
        set_is_deaf( is_npc );
    } else {
        condition = []( dialogue const & ) {
            return false;
        };
    }
}

template std::function<double( dialogue & )>
conditional_t::get_get_dbl<>( kwargs_shim const & );

template std::function<void( dialogue &, double )>
conditional_t::get_set_dbl<>( const kwargs_shim &,
                              const std::optional<dbl_or_var_part> &,
                              const std::optional<dbl_or_var_part> &, bool );
