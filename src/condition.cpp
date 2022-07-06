#include "condition.h"

#include <climits>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "enum_conversions.h"
#include "field.h"
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
#include "mission.h"
#include "mtype.h"
#include "npc.h"
#include "optional.h"
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

class basecamp;
class recipe;

static const efftype_id effect_currently_busy( "currently_busy" );

static const json_character_flag json_flag_MUTATION_THRESHOLD( "MUTATION_THRESHOLD" );

template<class T>
std::string get_talk_varname( const JsonObject &jo, const std::string &member,
                              bool check_value, int_or_var<T> &default_val )
{
    if( check_value && !( jo.has_string( "value" ) || jo.has_member( "time" ) ||
                          jo.has_array( "possible_values" ) ) ) {
        jo.throw_error( "invalid " + member + " condition in " + jo.str() );
    }
    const std::string &var_basename = jo.get_string( member );
    const std::string &type_var = jo.get_string( "type", "" );
    const std::string &var_context = jo.get_string( "context", "" );
    default_val = get_int_or_var<T>( jo, "default", false );
    if( jo.has_member( "default_time" ) ) {
        int_or_var<T> value;
        time_duration max_time;
        mandatory( jo, false, "default_time", max_time );
        value.min.int_val = to_turns<int>( max_time );
        default_val = value;
    }
    return "npctalk_var" + ( type_var.empty() ? "" : "_" + type_var ) + ( var_context.empty() ? "" : "_"
            + var_context ) + "_" + var_basename;
}

template<class T>
int_or_var_part<T> get_int_or_var_part( const JsonValue &jv, std::string member, bool required,
                                        int default_val )
{
    int_or_var_part<T> ret_val;
    if( jv.test_int() ) {
        ret_val.int_val = jv.get_int();
    } else if( jv.test_object() ) {
        JsonObject jo = jv.get_object();
        jo.allow_omitted_members();
        if( jo.has_array( "arithmetic" ) ) {
            talk_effect_fun_t<T> arith;
            arith.set_arithmetic( jo, "arithmetic", true );
            ret_val.arithmetic_val = arith;
        } else {
            ret_val.var_val = read_var_info( jo, true );
        }
    } else if( required ) {
        jv.throw_error( "No valid value for " + member );
    } else {
        ret_val.int_val = default_val;
    }
    return ret_val;
}

template<class T>
int_or_var<T> get_int_or_var( const JsonObject &jo, std::string member, bool required,
                              int default_val )
{
    int_or_var<T> ret_val;
    if( jo.has_array( member ) ) {
        JsonArray ja = jo.get_array( member );
        ret_val.min = get_int_or_var_part<T>( ja.next(), member );
        ret_val.max = get_int_or_var_part<T>( ja.next(), member );
        ret_val.pair = true;
    } else if( required ) {
        ret_val.min = get_int_or_var_part<T>( jo.get_member( member ), member, required, default_val );
    } else {
        if( jo.has_member( member ) ) {
            ret_val.min = get_int_or_var_part<T>( jo.get_member( member ), member, required, default_val );
        } else {
            ret_val.min.int_val = default_val;
        }
    }
    return ret_val;
}

template<class T>
duration_or_var_part<T> get_duration_or_var_part( const JsonValue &jv, std::string member,
        bool required, time_duration default_val )
{
    duration_or_var_part<T> ret_val;
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
            talk_effect_fun_t<T> arith;
            arith.set_arithmetic( jo, "arithmetic", true );
            ret_val.arithmetic_val = arith;
        } else {
            ret_val.var_val = read_var_info( jo, true );
        }
    } else if( required ) {
        jv.throw_error( "No valid value for " + member );
    } else {
        ret_val.dur_val = default_val;
    }
    return ret_val;
}

template<class T>
duration_or_var<T> get_duration_or_var( const JsonObject &jo, std::string member, bool required,
                                        time_duration default_val )
{
    duration_or_var<T> ret_val;
    if( jo.has_array( member ) ) {
        JsonArray ja = jo.get_array( member );
        ret_val.min = get_duration_or_var_part<T>( ja.next(), member );
        ret_val.max = get_duration_or_var_part<T>( ja.next(), member );
        ret_val.pair = true;
    } else if( required ) {
        ret_val.min = get_duration_or_var_part<T>( jo.get_member( member ), member, required, default_val );
    } else {
        if( jo.has_member( member ) ) {
            ret_val.min = get_duration_or_var_part<T>( jo.get_member( member ), member, required, default_val );
        } else {
            ret_val.min.dur_val = default_val;
        }
    }
    return ret_val;
}

template<class T>
str_or_var<T> get_str_or_var( const JsonValue &jv, std::string member, bool required,
                              std::string default_val )
{
    str_or_var<T> ret_val;
    if( jv.test_string() ) {
        ret_val.str_val = jv.get_string();
    } else if( jv.test_object() ) {
        ret_val.var_val = read_var_info( jv.get_object(), true );
    } else if( required ) {
        jv.throw_error( "No valid value for " + member );
    } else {
        ret_val.str_val = default_val;
    }
    return ret_val;
}

template<class T>
tripoint_abs_ms get_tripoint_from_var( cata::optional<var_info> var, const T &d )
{
    tripoint_abs_ms target_pos = get_map().getglobal( d.actor( false )->pos() );
    if( var.has_value() ) {
        std::string value = read_var_value<T>( var.value(), d );
        if( !value.empty() ) {
            target_pos = tripoint_abs_ms( tripoint::from_string( value ) );
        }
    }
    return target_pos;
}

var_info read_var_info( const JsonObject &jo, bool require_default )
{
    std::string default_val;
    int_or_var<dialogue> empty;
    var_type type;
    std::string name;
    if( jo.has_string( "default_str" ) ) {
        default_val = jo.get_string( "default_str" );
    } else if( jo.has_string( "default" ) ) {
        default_val = std::to_string( to_turns<int>( read_from_json_string<time_duration>
                                      ( jo.get_member( "default" ), time_duration::units ) ) );
    } else if( jo.has_int( "default" ) ) {
        default_val = std::to_string( jo.get_int( "default" ) );
    } else if( jo.has_member( "default_time" ) ) {
        time_duration max_time;
        mandatory( jo, false, "default_time", max_time );
        default_val = std::to_string( to_turns<int>( max_time ) );
    } else if( require_default ) {
        jo.throw_error( "No default value provided." );
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

void write_var_value( var_type type, std::string name, talker *talk, std::string value )
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
        default:
            debugmsg( "Invalid type." );
            break;
    }
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

template<class T>
void read_condition( const JsonObject &jo, const std::string &member_name,
                     std::function<bool( const T & )> &condition, bool default_val )
{
    const auto null_function = [default_val]( const T & ) {
        return default_val;
    };

    if( !jo.has_member( member_name ) ) {
        condition = null_function;
    } else if( jo.has_string( member_name ) ) {
        const std::string type = jo.get_string( member_name );
        conditional_t<T> sub_condition( type );
        condition = [sub_condition]( const T & d ) {
            return sub_condition( d );
        };
    } else if( jo.has_object( member_name ) ) {
        JsonObject con_obj = jo.get_object( member_name );
        conditional_t<T> sub_condition( con_obj );
        condition = [sub_condition]( const T & d ) {
            return sub_condition( d );
        };
    } else {
        jo.throw_error_at( member_name, "invalid condition syntax" );
    }
}

template<class T>
void conditional_t<T>::set_has_any_trait( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    std::vector<trait_id> traits_to_check;
    for( auto&& f : jo.get_string_array( member ) ) { // *NOPAD*
        traits_to_check.emplace_back( f );
    }
    condition = [traits_to_check, is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
        for( const auto &trait : traits_to_check ) {
            if( actor->has_trait( trait ) ) {
                return true;
            }
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_has_trait( const JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &trait_to_check = jo.get_string( member );
    condition = [trait_to_check, is_npc]( const T & d ) {
        return d.actor( is_npc )->has_trait( trait_id( trait_to_check ) );
    };
}

template<class T>
void conditional_t<T>::set_has_flag( const JsonObject &jo, const std::string &member,
                                     bool is_npc )
{
    const json_character_flag &trait_flag_to_check = json_character_flag( jo.get_string( member ) );
    condition = [trait_flag_to_check, is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
        if( trait_flag_to_check == json_flag_MUTATION_THRESHOLD ) {
            return actor->crossed_threshold();
        }
        return actor->has_flag( trait_flag_to_check );
    };
}

template<class T>
void conditional_t<T>::set_has_activity( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return d.actor( is_npc )->has_activity();
    };
}

template<class T>
void conditional_t<T>::set_is_riding( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return d.actor( is_npc )->is_mounted();
    };
}

template<class T>
void conditional_t<T>::set_npc_has_class( const JsonObject &jo, bool is_npc )
{
    const std::string &class_to_check = jo.get_string( "npc_has_class" );
    condition = [class_to_check, is_npc]( const T & d ) {
        return d.actor( is_npc )->is_myclass( npc_class_id( class_to_check ) );
    };
}

template<class T>
void conditional_t<T>::set_u_has_mission( const JsonObject &jo )
{
    const std::string &u_mission = jo.get_string( "u_has_mission" );
    condition = [u_mission]( const T & ) {
        for( mission *miss_it : get_avatar().get_active_missions() ) {
            if( miss_it->mission_id() == mission_type_id( u_mission ) ) {
                return true;
            }
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_u_monsters_in_direction( const JsonObject &jo )
{
    const std::string &dir = jo.get_string( "u_monsters_in_direction" );
    condition = [dir]( const T & ) {
        //This string_to_enum function is defined in widget.h. Should it be moved?
        const int card_dir = static_cast<int>( io::string_to_enum<cardinal_direction>( dir ) );
        int monster_count = get_avatar().get_mon_visible().unique_mons[card_dir].size();
        return monster_count > 0;
    };
}

template<class T>
void conditional_t<T>::set_u_safe_mode_trigger( const JsonObject &jo )
{
    const std::string &dir = jo.get_string( "u_safe_mode_trigger" );
    condition = [dir]( const T & ) {
        //This string_to_enum function is defined in widget.h. Should it be moved?
        const int card_dir = static_cast<int>( io::string_to_enum<cardinal_direction>( dir ) );
        return get_avatar().get_mon_visible().dangerous[card_dir];
    };
}

template<class T>
void conditional_t<T>::set_has_strength( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, member );
    condition = [iov, is_npc]( const T & d ) {
        return d.actor( is_npc )->str_cur() >= iov.evaluate( d );
    };
}

template<class T>
void conditional_t<T>::set_has_dexterity( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, member );
    condition = [iov, is_npc]( const T & d ) {
        return d.actor( is_npc )->dex_cur() >= iov.evaluate( d );
    };
}

template<class T>
void conditional_t<T>::set_has_intelligence( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, member );
    condition = [iov, is_npc]( const T & d ) {
        return d.actor( is_npc )->int_cur() >= iov.evaluate( d );
    };
}

template<class T>
void conditional_t<T>::set_has_perception( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, member );
    condition = [iov, is_npc]( const T & d ) {
        return d.actor( is_npc )->per_cur() >= iov.evaluate( d );
    };
}

template<class T>
void conditional_t<T>::set_has_hp( const JsonObject &jo, const std::string &member, bool is_npc )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, member );
    cata::optional<bodypart_id> bp;
    optional( jo, false, "bodypart", bp );
    condition = [iov, bp, is_npc]( const T & d ) {
        bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
        return d.actor( is_npc )->get_cur_hp( bid ) >= iov.evaluate( d );
    };
}

template<class T>
void conditional_t<T>::set_is_wearing( const JsonObject &jo, const std::string &member,
                                       bool is_npc )
{
    const itype_id item_id( jo.get_string( member ) );
    condition = [item_id, is_npc]( const T & d ) {
        return d.actor( is_npc )->is_wearing( item_id );
    };
}

template<class T>
void conditional_t<T>::set_has_item( const JsonObject &jo, const std::string &member, bool is_npc )
{
    const itype_id item_id( jo.get_string( member ) );
    condition = [item_id, is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
        return actor->charges_of( item_id ) > 0 || actor->has_amount( item_id, 1 );
    };
}

template<class T>
void conditional_t<T>::set_has_items( const JsonObject &jo, const std::string &member, bool is_npc )
{
    JsonObject has_items = jo.get_object( member );
    if( !has_items.has_string( "item" ) || ( !has_items.has_int( "count" ) &&
            !has_items.has_int( "charges" ) ) ) {
        condition = []( const T & ) {
            return false;
        };
    } else {
        const itype_id item_id( has_items.get_string( "item" ) );
        int count = 0;
        if( has_items.has_int( "count" ) ) {
            count = has_items.get_int( "count" );
        }
        int charges = 0;
        if( has_items.has_int( "charges" ) ) {
            charges = has_items.get_int( "charges" );
        }
        condition = [item_id, count, charges, is_npc]( const T & d ) {
            const talker *actor = d.actor( is_npc );
            if( charges == 0 && item::count_by_charges( item_id ) ) {
                return actor->has_charges( item_id, count, true );
            }
            if( charges > 0 && count == 0 ) {
                return actor->has_charges( item_id, charges, true );
            }
            bool has_enough_charges = true;
            if( charges > 0 ) {
                has_enough_charges = actor->has_charges( item_id, charges, true );
            }
            return has_enough_charges && actor->has_amount( item_id, count );
        };
    }
}

template<class T>
void conditional_t<T>::set_has_item_with_flag( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const std::string flag( jo.get_string( member ) );
    condition = [flag, is_npc]( const T & d ) {
        return d.actor( is_npc )->has_item_with_flag( flag_id( flag ) );
    };
}

template<class T>
void conditional_t<T>::set_has_item_category( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const item_category_id category_id = item_category_id( jo.get_string( member ) );

    size_t count = 1;
    if( jo.has_int( "count" ) ) {
        int tcount = jo.get_int( "count" );
        if( tcount > 1 && tcount < INT_MAX ) {
            count = static_cast<size_t>( tcount );
        }
    }

    condition = [category_id, count, is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
        const auto items_with = actor->const_items_with( [category_id]( const item & it ) {
            return it.get_category_shallow().get_id() == category_id;
        } );
        return items_with.size() >= count;
    };
}

template<class T>
void conditional_t<T>::set_has_bionics( const JsonObject &jo, const std::string &member,
                                        bool is_npc )
{
    const std::string bionics_id = jo.get_string( member );
    condition = [bionics_id, is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
        if( bionics_id == "ANY" ) {
            return actor->num_bionics() > 0 || actor->has_max_power();
        }
        return actor->has_bionic( bionic_id( bionics_id ) );
    };
}

template<class T>
void conditional_t<T>::set_has_effect( const JsonObject &jo, const std::string &member,
                                       bool is_npc )
{
    const std::string &effect_id = jo.get_string( member );
    cata::optional<int> intensity;
    cata::optional<bodypart_id> bp;
    optional( jo, false, "intensity", intensity );
    optional( jo, false, "bodypart", bp );
    condition = [effect_id, intensity, bp, is_npc]( const T & d ) {
        bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
        effect target = d.actor( is_npc )->get_effect( efftype_id( effect_id ), bid );
        return !target.is_null() && intensity.value_or( -1 ) <= target.get_intensity();
    };
}

template<class T>
void conditional_t<T>::set_need( const JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &need = jo.get_string( member );
    int_or_var<T> iov;
    if( jo.has_int( "amount" ) ) {
        iov.min.int_val = jo.get_int( "amount" );
    } else if( jo.has_object( "amount" ) ) {
        iov = get_int_or_var<T>( jo, "amount" );
    } else if( jo.has_string( "level" ) ) {
        const std::string &level = jo.get_string( "level" );
        auto flevel = fatigue_level_strs.find( level );
        if( flevel != fatigue_level_strs.end() ) {
            iov.min.int_val = static_cast<int>( flevel->second );
        }
    }
    condition = [need, iov, is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
        int amount = iov.evaluate( d );
        return ( actor->get_fatigue() > amount && need == "fatigue" ) ||
               ( actor->get_hunger() > amount && need == "hunger" ) ||
               ( actor->get_thirst() > amount && need == "thirst" );
    };
}

template<class T>
void conditional_t<T>::set_at_om_location( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const std::string &location = jo.get_string( member );
    condition = [location, is_npc]( const T & d ) {
        const tripoint_abs_omt omt_pos = d.actor( is_npc )->global_omt_location();
        const oter_id &omt_ter = overmap_buffer.ter( omt_pos );
        const std::string &omt_str = omt_ter.id().c_str();

        if( location == "FACTION_CAMP_ANY" ) {
            cata::optional<basecamp *> bcp = overmap_buffer.find_camp( omt_pos.xy() );
            if( bcp ) {
                return true;
            }
            // legacy check
            return omt_str.find( "faction_base_camp" ) != std::string::npos;
        } else if( location == "FACTION_CAMP_START" ) {
            return !recipe_group::get_recipes_by_id( "all_faction_base_types", omt_str ).empty();
        } else {
            return oter_no_dir( omt_ter ) == location;
        }
    };
}

template<class T>
void conditional_t<T>::set_near_om_location( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const std::string &location = jo.get_string( member );
    const int_or_var<T> range = get_int_or_var<T>( jo, "range", false, 1 );
    condition = [location, range, is_npc]( const T & d ) {
        const tripoint_abs_omt omt_pos = d.actor( is_npc )->global_omt_location();
        for( const tripoint_abs_omt &curr_pos : points_in_radius( omt_pos,
                range.evaluate( d ) ) ) {
            const oter_id &omt_ter = overmap_buffer.ter( curr_pos );
            const std::string &omt_str = omt_ter.id().c_str();

            if( location == "FACTION_CAMP_ANY" ) {
                cata::optional<basecamp *> bcp = overmap_buffer.find_camp( curr_pos.xy() );
                if( bcp ) {
                    return true;
                }
                // legacy check
                if( omt_str.find( "faction_base_camp" ) != std::string::npos ) {
                    return true;
                }
            } else if( location == "FACTION_CAMP_START" &&
                       !recipe_group::get_recipes_by_id( "all_faction_base_types", omt_str ).empty() ) {
                return true;
            } else {
                if( oter_no_dir( omt_ter ) == location ) {
                    return true;
                }
            }
        }
        // should never get here this is for safety
        return false;
    };
}

template<class T>
void conditional_t<T>::set_has_var( const JsonObject &jo, const std::string &member, bool is_npc )
{
    int_or_var<dialogue> empty;
    const std::string var_name = get_talk_varname( jo, member, false, empty );
    const std::string &value = jo.has_member( "value" ) ? jo.get_string( "value" ) : std::string();
    const bool time_check = jo.has_member( "time" ) && jo.get_bool( "time" );
    condition = [var_name, value, time_check, is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
        if( time_check ) {
            return !actor->get_value( var_name ).empty();
        }
        return actor->get_value( var_name ) == value;
    };
}

template<class T>
void conditional_t<T>::set_compare_var( const JsonObject &jo, const std::string &member,
                                        bool is_npc )
{
    int_or_var<dialogue> empty;
    const std::string var_name = get_talk_varname( jo, member, false, empty );
    const std::string &op = jo.get_string( "op" );

    int_or_var<T> iov = get_int_or_var<T>( jo, "value" );
    condition = [var_name, op, iov, is_npc]( const T & d ) {
        int stored_value = 0;
        int value = iov.evaluate( d );
        const std::string &var = d.actor( is_npc )->get_value( var_name );
        if( !var.empty() ) {
            stored_value = std::stoi( var );
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

template<class T>
void conditional_t<T>::set_compare_time_since_var( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    int_or_var<dialogue> empty;
    const std::string var_name = get_talk_varname( jo, member, false, empty );
    const std::string &op = jo.get_string( "op" );
    const int value = to_turns<int>( read_from_json_string<time_duration>( jo.get_member( "time" ),
                                     time_duration::units ) );
    condition = [var_name, op, value, is_npc]( const T & d ) {
        int stored_value = 0;
        const std::string &var = d.actor( is_npc )->get_value( var_name );
        if( var.empty() ) {
            return false;
        } else {
            stored_value = std::stoi( var );
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

template<class T>
void conditional_t<T>::set_npc_role_nearby( const JsonObject &jo )
{
    const std::string &role = jo.get_string( "npc_role_nearby" );
    condition = [role]( const T & d ) {
        const std::vector<npc *> available = g->get_npcs_if( [&]( const npc & guy ) {
            return d.actor( false )->posz() == guy.posz() && guy.companion_mission_role_id == role &&
                   ( rl_dist( d.actor( false )->pos(), guy.pos() ) <= 48 );
        } );
        return !available.empty();
    };
}

template<class T>
void conditional_t<T>::set_npc_allies( const JsonObject &jo )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, "npc_allies" );
    condition = [iov]( const T & d ) {
        return g->allies().size() >= static_cast<std::vector<npc *>::size_type>( iov.evaluate( d ) );
    };
}


template<class T>
void conditional_t<T>::set_npc_allies_global( const JsonObject &jo )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, "npc_allies_global" );

    condition = [iov]( const T & d ) {
        const auto all_npcs = overmap_buffer.get_overmap_npcs();
        const size_t count = std::count_if( all_npcs.begin(),
        all_npcs.end(), []( const shared_ptr_fast<npc> &ptr ) {
            return ptr.get()->is_player_ally() && !ptr.get()->hallucination && !ptr.get()->is_dead();
        } );

        return count >= static_cast<size_t>( iov.evaluate( d ) );
    };
}


template<class T>
void conditional_t<T>::set_u_has_cash( const JsonObject &jo )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, "u_has_cash" );
    condition = [iov]( const T & d ) {
        return d.actor( false )->cash() >= iov.evaluate( d );
    };
}

template<class T>
void conditional_t<T>::set_u_are_owed( const JsonObject &jo )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, "u_are_owed" );
    condition = [iov]( const T & d ) {
        return d.actor( true )->debt() >= iov.evaluate( d );
    };
}

template<class T>
void conditional_t<T>::set_npc_aim_rule( const JsonObject &jo, bool is_npc )
{
    const std::string &setting = jo.get_string( "npc_aim_rule" );
    condition = [setting, is_npc]( const T & d ) {
        return d.actor( is_npc )->has_ai_rule( "aim_rule", setting );
    };
}

template<class T>
void conditional_t<T>::set_npc_engagement_rule( const JsonObject &jo, bool is_npc )
{
    const std::string &setting = jo.get_string( "npc_engagement_rule" );
    condition = [setting, is_npc]( const T & d ) {
        return d.actor( is_npc )->has_ai_rule( "engagement_rule", setting );
    };
}

template<class T>
void conditional_t<T>::set_npc_cbm_reserve_rule( const JsonObject &jo, bool is_npc )
{
    const std::string &setting = jo.get_string( "npc_cbm_reserve_rule" );
    condition = [setting, is_npc]( const T & d ) {
        return d.actor( is_npc )->has_ai_rule( "cbm_reserve_rule", setting );
    };
}

template<class T>
void conditional_t<T>::set_npc_cbm_recharge_rule( const JsonObject &jo, bool is_npc )
{
    const std::string &setting = jo.get_string( "npc_cbm_recharge_rule" );
    condition = [setting, is_npc]( const T & d ) {
        return d.actor( is_npc )->has_ai_rule( "cbm_recharge_rule", setting );
    };
}

template<class T>
void conditional_t<T>::set_npc_rule( const JsonObject &jo, bool is_npc )
{
    std::string rule = jo.get_string( "npc_rule" );
    condition = [rule, is_npc]( const T & d ) {
        return d.actor( is_npc )->has_ai_rule( "ally_rule", rule );
    };
}

template<class T>
void conditional_t<T>::set_npc_override( const JsonObject &jo, bool is_npc )
{
    std::string rule = jo.get_string( "npc_override" );
    condition = [rule, is_npc]( const T & d ) {
        return d.actor( is_npc )->has_ai_rule( "ally_override", rule );
    };
}

template<class T>
void conditional_t<T>::set_days_since( const JsonObject &jo )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, "days_since_cataclysm" );
    condition = [iov]( const T & d ) {
        return calendar::turn >= calendar::start_of_cataclysm + 1_days * iov.evaluate( d );
    };
}

template<class T>
void conditional_t<T>::set_is_season( const JsonObject &jo )
{
    std::string season_name = jo.get_string( "is_season" );
    condition = [season_name]( const T & ) {
        const season_type season = season_of_year( calendar::turn );
        return ( season == SPRING && season_name == "spring" ) ||
               ( season == SUMMER && season_name == "summer" ) ||
               ( season == AUTUMN && season_name == "autumn" ) ||
               ( season == WINTER && season_name == "winter" );
    };
}

template<class T>
void conditional_t<T>::set_mission_goal( const JsonObject &jo, bool is_npc )
{
    std::string mission_goal_str = jo.get_string( "mission_goal" );
    condition = [mission_goal_str, is_npc]( const T & d ) {
        mission *miss = d.actor( is_npc )->selected_mission();
        if( !miss ) {
            return false;
        }
        const mission_goal mgoal = io::string_to_enum<mission_goal>( mission_goal_str );
        return miss->get_type().goal == mgoal;
    };
}

template<class T>
void conditional_t<T>::set_is_gender( bool is_male, bool is_npc )
{
    condition = [is_male, is_npc]( const T & d ) {
        return d.actor( is_npc )->is_male() == is_male;
    };
}

template<class T>
void conditional_t<T>::set_no_assigned_mission()
{
    condition = []( const T & d ) {
        return d.missions_assigned.empty();
    };
}

template<class T>
void conditional_t<T>::set_has_assigned_mission()
{
    condition = []( const T & d ) {
        return d.missions_assigned.size() == 1;
    };
}

template<class T>
void conditional_t<T>::set_has_many_assigned_missions()
{
    condition = []( const T & d ) {
        return d.missions_assigned.size() >= 2;
    };
}

template<class T>
void conditional_t<T>::set_no_available_mission( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return d.actor( is_npc )->available_missions().empty();
    };
}

template<class T>
void conditional_t<T>::set_has_available_mission( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return d.actor( is_npc )->available_missions().size() == 1;
    };
}

template<class T>
void conditional_t<T>::set_has_many_available_missions( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return d.actor( is_npc )->available_missions().size() >= 2;
    };
}

template<class T>
void conditional_t<T>::set_mission_complete( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        mission *miss = d.actor( is_npc )->selected_mission();
        return miss && miss->is_complete( d.actor( is_npc )->getID() );
    };
}

template<class T>
void conditional_t<T>::set_mission_incomplete( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        mission *miss = d.actor( is_npc )->selected_mission();
        return miss && !miss->is_complete( d.actor( is_npc )->getID() );
    };
}

template<class T>
void conditional_t<T>::set_npc_available( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return !d.actor( is_npc )->has_effect( effect_currently_busy, bodypart_str_id::NULL_ID() );
    };
}

template<class T>
void conditional_t<T>::set_npc_following( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return d.actor( is_npc )->is_following();
    };
}

template<class T>
void conditional_t<T>::set_npc_friend( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return d.actor( is_npc )->is_friendly( get_player_character() );
    };
}

template<class T>
void conditional_t<T>::set_npc_hostile( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return d.actor( is_npc )->is_enemy();
    };
}

template<class T>
void conditional_t<T>::set_npc_train_skills( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return !d.actor( is_npc )->skills_offered_to( *d.actor( !is_npc ) ).empty();
    };
}

template<class T>
void conditional_t<T>::set_npc_train_styles( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return !d.actor( is_npc )->styles_offered_to( *d.actor( !is_npc ) ).empty();
    };
}

template<class T>
void conditional_t<T>::set_npc_train_spells( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return !d.actor( is_npc )->spells_offered_to( *d.actor( !is_npc ) ).empty();
    };
}

template<class T>
void conditional_t<T>::set_at_safe_space( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return overmap_buffer.is_safe( d.actor( is_npc )->global_omt_location() ) &&
               d.actor( is_npc )->is_safe();
    };
}

template<class T>
void conditional_t<T>::set_can_stow_weapon( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
        return !actor->unarmed_attack() && actor->can_stash_weapon();
    };
}

template<class T>
void conditional_t<T>::set_has_weapon( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return !d.actor( is_npc )->unarmed_attack();
    };
}

template<class T>
void conditional_t<T>::set_is_driving( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
        if( const optional_vpart_position vp = get_map().veh_at( actor->pos() ) ) {
            return vp->vehicle().is_moving() && actor->is_in_control_of( vp->vehicle() );
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_has_stolen_item( bool /*is_npc*/ )
{
    condition = []( const T & d ) {
        return d.actor( false )->has_stolen_item( *d.actor( true ) );
    };
}

template<class T>
void conditional_t<T>::set_is_day()
{
    condition = []( const T & ) {
        return !is_night( calendar::turn );
    };
}

template<class T>
void conditional_t<T>::set_is_outside( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return is_creature_outside( *d.actor( is_npc )->get_creature() );
    };
}

template<class T>
void conditional_t<T>::set_is_underwater( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return get_map().is_divable( d.actor( is_npc )->pos() );
    };
}

template<class T>
void conditional_t<T>::set_one_in_chance( const JsonObject &jo, const std::string &member )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, member );
    condition = [iov]( const T & d ) {
        return one_in( iov.evaluate( d ) );
    };
}

template<class T>
void conditional_t<T>::set_query( const JsonObject &jo, const std::string &member, bool is_npc )
{
    std::string message = jo.get_string( member );
    bool default_val = jo.get_bool( "default" );
    condition = [message, default_val, is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
        if( actor->get_character() && actor->get_character()->is_avatar() ) {
            std::string translated_message = _( message );
            return query_yn( translated_message );
        } else {
            return default_val;
        }
    };
}

template<class T>
void conditional_t<T>::set_x_in_y_chance( const JsonObject &jo, const std::string &member )
{
    const JsonObject &var_obj = jo.get_object( member );
    int_or_var<T> iovx = get_int_or_var<T>( var_obj, "x" );
    int_or_var<T> iovy = get_int_or_var<T>( var_obj, "y" );
    condition = [iovx, iovy]( const T & d ) {
        return x_in_y( iovx.evaluate( d ),
                       iovy.evaluate( d ) );
    };
}

template<class T>
void conditional_t<T>::set_is_weather( const JsonObject &jo )
{
    weather_type_id weather = weather_type_id( jo.get_string( "is_weather" ) );
    condition = [weather]( const T & ) {
        return get_weather().weather_id == weather;
    };
}

template<class T>
void conditional_t<T>::set_has_faction_trust( const JsonObject &jo, const std::string &member )
{
    int_or_var<T> iov = get_int_or_var<T>( jo, member );
    condition = [iov]( const T & d ) {
        return d.actor( true )->get_faction()->trusts_u >= iov.evaluate( d );
    };
}

static std::string get_string_from_input( JsonArray objects, int index )
{
    if( objects.has_string( index ) ) {
        std::string type = objects.get_string( index );
        if( type == "u" || type == "npc" ) {
            return type;
        }
    }
    int_or_var<dialogue> empty;
    JsonObject object = objects.get_object( index );
    if( object.has_string( "u_val" ) ) {
        return "u_" + get_talk_varname( object, "u_val", false, empty );
    } else if( object.has_string( "npc_val" ) ) {
        return "npc_" + get_talk_varname( object, "npc_val", false, empty );
    } else if( object.has_string( "global_val" ) ) {
        return "global_" + get_talk_varname( object, "global_val", false, empty );
    } else if( object.has_string( "faction_val" ) ) {
        return "faction_" + get_talk_varname( object, "faction_val", false, empty );
    } else if( object.has_string( "party_val" ) ) {
        return "party_" + get_talk_varname( object, "party_val", false, empty );
    }
    object.throw_error( "Invalid input type." );
    return "";
}

template<class T>
static tripoint_abs_ms get_tripoint_from_string( std::string type, T &d )
{
    if( type == "u" ) {
        return get_map().getglobal( d.actor( false )->pos() );
    } else if( type == "npc" ) {
        return get_map().getglobal( d.actor( true )->pos() );
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
    }
    return tripoint_abs_ms();
}

template<class T>
void conditional_t<T>::set_compare_string( const JsonObject &jo, const std::string &member )
{
    str_or_var<T> first;
    str_or_var<T> second;
    JsonArray objects = jo.get_array( member );
    if( objects.size() != 2 ) {
        jo.throw_error( "incorrect number of values.  Expected 2 in " + jo.str() );
        condition = []( const T & ) {
            return false;
        };
        return;
    }

    if( objects.has_object( 0 ) ) {
        first = get_str_or_var<T>( objects.next(), member, true );
    } else {
        first.str_val = objects.next_string();
    }
    if( objects.has_object( 1 ) ) {
        second = get_str_or_var<T>( objects.next(), member, true );
    } else {
        second.str_val = objects.next_string();
    }

    condition = [first, second]( const T & d ) {
        return first.evaluate( d ) == second.evaluate( d );
    };
}

template<class T>
void conditional_t<T>::set_compare_int( const JsonObject &jo, const std::string &member )
{
    JsonArray objects = jo.get_array( member );
    if( objects.size() != 3 ) {
        jo.throw_error( "incorrect number of values.  Expected three in " + jo.str() );
        condition = []( const T & ) {
            return false;
        };
        return;
    }
    std::function<int( const T & )> get_first_int = objects.has_object( 0 ) ? get_get_int(
                objects.get_object( 0 ) ) : get_get_int( objects.get_string( 0 ), jo );
    std::function<int( const T & )> get_second_int = objects.has_object( 2 ) ? get_get_int(
                objects.get_object( 2 ) ) : get_get_int( objects.get_string( 2 ), jo );
    const std::string &op = objects.get_string( 1 );

    if( op == "==" || op == "=" ) {
        condition = [get_first_int, get_second_int]( const T & d ) {
            return get_first_int( d ) == get_second_int( d );
        };
    } else if( op == "!=" ) {
        condition = [get_first_int, get_second_int]( const T & d ) {
            return get_first_int( d ) != get_second_int( d );
        };
    } else if( op == "<=" ) {
        condition = [get_first_int, get_second_int]( const T & d ) {
            return get_first_int( d ) <= get_second_int( d );
        };
    } else if( op == ">=" ) {
        condition = [get_first_int, get_second_int]( const T & d ) {
            return get_first_int( d ) >= get_second_int( d );
        };
    } else if( op == "<" ) {
        condition = [get_first_int, get_second_int]( const T & d ) {
            return get_first_int( d ) < get_second_int( d );
        };
    } else if( op == ">" ) {
        condition = [get_first_int, get_second_int]( const T & d ) {
            return get_first_int( d ) > get_second_int( d );
        };
    } else {
        jo.throw_error( "unexpected operator " + jo.get_string( "op" ) + " in " + jo.str() );
        condition = []( const T & ) {
            return false;
        };
    }
}

template<class T>
std::function<int( const T & )> conditional_t<T>::get_get_int( const JsonObject &jo )
{
    if( jo.has_member( "const" ) ) {
        const int const_value = jo.get_int( "const" );
        return [const_value]( const T & ) {
            return const_value;
        };
    } else if( jo.has_member( "time" ) ) {
        const int value = to_turns<int>( read_from_json_string<time_duration>( jo.get_member( "time" ),
                                         time_duration::units ) );
        return [value]( const T & ) {
            return value;
        };
    } else if( jo.has_member( "power" ) ) {
        units::energy power;
        assign( jo, "power", power, false, 0_kJ );
        const int power_value = units::to_millijoule( power );
        return [power_value]( const T & ) {
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
        return [given_unit]( const T & ) {
            return ( to_turn<int>( calendar::turn ) - to_turn<int>( calendar::start_of_cataclysm ) ) /
                   to_turns<int>( given_unit );
        };
    } else if( jo.has_member( "rand" ) ) {
        int max_value = jo.get_int( "rand" );
        return [max_value]( const T & ) {
            return rng( 0, max_value );
        };
    } else if( jo.has_member( "weather" ) ) {
        std::string weather_aspect = jo.get_string( "weather" );
        if( weather_aspect == "temperature" ) {
            return []( const T & ) {
                return static_cast<int>( get_weather().weather_precise->temperature );
            };
        } else if( weather_aspect == "windpower" ) {
            return []( const T & ) {
                return static_cast<int>( get_weather().weather_precise->windpower );
            };
        } else if( weather_aspect == "humidity" ) {
            return []( const T & ) {
                return static_cast<int>( get_weather().weather_precise->humidity );
            };
        } else if( weather_aspect == "pressure" ) {
            return []( const T & ) {
                return static_cast<int>( get_weather().weather_precise->pressure );
            };
        }
    } else if( jo.has_member( "u_val" ) || jo.has_member( "npc_val" ) ||
               jo.has_member( "global_val" ) ) {
        const bool is_npc = jo.has_member( "npc_val" );
        const bool is_global = jo.has_member( "global_val" );
        const std::string checked_value = is_npc ? jo.get_string( "npc_val" ) :
                                          ( is_global ? jo.get_string( "global_val" ) : jo.get_string( "u_val" ) );
        if( checked_value == "strength" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->str_cur();
            };
        } else if( checked_value == "dexterity" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->dex_cur();
            };
        } else if( checked_value == "intelligence" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->int_cur();
            };
        } else if( checked_value == "perception" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->per_cur();
            };
        } else if( checked_value == "strength_base" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_str_max();
            };
        } else if( checked_value == "dexterity_base" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_dex_max();
            };
        } else if( checked_value == "intelligence_base" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_int_max();
            };
        } else if( checked_value == "perception_base" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_per_max();
            };
        } else if( checked_value == "strength_bonus" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_str_bonus();
            };
        } else if( checked_value == "dexterity_bonus" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_dex_bonus();
            };
        } else if( checked_value == "intelligence_bonus" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_int_bonus();
            };
        } else if( checked_value == "perception_bonus" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_per_bonus();
            };
        } else if( checked_value == "hp" ) {
            cata::optional<bodypart_id> bp;
            optional( jo, false, "bodypart", bp );
            return [is_npc, bp]( const T & d ) {
                bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
                return d.actor( is_npc )->get_cur_hp( bid );
            };
        } else if( checked_value == "effect_intensity" ) {
            const std::string &effect_id = jo.get_string( "effect" );
            cata::optional<bodypart_id> bp;
            optional( jo, false, "bodypart", bp );
            return [effect_id, bp, is_npc]( const T & d ) {
                bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
                effect target = d.actor( is_npc )->get_effect( efftype_id( effect_id ), bid );
                return target.is_null() ? -1 : target.get_intensity();
            };
        } else if( checked_value == "var" ) {
            var_info info = read_var_info( jo, false );
            return [info]( const T & d ) {
                std::string var = read_var_value( info, d );
                if( !var.empty() ) {
                    return std::stoi( var );
                } else {
                    try {
                        return std::stoi( info.default_val );
                    } catch( const std::exception & ) {
                        return 0;
                    }
                }
            };
        } else if( checked_value == "time_since_var" ) {
            int_or_var<dialogue> empty;
            const std::string var_name = get_talk_varname( jo, "var_name", false, empty );
            return [is_npc, var_name]( const T & d ) {
                int stored_value = 0;
                const std::string &var = d.actor( is_npc )->get_value( var_name );
                if( !var.empty() ) {
                    stored_value = std::stoi( var );
                }
                return to_turn<int>( calendar::turn ) - stored_value;
            };
        } else if( checked_value == "allies" ) {
            if( is_npc ) {
                jo.throw_error( "allies count not supported for NPCs.  In " + jo.str() );
            } else {
                return []( const T & ) {
                    return static_cast<int>( g->allies().size() );
                };
            }
        } else if( checked_value == "cash" ) {
            if( is_npc ) {
                jo.throw_error( "cash count not supported for NPCs.  In " + jo.str() );
            } else {
                return [is_npc]( const T & d ) {
                    return d.actor( is_npc )->cash();
                };
            }
        } else if( checked_value == "owed" ) {
            if( is_npc ) {
                jo.throw_error( "owed amount not supported for NPCs.  In " + jo.str() );
            } else {
                return []( const T & d ) {
                    return d.actor( true )->debt();
                };
            }
        } else if( checked_value == "sold" ) {
            if( is_npc ) {
                jo.throw_error( "owed amount not supported for NPCs.  In " + jo.str() );
            } else {
                return []( const T & d ) {
                    return d.actor( true )->sold();
                };
            }
        } else if( checked_value == "skill_level" ) {
            const skill_id skill( jo.get_string( "skill" ) );
            return [is_npc, skill]( const T & d ) {
                return d.actor( is_npc )->get_skill_level( skill );
            };
        } else if( checked_value == "pos_x" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->posx();
            };
        } else if( checked_value == "pos_y" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->posy();
            };
        } else if( checked_value == "pos_z" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->posz();
            };
        } else if( checked_value == "pain" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->pain_cur();
            };
        } else if( checked_value == "power" ) {
            return [is_npc]( const T & d ) {
                // Energy in milijoule
                return static_cast<int>( d.actor( is_npc )->power_cur().value() );
            };
        } else if( checked_value == "power_max" ) {
            return [is_npc]( const T & d ) {
                // Energy in milijoule
                return static_cast<int>( d.actor( is_npc )->power_max().value() );
            };
        } else if( checked_value == "power_percentage" ) {
            return [is_npc]( const T & d ) {
                // Energy in milijoule
                int power_max = d.actor( is_npc )->power_max().value();
                if( power_max == 0 ) {
                    return 0; //Default value if character does not have power, avoids division with 0.
                } else {
                    return static_cast<int>( d.actor( is_npc )->power_cur().value() * 100 ) / power_max;
                }
            };
        } else if( checked_value == "morale" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->morale_cur();
            };
        } else if( checked_value == "focus" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->focus_cur();
            };
        } else if( checked_value == "mana" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->mana_cur();
            };
        } else if( checked_value == "mana_max" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->mana_max();
            };
        } else if( checked_value == "mana_percentage" ) {
            return [is_npc]( const T & d ) {
                int mana_max = d.actor( is_npc )->mana_max();
                if( mana_max == 0 ) {
                    return 0; //Default value if character does not have mana, avoids division with 0.
                } else {
                    return ( d.actor( is_npc )->mana_cur() * 100 ) / mana_max;
                }
            };
        } else if( checked_value == "hunger" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_hunger();
            };
        } else if( checked_value == "thirst" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_thirst();
            };
        } else if( checked_value == "instant_thirst" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_instant_thirst();
            };
        } else if( checked_value == "stored_kcal" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_stored_kcal();
            };
        } else if( checked_value == "stored_kcal_percentage" ) {
            // 100% is 55'000 kcal, which is considered healthy.
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_stored_kcal() / 550;
            };
        } else if( checked_value == "item_count" ) {
            const itype_id item_id( jo.get_string( "item" ) );
            return [is_npc, item_id]( const T & d ) {
                return std::max( d.actor( is_npc )->charges_of( item_id ),
                                 d.actor( is_npc )->get_amount( item_id ) );
            };
        } else if( checked_value == "exp" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_kill_xp();
            };
        } else if( checked_value == "addiction_intensity" ) {
            const addiction_id add_id( jo.get_string( "addiction" ) );
            if( jo.has_object( "mod" ) ) {
                // final_value = (val / (val - step * intensity)) - 1
                JsonObject jobj = jo.get_object( "mod" );
                const int val = jobj.get_int( "val", 0 );
                const int step = jobj.get_int( "step", 0 );
                return [is_npc, add_id, val, step]( const T & d ) {
                    int intens = d.actor( is_npc )->get_addiction_intensity( add_id );
                    int denom = val - step * intens;
                    return denom == 0 ? 0 : ( val / std::max( 1, denom ) - 1 );
                };
            }
            const int mod = jo.get_int( "mod", 1 );
            return [is_npc, add_id, mod]( const T & d ) {
                return d.actor( is_npc )->get_addiction_intensity( add_id ) * mod;
            };
        } else if( checked_value == "addiction_turns" ) {
            const addiction_id add_id( jo.get_string( "addiction" ) );
            return [is_npc, add_id]( const T & d ) {
                return d.actor( is_npc )->get_addiction_turns( add_id );
            };
        } else if( checked_value == "stim" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_stim();
            };
        } else if( checked_value == "pkill" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_pkill();
            };
        } else if( checked_value == "rad" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_rad();
            };
        } else if( checked_value == "focus" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->focus_cur();
            };
        } else if( checked_value == "activity_level" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_activity_level();
            };
        } else if( checked_value == "fatigue" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_fatigue();
            };
        } else if( checked_value == "stamina" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_stamina();
            };
        } else if( checked_value == "sleep_deprivation" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_sleep_deprivation();
            };
        } else if( checked_value == "anger" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_anger();
            };
        } else if( checked_value == "friendly" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_friendly();
            };
        } else if( checked_value == "vitamin" ) {
            std::string vitamin_name = jo.get_string( "name" );
            return [is_npc, vitamin_name]( const T & d ) {
                Character *you = d.actor( is_npc )->get_character();
                if( you ) {
                    return you->vitamin_get( vitamin_id( vitamin_name ) );
                } else {
                    return 0;
                }
            };
        } else if( checked_value == "age" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_age();
            };
        } else if( checked_value == "height" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_height();
            };
        } else if( checked_value == "bmi_permil" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_bmi_permil();
            };
        } else if( checked_value == "fine_detail_vision_mod" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_fine_detail_vision_mod();
            };
        } else if( checked_value == "health" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_health();
            };
        } else if( checked_value == "body_temp" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_body_temp();
            };
        } else if( checked_value == "body_temp_delta" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_body_temp_delta();
            };
        } else if( checked_value == "npc_trust" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_npc_trust();
            };
        } else if( checked_value == "npc_fear" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_npc_fear();
            };
        } else if( checked_value == "npc_value" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_npc_value();
            };
        } else if( checked_value == "npc_anger" ) {
            return [is_npc]( const T & d ) {
                return d.actor( is_npc )->get_npc_anger();
            };
        } else if( checked_value == "monsters_nearby" ) {
            cata::optional<var_info> target_var;
            if( jo.has_object( "target_var" ) ) {
                read_var_info( jo.get_member( "target_var" ), false );
            }
            str_or_var<T> id;
            if( jo.has_member( "id" ) ) {
                id = get_str_or_var<T>( jo.get_member( "id" ), "id", false, "" );
            } else {
                id.str_val = "";
            }
            int_or_var<T> radius_iov = get_int_or_var<T>( jo, "radius", false, 10000 );
            int_or_var<T> number_iov = get_int_or_var<T>( jo, "number", false, 1 );
            return [target_var, radius_iov, id, number_iov, is_npc]( const T & d ) {
                tripoint_abs_ms loc;
                if( target_var.has_value() ) {
                    loc = get_tripoint_from_var( target_var, d );
                } else {
                    loc = d.actor( is_npc )->global_pos();
                }

                int radius = radius_iov.evaluate( d );
                std::vector<Creature *> targets = g->get_creatures_if( [&radius, id, &d,
                         loc]( const Creature & critter ) {
                    if( critter.is_monster() ) {
                        // friendly to the player, not a target for us
                        return static_cast<const monster *>( &critter )->friendly == 0 &&
                               radius >= rl_dist( critter.get_location(), loc ) &&
                               ( id.evaluate( d ) == "" ||
                                 static_cast<const monster *>( &critter )->type->id == mtype_id( id.evaluate( d ) ) );
                    }
                    return false;
                } );
                return targets.size();
            };
        }
    } else if( jo.has_member( "moon" ) ) {
        return []( const T & ) {
            return static_cast<int>( get_moon_phase( calendar::turn ) );
        };
    } else if( jo.has_member( "hour" ) ) {
        return []( const T & ) {
            return to_hours<int>( time_past_midnight( calendar::turn ) );
        };
    } else if( jo.has_array( "distance" ) ) {
        JsonArray objects = jo.get_array( "distance" );
        if( objects.size() != 2 ) {
            objects.throw_error( "distance requires an array with 2 elements." );
        }
        std::string first = get_string_from_input( objects, 0 );
        std::string second = get_string_from_input( objects, 1 );
        return [first, second]( const T & d ) {
            tripoint_abs_ms first_point = get_tripoint_from_string( first, d );
            tripoint_abs_ms second_point = get_tripoint_from_string( second, d );
            return rl_dist( first_point, second_point );
        };
    } else if( jo.has_array( "arithmetic" ) ) {
        talk_effect_fun_t<T> arith;
        arith.set_arithmetic( jo, "arithmetic", true );
        return [arith]( const T & d ) {
            arith( d );
            var_info info = var_info( var_type::global, "temp_var" );
            std::string val = read_var_value( info, d );
            if( !val.empty() ) {
                return std::stoi( val );
            } else {
                debugmsg( "No valid value." );
                return 0;
            }
        };
    }
    jo.throw_error( "unrecognized integer source in " + jo.str() );
    return []( const T & ) {
        return 0;
    };
}

template<class T>
std::function<int( const T & )> conditional_t<T>::get_get_int( std::string value,
        const JsonObject &jo )
{
    if( value == "moon" ) {
        return []( const T & ) {
            return static_cast<int>( get_moon_phase( calendar::turn ) );
        };
    } else if( value == "hour" ) {
        return []( const T & ) {
            return to_hours<int>( time_past_midnight( calendar::turn ) );
        };
    }
    jo.throw_error( "unrecognized integer source in " + value );
    return []( const T & ) {
        return 0;
    };
}

template<class T>
static int handle_min_max( const T &d, int input, cata::optional<int_or_var_part<T>> min,
                           cata::optional<int_or_var_part<T>> max )
{
    if( min.has_value() ) {
        int min_val = min.value().evaluate( d );
        input = std::max( min_val, input );
    }
    if( max.has_value() ) {
        int max_val = max.value().evaluate( d );
        input = std::min( max_val, input );
    }
    return input;
}

template<class T>
static std::function<void( const T &, int )> get_set_int( const JsonObject &jo,
        cata::optional<int_or_var_part<T>> min, cata::optional<int_or_var_part<T>> max, bool temp_var )
{
    if( temp_var ) {
        jo.allow_omitted_members();
        return [min, max]( const T & d, int input ) {
            write_var_value( var_type::global, "temp_var", d.actor( false ),
                             std::to_string( handle_min_max<T>( d,
                                             input, min,
                                             max ) ) );
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
        return [given_unit, min, max]( const T & d, int input ) {
            calendar::turn = time_point( handle_min_max<T>( d, input, min,
                                         max ) * to_turns<int>( given_unit ) );
        };
    } else if( jo.has_member( "rand" ) ) {
        jo.throw_error( "can not alter the random number generator, silly!  In " + jo.str() );
    } else if( jo.has_member( "weather" ) ) {
        std::string weather_aspect = jo.get_string( "weather" );
        if( weather_aspect == "temperature" ) {
            return [min, max]( const T & d, int input ) {
                const int new_temperature = handle_min_max<T>( d, input, min, max );
                get_weather().weather_precise->temperature = new_temperature;
                get_weather().temperature = new_temperature;
                get_weather().clear_temp_cache();
            };
        } else if( weather_aspect == "windpower" ) {
            return [min, max]( const T & d, int input ) {
                get_weather().weather_precise->windpower = handle_min_max<T>( d, input, min, max );
                get_weather().clear_temp_cache();
            };
        } else if( weather_aspect == "humidity" ) {
            return [min, max]( const T & d, int input ) {
                get_weather().weather_precise->humidity = handle_min_max<T>( d, input, min, max );
                get_weather().clear_temp_cache();
            };
        } else if( weather_aspect == "pressure" ) {
            return [min, max]( const T & d, int input ) {
                get_weather().weather_precise->pressure = handle_min_max<T>( d, input, min, max );
                get_weather().clear_temp_cache();
            };
        }
    } else if( jo.has_member( "u_val" ) || jo.has_member( "npc_val" ) ||
               jo.has_member( "global_val" ) || jo.has_member( "faction_val" ) || jo.has_member( "party_val" ) ) {
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
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_str_max( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "dexterity_base" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_dex_max( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "intelligence_base" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_int_max( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "perception_base" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_per_max( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "strength_bonus" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_str_max( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "dexterity_bonus" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_dex_max( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "intelligence_bonus" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_int_max( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "perception_bonus" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_per_max( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "var" ) {
            int_or_var<dialogue> empty;
            const std::string var_name = get_talk_varname( jo, "var_name", false, empty );
            return [is_npc, var_name, type, min, max]( const T & d, int input ) {
                write_var_value( type, var_name, d.actor( is_npc ), std::to_string( handle_min_max<T>( d, input,
                                 min,
                                 max ) ) );
            };
        } else if( checked_value == "time_since_var" ) {
            // This is a strange thing to want to adjust. But we allow it nevertheless.
            int_or_var<dialogue> empty;
            const std::string var_name = get_talk_varname( jo, "var_name", false, empty );
            return [is_npc, var_name, min, max]( const T & d, int input ) {
                int storing_value = to_turn<int>( calendar::turn ) - handle_min_max<T>( d, input, min, max );
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
                return [min, max]( const T & d, int input ) {
                    d.actor( true )->add_debt( handle_min_max<T>( d, input, min, max ) - d.actor( true )->debt() );
                };
            }
        } else if( checked_value == "sold" ) {
            if( is_npc ) {
                jo.throw_error( "sold amount not supported for NPCs.  In " + jo.str() );
            } else {
                return [min, max]( const T & d, int input ) {
                    d.actor( true )->add_sold( handle_min_max<T>( d, input, min, max ) - d.actor( true )->sold() );
                };
            }
        } else if( checked_value == "skill_level" ) {
            const skill_id skill( jo.get_string( "skill" ) );
            return [is_npc, skill, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_skill_level( skill, handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "pos_x" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_pos( tripoint( handle_min_max<T>( d, input, min, max ),
                                                      d.actor( is_npc )->posy(),
                                                      d.actor( is_npc )->posz() ) );
            };
        } else if( checked_value == "pos_y" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_pos( tripoint( d.actor( is_npc )->posx(), handle_min_max<T>( d, input, min,
                                                      max ),
                                                      d.actor( is_npc )->posz() ) );
            };
        } else if( checked_value == "pos_z" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_pos( tripoint( d.actor( is_npc )->posx(), d.actor( is_npc )->posy(),
                                                      handle_min_max<T>( d, input, min, max ) ) );
            };
        } else if( checked_value == "pain" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->mod_pain( handle_min_max<T>( d, input, min,
                                             max ) - d.actor( is_npc )->pain_cur() );
            };
        } else if( checked_value == "power" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                // Energy in milijoule
                d.actor( is_npc )->set_power_cur( 1_mJ * handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "power_max" ) {
            jo.throw_error( "altering max power this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "power_percentage" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                // Energy in milijoule
                d.actor( is_npc )->set_power_cur( ( d.actor( is_npc )->power_max() * handle_min_max<T>( d, input,
                                                    min,
                                                    max ) ) / 100 );
            };
        } else if( checked_value == "focus" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->mod_focus( handle_min_max<T>( d, input, min,
                                              max ) - d.actor( is_npc )->focus_cur() );
            };
        } else if( checked_value == "mana" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_mana_cur( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "mana_max" ) {
            jo.throw_error( "altering max mana this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "mana_percentage" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_mana_cur( ( d.actor( is_npc )->mana_max() * handle_min_max<T>( d, input, min,
                                                   max ) ) / 100 );
            };
        } else if( checked_value == "hunger" ) {
            jo.throw_error( "altering hunger this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "thirst" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_thirst( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "stored_kcal" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_stored_kcal( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "stored_kcal_percentage" ) {
            // 100% is 55'000 kcal, which is considered healthy.
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_stored_kcal( handle_min_max<T>( d, input, min, max ) * 5500 );
            };
        } else if( checked_value == "item_count" ) {
            jo.throw_error( "altering items this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "exp" ) {
            jo.throw_error( "altering max exp this way is currently not supported.  In " + jo.str() );
        } else if( checked_value == "addiction_turns" ) {
            const addiction_id add_id( jo.get_string( "addiction" ) );
            return [is_npc, min, max, add_id]( const T & d, int input ) {
                d.actor( is_npc )->set_addiction_turns( add_id, handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "stim" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_stim( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "pkill" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_pkill( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "rad" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_rad( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "fatigue" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_fatigue( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "stamina" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_stamina( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "sleep_deprivation" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_sleep_deprivation( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "anger" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_anger( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "morale" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_morale( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "friendly" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_friendly( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "exp" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_kill_xp( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "vitamin" ) {
            std::string vitamin_name = jo.get_string( "name" );
            return [is_npc, min, max, vitamin_name]( const T & d, int input ) {
                Character *you = d.actor( is_npc )->get_character();
                if( you ) {
                    you->vitamin_set( vitamin_id( vitamin_name ), handle_min_max<T>( d, input, min, max ) );
                }
            };
        } else if( checked_value == "age" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_age( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "height" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_height( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "npc_trust" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_npc_trust( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "npc_fear" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_npc_fear( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "npc_value" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_npc_value( handle_min_max<T>( d, input, min, max ) );
            };
        } else if( checked_value == "npc_anger" ) {
            return [is_npc, min, max]( const T & d, int input ) {
                d.actor( is_npc )->set_npc_anger( handle_min_max<T>( d, input, min, max ) );
            };
        }
    }
    jo.throw_error( "error setting integer destination in " + jo.str() );
    return []( const T &, int ) {};
}

template<class T>
void talk_effect_fun_t<T>::set_arithmetic( const JsonObject &jo, const std::string &member,
        bool no_result )
{
    JsonArray objects = jo.get_array( member );
    cata::optional<int_or_var_part<T>> min;
    cata::optional<int_or_var_part<T>> max;
    if( jo.has_member( "min" ) ) {
        min = get_int_or_var_part<T>( jo.get_member( "min" ), "min" );
    } else if( jo.has_member( "min_time" ) ) {
        int_or_var_part<T> value;
        time_duration min_time;
        mandatory( jo, false, "min_time", min_time );
        value.int_val = to_turns<int>( min_time );
        min = value;
    }
    if( jo.has_member( "max" ) ) {
        max = get_int_or_var_part<T>( jo.get_member( "max" ), "max" );
    } else if( jo.has_member( "max_time" ) ) {
        int_or_var_part<T> value;
        time_duration max_time;
        mandatory( jo, false, "max_time", max_time );
        value.int_val = to_turns<int>( max_time );
        max = value;
    }
    std::string op = "none";
    std::string result = "none";
    std::function<void( const T &, int )> set_int = get_set_int( objects.get_object( 0 ), min,
            max, no_result );
    int no_result_mod = no_result ? 2 : 0; //In the case of a no result we have fewer terms.
    // Normal full version
    if( static_cast<int>( objects.size() ) == 5 - no_result_mod ) {
        op = objects.get_string( 3 - no_result_mod );
        if( !no_result ) {
            result = objects.get_string( 1 );
            if( result != "=" ) {
                jo.throw_error( "invalid result " + op + " in " + jo.str() );
                function = []( const T & ) {
                    return false;
                };
            }
        }
        std::function<int( const T & )> get_first_int = conditional_t< T >::get_get_int(
                    objects.get_object( 2 - no_result_mod ) );
        std::function<int( const T & )> get_second_int = conditional_t< T >::get_get_int(
                    objects.get_object( 4 - no_result_mod ) );
        if( op == "*" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) * get_second_int( d ) );
            };
        } else if( op == "/" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) / get_second_int( d ) );
            };
        } else if( op == "+" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) + get_second_int( d ) );
            };
        } else if( op == "-" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) - get_second_int( d ) );
            };
        } else if( op == "%" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) % get_second_int( d ) );
            };
        } else if( op == "&" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) & get_second_int( d ) );
            };
        } else if( op == "|" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) | get_second_int( d ) );
            };
        } else if( op == "<<" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) << get_second_int( d ) );
            };
        } else if( op == ">>" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) >> get_second_int( d ) );
            };
        } else if( op == "^" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) ^ get_second_int( d ) );
            };
        } else {
            jo.throw_error( "unexpected operator " + op + " in " + jo.str() );
            function = []( const T & ) {
                return false;
            };
        }
        // ~
    } else if( objects.size() == 4 && !no_result ) {
        op = objects.get_string( 3 );
        result = objects.get_string( 1 );
        if( result != "=" ) {
            jo.throw_error( "invalid result " + op + " in " + jo.str() );
            function = []( const T & ) {
                return false;
            };
        }
        std::function<int( const T & )> get_first_int = conditional_t< T >::get_get_int(
                    objects.get_object( 2 ) );
        if( op == "~" ) {
            function = [get_first_int, set_int]( const T & d ) {
                set_int( d, ~get_first_int( d ) );
            };
        } else {
            jo.throw_error( "unexpected operator " + op + " in " + jo.str() );
            function = []( const T & ) {
                return false;
            };
        }

        // =, -=, +=, *=, and /=
    } else if( objects.size() == 3 && !no_result ) {
        result = objects.get_string( 1 );
        std::function<int( const T & )> get_first_int = conditional_t< T >::get_get_int(
                    objects.get_object( 0 ) );
        std::function<int( const T & )> get_second_int = conditional_t< T >::get_get_int(
                    objects.get_object( 2 ) );
        if( result == "+=" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) + get_second_int( d ) );
            };
        } else if( result == "-=" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) - get_second_int( d ) );
            };
        } else if( result == "*=" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) * get_second_int( d ) );
            };
        } else if( result == "/=" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) / get_second_int( d ) );
            };
        } else if( result == "%=" ) {
            function = [get_first_int, get_second_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) % get_second_int( d ) );
            };
        } else if( result == "=" ) {
            function = [get_second_int, set_int]( const T & d ) {
                set_int( d, get_second_int( d ) );
            };
        } else {
            jo.throw_error( "unexpected result " + result + " in " + jo.str() );
            function = []( const T & ) {
                return false;
            };
        }
        // ++ and --
    } else if( objects.size() == 2 && !no_result ) {
        op = objects.get_string( 1 );
        std::function<int( const T & )> get_first_int = conditional_t< T >::get_get_int(
                    objects.get_object( 0 ) );
        if( op == "++" ) {
            function = [get_first_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) + 1 );
            };
        } else if( op == "--" ) {
            function = [get_first_int, set_int]( const T & d ) {
                set_int( d, get_first_int( d ) - 1 );
            };
        } else {
            jo.throw_error( "unexpected operator " + op + " in " + jo.str() );
            function = []( const T & ) {
                return false;
            };
        }
    } else {
        jo.throw_error( "Invalid number of args in " + jo.str() );
        function = []( const T & ) {
            return false;
        };
        return;
    }
}


template<class T>
void conditional_t<T>::set_u_has_camp()
{
    condition = []( const T & ) {
        return !get_player_character().camps.empty();
    };
}

template<class T>
void conditional_t<T>::set_has_pickup_list( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return d.actor( is_npc )->has_ai_rule( "pickup_rule", "any" );
    };
}

template<class T>
void conditional_t<T>::set_is_by_radio()
{
    condition = []( const T & d ) {
        return d.by_radio;
    };
}

template<class T>
void conditional_t<T>::set_has_reason()
{
    condition = []( const T & d ) {
        return !d.reason.empty();
    };
}

template<class T>
void conditional_t<T>::set_has_skill( const JsonObject &jo, const std::string &member,
                                      bool is_npc )
{
    JsonObject has_skill = jo.get_object( member );
    if( !has_skill.has_string( "skill" ) || !has_skill.has_int( "level" ) ) {
        condition = []( const T & ) {
            return false;
        };
    } else {
        const skill_id skill( has_skill.get_string( "skill" ) );
        int level = has_skill.get_int( "level" );
        condition = [skill, level, is_npc]( const T & d ) {
            return d.actor( is_npc )->get_skill_level( skill ) >= level;
        };
    }
}

template<class T>
void conditional_t<T>::set_u_know_recipe( const JsonObject &jo, const std::string &member )
{
    const std::string &known_recipe_id = jo.get_string( member );
    condition = [known_recipe_id]( const T & ) {
        const recipe &rep = recipe_id( known_recipe_id ).obj();
        // should be a talker function but recipes aren't in Character:: yet
        return get_player_character().knows_recipe( &rep );
    };
}

template<class T>
void conditional_t<T>::set_mission_has_generic_rewards()
{
    condition = []( const T & d ) {
        mission *miss = d.actor( true )->selected_mission();
        if( miss == nullptr ) {
            debugmsg( "mission_has_generic_rewards: mission_selected == nullptr" );
            return true;
        }
        return miss->has_generic_rewards();
    };
}

template<class T>
void conditional_t<T>::set_has_worn_with_flag( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const std::string flag( jo.get_string( member ) );
    cata::optional<bodypart_id> bp;
    optional( jo, false, "bodypart", bp );
    condition = [flag, bp, is_npc]( const T & d ) {
        bodypart_id bid = bp.value_or( get_bp_from_str( d.reason ) );
        return d.actor( is_npc )->worn_with_flag( flag_id( flag ), bid );
    };
}

template<class T>
void conditional_t<T>::set_has_wielded_with_flag( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const std::string flag( jo.get_string( member ) );
    condition = [flag, is_npc]( const T & d ) {
        return d.actor( is_npc )->wielded_with_flag( flag_id( flag ) );
    };
}

template<class T>
void conditional_t<T>::set_can_see( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return d.actor( is_npc )->can_see();
    };
}

template<class T>
void conditional_t<T>::set_is_deaf( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return d.actor( is_npc )->is_deaf();
    };
}

template<class T>
void conditional_t<T>::set_is_on_terrain( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    std::string terrain_type = jo.get_string( member );
    condition = [terrain_type, is_npc]( const T & d ) {
        map &here = get_map();
        return here.ter( d.actor( is_npc )->pos() ) == ter_id( terrain_type );
    };
}

template<class T>
void conditional_t<T>::set_is_in_field( const JsonObject &jo, const std::string &member,
                                        bool is_npc )
{
    std::string field_type = jo.get_string( member );
    condition = [field_type, is_npc]( const T & d ) {
        map &here = get_map();
        field_type_id ft = field_type_id( field_type );
        for( const std::pair<const field_type_id, field_entry> &f : here.field_at( d.actor(
                    is_npc )->pos() ) ) {
            if( f.second.get_field_type() == ft ) {
                return true;
            }
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_has_move_mode( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    move_mode_id mode( jo.get_string( member ) );
    condition = [mode, is_npc]( const T & d ) {
        return d.actor( is_npc )->get_move_mode() == mode;
    };
}

template<class T>
conditional_t<T>::conditional_t( const JsonObject &jo )
{
    // improve the clarity of NPC setter functions
    const bool is_npc = true;
    bool found_sub_member = false;
    const auto parse_array = []( const JsonObject & jo, const std::string & type ) {
        std::vector<conditional_t> conditionals;
        for( const JsonValue entry : jo.get_array( type ) ) {
            if( entry.test_string() ) {
                conditional_t<T> type_condition( entry.get_string() );
                conditionals.emplace_back( type_condition );
            } else {
                JsonObject cond = entry.get_object();
                conditional_t<T> type_condition( cond );
                conditionals.emplace_back( type_condition );
            }
        }
        return conditionals;
    };
    if( jo.has_array( "and" ) ) {
        std::vector<conditional_t> and_conditionals = parse_array( jo, "and" );
        found_sub_member = true;
        condition = [and_conditionals]( const T & d ) {
            for( const auto &cond : and_conditionals ) {
                if( !cond( d ) ) {
                    return false;
                }
            }
            return true;
        };
    } else if( jo.has_array( "or" ) ) {
        std::vector<conditional_t> or_conditionals = parse_array( jo, "or" );
        found_sub_member = true;
        condition = [or_conditionals]( const T & d ) {
            for( const auto &cond : or_conditionals ) {
                if( cond( d ) ) {
                    return true;
                }
            }
            return false;
        };
    } else if( jo.has_object( "not" ) ) {
        JsonObject cond = jo.get_object( "not" );
        const conditional_t<T> sub_condition = conditional_t<T>( cond );
        found_sub_member = true;
        condition = [sub_condition]( const T & d ) {
            return !sub_condition( d );
        };
    } else if( jo.has_string( "not" ) ) {
        const conditional_t<T> sub_condition = conditional_t<T>( jo.get_string( "not" ) );
        found_sub_member = true;
        condition = [sub_condition]( const T & d ) {
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
    if( jo.has_member( "u_has_any_trait" ) ) {
        set_has_any_trait( jo, "u_has_any_trait" );
    } else if( jo.has_member( "npc_has_any_trait" ) ) {
        set_has_any_trait( jo, "npc_has_any_trait", true );
    } else if( jo.has_member( "u_has_trait" ) ) {
        set_has_trait( jo, "u_has_trait" );
    } else if( jo.has_member( "npc_has_trait" ) ) {
        set_has_trait( jo, "npc_has_trait", true );
    } else if( jo.has_member( "u_has_flag" ) ) {
        set_has_flag( jo, "u_has_flag" );
    } else if( jo.has_member( "npc_has_flag" ) ) {
        set_has_flag( jo, "npc_has_flag", true );
    } else if( jo.has_member( "npc_has_class" ) ) {
        set_npc_has_class( jo, true );
    } else if( jo.has_member( "u_has_class" ) ) {
        set_npc_has_class( jo, false );
    } else if( jo.has_string( "npc_has_activity" ) ) {
        set_has_activity( is_npc );
    } else if( jo.has_string( "npc_is_riding" ) ) {
        set_is_riding( is_npc );
    } else if( jo.has_string( "u_has_mission" ) ) {
        set_u_has_mission( jo );
    } else if( jo.has_string( "u_monsters_in_direction" ) ) {
        set_u_monsters_in_direction( jo );
    } else if( jo.has_string( "u_safe_mode_trigger" ) ) {
        set_u_safe_mode_trigger( jo );
    } else if( jo.has_int( "u_has_strength" ) || jo.has_object( "u_has_strength" ) ) {
        set_has_strength( jo, "u_has_strength" );
    } else if( jo.has_int( "npc_has_strength" ) || jo.has_object( "npc_has_strength" ) ) {
        set_has_strength( jo, "npc_has_strength", is_npc );
    } else if( jo.has_int( "u_has_dexterity" ) || jo.has_object( "u_has_dexterity" ) ) {
        set_has_dexterity( jo, "u_has_dexterity" );
    } else if( jo.has_int( "npc_has_dexterity" ) || jo.has_object( "npc_has_dexterity" ) ) {
        set_has_dexterity( jo, "npc_has_dexterity", is_npc );
    } else if( jo.has_int( "u_has_intelligence" ) || jo.has_object( "u_has_intelligence" ) ) {
        set_has_intelligence( jo, "u_has_intelligence" );
    } else if( jo.has_int( "npc_has_intelligence" ) || jo.has_object( "npc_has_intelligence" ) ) {
        set_has_intelligence( jo, "npc_has_intelligence", is_npc );
    } else if( jo.has_int( "u_has_perception" ) || jo.has_object( "u_has_perception" ) ) {
        set_has_perception( jo, "u_has_perception" );
    } else if( jo.has_int( "npc_has_perception" ) || jo.has_object( "npc_has_perception" ) ) {
        set_has_perception( jo, "npc_has_perception", is_npc );
    } else if( jo.has_int( "u_has_hp" ) || jo.has_object( "u_has_hp" ) ) {
        set_has_hp( jo, "u_has_hp" );
    } else if( jo.has_int( "npc_has_hp" ) || jo.has_object( "npc_has_hp" ) ) {
        set_has_hp( jo, "npc_has_hp", is_npc );
    } else if( jo.has_string( "u_is_wearing" ) ) {
        set_is_wearing( jo, "u_is_wearing" );
    } else if( jo.has_string( "npc_is_wearing" ) ) {
        set_is_wearing( jo, "npc_is_wearing", is_npc );
    } else if( jo.has_string( "u_has_item" ) ) {
        set_has_item( jo, "u_has_item" );
    } else if( jo.has_string( "npc_has_item" ) ) {
        set_has_item( jo, "npc_has_item", is_npc );
    } else if( jo.has_string( "u_has_item_with_flag" ) ) {
        set_has_item_with_flag( jo, "u_has_item_with_flag" );
    } else if( jo.has_string( "npc_has_item_with_flag" ) ) {
        set_has_item_with_flag( jo, "npc_has_item_with_flag", is_npc );
    } else if( jo.has_member( "u_has_items" ) ) {
        set_has_items( jo, "u_has_items" );
    } else if( jo.has_member( "npc_has_items" ) ) {
        set_has_items( jo, "npc_has_items", is_npc );
    } else if( jo.has_string( "u_has_item_category" ) ) {
        set_has_item_category( jo, "u_has_item_category" );
    } else if( jo.has_string( "npc_has_item_category" ) ) {
        set_has_item_category( jo, "npc_has_item_category", is_npc );
    } else if( jo.has_string( "u_has_bionics" ) ) {
        set_has_bionics( jo, "u_has_bionics" );
    } else if( jo.has_string( "npc_has_bionics" ) ) {
        set_has_bionics( jo, "npc_has_bionics", is_npc );
    } else if( jo.has_string( "u_has_effect" ) ) {
        set_has_effect( jo, "u_has_effect" );
    } else if( jo.has_string( "npc_has_effect" ) ) {
        set_has_effect( jo, "npc_has_effect", is_npc );
    } else if( jo.has_string( "u_need" ) ) {
        set_need( jo, "u_need" );
    } else if( jo.has_string( "npc_need" ) ) {
        set_need( jo, "npc_need", is_npc );
    } else if( jo.has_member( "u_query" ) ) {
        set_query( jo, "u_query" );
    } else if( jo.has_member( "npc_query" ) ) {
        set_query( jo, "npc_query", is_npc );
    } else if( jo.has_string( "u_at_om_location" ) ) {
        set_at_om_location( jo, "u_at_om_location" );
    } else if( jo.has_string( "npc_at_om_location" ) ) {
        set_at_om_location( jo, "npc_at_om_location", is_npc );
    } else if( jo.has_string( "u_near_om_location" ) ) {
        set_near_om_location( jo, "u_near_om_location" );
    } else if( jo.has_string( "npc_near_om_location" ) ) {
        set_near_om_location( jo, "npc_near_om_location", is_npc );
    } else if( jo.has_string( "u_has_var" ) ) {
        set_has_var( jo, "u_has_var" );
    } else if( jo.has_string( "npc_has_var" ) ) {
        set_has_var( jo, "npc_has_var", is_npc );
    } else if( jo.has_string( "u_compare_var" ) ) {
        set_compare_var( jo, "u_compare_var" );
    } else if( jo.has_string( "npc_compare_var" ) ) {
        set_compare_var( jo, "npc_compare_var", is_npc );
    } else if( jo.has_string( "u_compare_time_since_var" ) ) {
        set_compare_time_since_var( jo, "u_compare_time_since_var" );
    } else if( jo.has_string( "npc_compare_time_since_var" ) ) {
        set_compare_time_since_var( jo, "npc_compare_time_since_var", is_npc );
    } else if( jo.has_string( "npc_role_nearby" ) ) {
        set_npc_role_nearby( jo );
    } else if( jo.has_int( "npc_allies" ) || jo.has_object( "npc_allies" ) ) {
        set_npc_allies( jo );
    } else if( jo.has_int( "npc_allies_global" ) || jo.has_object( "npc_allies_global" ) ) {
        set_npc_allies_global( jo );
    } else if( jo.get_bool( "npc_service", false ) ) {
        set_npc_available( true );
    } else if( jo.get_bool( "u_service", false ) ) {
        set_npc_available( false );
    } else if( jo.has_int( "u_has_cash" ) || jo.has_object( "u_has_cash" ) ) {
        set_u_has_cash( jo );
    } else if( jo.has_int( "u_are_owed" ) || jo.has_object( "u_are_owed" ) ) {
        set_u_are_owed( jo );
    } else if( jo.has_string( "npc_aim_rule" ) ) {
        set_npc_aim_rule( jo, true );
    } else if( jo.has_string( "u_aim_rule" ) ) {
        set_npc_aim_rule( jo, false );
    } else if( jo.has_string( "npc_engagement_rule" ) ) {
        set_npc_engagement_rule( jo, true );
    } else if( jo.has_string( "u_engagement_rule" ) ) {
        set_npc_engagement_rule( jo, false );
    } else if( jo.has_string( "npc_cbm_reserve_rule" ) ) {
        set_npc_cbm_reserve_rule( jo, true );
    } else if( jo.has_string( "u_cbm_reserve_rule" ) ) {
        set_npc_cbm_reserve_rule( jo, false );
    } else if( jo.has_string( "npc_cbm_recharge_rule" ) ) {
        set_npc_cbm_recharge_rule( jo, true );
    } else if( jo.has_string( "u_cbm_recharge_rule" ) ) {
        set_npc_cbm_recharge_rule( jo, false );
    } else if( jo.has_string( "npc_rule" ) ) {
        set_npc_rule( jo, true );
    } else if( jo.has_string( "u_rule" ) ) {
        set_npc_rule( jo, false );
    } else if( jo.has_string( "npc_override" ) ) {
        set_npc_override( jo, true );
    } else if( jo.has_string( "u_override" ) ) {
        set_npc_override( jo, false );
    } else if( jo.has_int( "days_since_cataclysm" ) || jo.has_object( "days_since_cataclysm" ) ) {
        set_days_since( jo );
    } else if( jo.has_string( "is_season" ) ) {
        set_is_season( jo );
    } else if( jo.has_string( "mission_goal" ) || jo.has_string( "npc_mission_goal" ) ) {
        set_mission_goal( jo, true );
    } else if( jo.has_string( "u_mission_goal" ) ) {
        set_mission_goal( jo, false );
    } else if( jo.has_member( "u_has_skill" ) ) {
        set_has_skill( jo, "u_has_skill" );
    } else if( jo.has_member( "npc_has_skill" ) ) {
        set_has_skill( jo, "npc_has_skill", is_npc );
    } else if( jo.has_member( "u_know_recipe" ) ) {
        set_u_know_recipe( jo, "u_know_recipe" );
    } else if( jo.has_int( "one_in_chance" ) || jo.has_object( "one_in_chance" ) ) {
        set_one_in_chance( jo, "one_in_chance" );
    } else if( jo.has_object( "x_in_y_chance" ) ) {
        set_x_in_y_chance( jo, "x_in_y_chance" );
    } else if( jo.has_string( "u_has_worn_with_flag" ) ) {
        set_has_worn_with_flag( jo, "u_has_worn_with_flag" );
    } else if( jo.has_string( "npc_has_worn_with_flag" ) ) {
        set_has_worn_with_flag( jo, "npc_has_worn_with_flag", is_npc );
    } else if( jo.has_string( "u_has_wielded_with_flag" ) ) {
        set_has_wielded_with_flag( jo, "u_has_wielded_with_flag" );
    } else if( jo.has_string( "npc_has_wielded_with_flag" ) ) {
        set_has_wielded_with_flag( jo, "npc_has_wielded_with_flag", is_npc );
    } else if( jo.has_string( "u_is_on_terrain" ) ) {
        set_is_on_terrain( jo, "u_is_on_terrain" );
    } else if( jo.has_string( "npc_is_on_terrain" ) ) {
        set_is_on_terrain( jo, "npc_is_on_terrain", is_npc );
    } else if( jo.has_string( "u_is_in_field" ) ) {
        set_is_in_field( jo, "u_is_in_field" );
    } else if( jo.has_string( "npc_is_in_field" ) ) {
        set_is_in_field( jo, "npc_is_in_field", is_npc );
    } else if( jo.has_string( "u_has_move_mode" ) ) {
        set_has_move_mode( jo, "u_has_move_mode" );
    } else if( jo.has_string( "npc_has_move_mode" ) ) {
        set_has_move_mode( jo, "npc_has_move_mode", is_npc );
    } else if( jo.has_string( "is_weather" ) ) {
        set_is_weather( jo );
    } else if( jo.has_int( "u_has_faction_trust" ) || jo.has_object( "u_has_faction_trust" ) ) {
        set_has_faction_trust( jo, "u_has_faction_trust" );
    } else if( jo.has_member( "compare_int" ) ) {
        set_compare_int( jo, "compare_int" );
    } else if( jo.has_member( "compare_string" ) ) {
        set_compare_string( jo, "compare_string" );
    } else {
        for( const std::string &sub_member : dialogue_data::simple_string_conds ) {
            if( jo.has_string( sub_member ) ) {
                const conditional_t<T> sub_condition( jo.get_string( sub_member ) );
                condition = [sub_condition]( const T & d ) {
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

template<class T>
conditional_t<T>::conditional_t( const std::string &type )
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
        condition = []( const T & ) {
            return false;
        };
    }
}

template struct conditional_t<dialogue>;
template void read_condition<dialogue>( const JsonObject &jo, const std::string &member_name,
                                        std::function<bool( const dialogue & )> &condition, bool default_val );

template duration_or_var<dialogue> get_duration_or_var( const JsonObject &jo, std::string member,
        bool required, time_duration default_val );
template std::string get_talk_varname<dialogue>( const JsonObject &jo, const std::string &member,
        bool check_value, int_or_var<dialogue> &default_val );
#if !defined(MACOSX)
template struct conditional_t<mission_goal_condition_context>;
#endif
template void read_condition<mission_goal_condition_context>( const JsonObject &jo,
        const std::string &member_name,
        std::function<bool( const mission_goal_condition_context & )> &condition, bool default_val );
