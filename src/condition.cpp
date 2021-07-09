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
#include "game.h"
#include "item.h"
#include "item_category.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "mission.h"
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

class basecamp;
class recipe;

static const efftype_id effect_currently_busy( "currently_busy" );

// throws an error on failure, so no need to return
std::string get_talk_varname( const JsonObject &jo, const std::string &member, bool check_value )
{
    if( !jo.has_string( "type" ) || !jo.has_string( "context" ) ||
        ( check_value && !( jo.has_string( "value" ) || jo.has_member( "time" ) ) ) ) {
        jo.throw_error( "invalid " + member + " condition in " + jo.str() );
    }
    const std::string &var_basename = jo.get_string( member );
    const std::string &type_var = jo.get_string( "type" );
    const std::string &var_context = jo.get_string( "context" );
    return "npctalk_var_" + type_var + "_" + var_context + "_" + var_basename;
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
        jo.throw_error( "invalid condition syntax", member_name );
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
void conditional_t<T>::set_has_trait_flag( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const json_character_flag &trait_flag_to_check = json_character_flag( jo.get_string( member ) );
    condition = [trait_flag_to_check, is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
        static const json_character_flag thresh( "MUTATION_THRESHOLD" );
        if( trait_flag_to_check == thresh ) {
            return actor->crossed_threshold();
        }
        return actor->has_trait_flag( trait_flag_to_check );
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
void conditional_t<T>::set_npc_has_class( const JsonObject &jo )
{
    const std::string &class_to_check = jo.get_string( "npc_has_class" );
    condition = [class_to_check]( const T & d ) {
        return d.beta->is_myclass( npc_class_id( class_to_check ) );
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
void conditional_t<T>::set_has_strength( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const int min_strength = jo.get_int( member );
    condition = [min_strength, is_npc]( const T & d ) {
        return d.actor( is_npc )->str_cur() >= min_strength;
    };
}

template<class T>
void conditional_t<T>::set_has_dexterity( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const int min_dexterity = jo.get_int( member );
    condition = [min_dexterity, is_npc]( const T & d ) {
        return d.actor( is_npc )->dex_cur() >= min_dexterity;
    };
}

template<class T>
void conditional_t<T>::set_has_intelligence( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const int min_intelligence = jo.get_int( member );
    condition = [min_intelligence, is_npc]( const T & d ) {
        return d.actor( is_npc )->int_cur() >= min_intelligence;
    };
}

template<class T>
void conditional_t<T>::set_has_perception( const JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const int min_perception = jo.get_int( member );
    condition = [min_perception, is_npc]( const T & d ) {
        return d.actor( is_npc )->per_cur() >= min_perception;
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
    if( !has_items.has_string( "item" ) || !has_items.has_int( "count" ) ) {
        condition = []( const T & ) {
            return false;
        };
    } else {
        const itype_id item_id( has_items.get_string( "item" ) );
        int count = has_items.get_int( "count" );
        condition = [item_id, count, is_npc]( const T & d ) {
            const talker *actor = d.actor( is_npc );
            return actor->has_charges( item_id, count ) || actor->has_amount( item_id, count );
        };
    }
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
        const auto items_with = actor->items_with( [category_id]( const item & it ) {
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
    condition = [effect_id, is_npc]( const T & d ) {
        return d.actor( is_npc )->has_effect( efftype_id( effect_id ) );
    };
}

template<class T>
void conditional_t<T>::set_need( const JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &need = jo.get_string( member );
    int amount = 0;
    if( jo.has_int( "amount" ) ) {
        amount = jo.get_int( "amount" );
    } else if( jo.has_string( "level" ) ) {
        const std::string &level = jo.get_string( "level" );
        auto flevel = fatigue_level_strs.find( level );
        if( flevel != fatigue_level_strs.end() ) {
            amount = static_cast<int>( flevel->second );
        }
    }
    condition = [need, amount, is_npc]( const T & d ) {
        const talker *actor = d.actor( is_npc );
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
void conditional_t<T>::set_has_var( const JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string var_name = get_talk_varname( jo, member, false );
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
    const std::string var_name = get_talk_varname( jo, member, false );
    const std::string &op = jo.get_string( "op" );
    const int value = jo.get_int( "value" );
    condition = [var_name, op, value, is_npc]( const T & d ) {
        int stored_value = 0;
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
    const std::string var_name = get_talk_varname( jo, member, false );
    const std::string &op = jo.get_string( "op" );
    const int value = to_turns<int>( read_from_json_string<time_duration>( *jo.get_raw( "time" ),
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
            return d.alpha->posz() == guy.posz() && guy.companion_mission_role_id == role &&
                   ( rl_dist( d.alpha->pos(), guy.pos() ) <= 48 );
        } );
        return !available.empty();
    };
}

template<class T>
void conditional_t<T>::set_npc_allies( const JsonObject &jo )
{
    const unsigned int min_allies = jo.get_int( "npc_allies" );
    condition = [min_allies]( const T & ) {
        return g->allies().size() >= min_allies;
    };
}

template<class T>
void conditional_t<T>::set_u_has_cash( const JsonObject &jo )
{
    const int min_cash = jo.get_int( "u_has_cash" );
    condition = [min_cash]( const T & d ) {
        return d.alpha->cash() >= min_cash;
    };
}

template<class T>
void conditional_t<T>::set_u_are_owed( const JsonObject &jo )
{
    const int min_debt = jo.get_int( "u_are_owed" );
    condition = [min_debt]( const T & d ) {
        return d.beta->debt() >= min_debt;
    };
}

template<class T>
void conditional_t<T>::set_npc_aim_rule( const JsonObject &jo )
{
    const std::string &setting = jo.get_string( "npc_aim_rule" );
    condition = [setting]( const T & d ) {
        return d.beta->has_ai_rule( "aim_rule", setting );
    };
}

template<class T>
void conditional_t<T>::set_npc_engagement_rule( const JsonObject &jo )
{
    const std::string &setting = jo.get_string( "npc_engagement_rule" );
    condition = [setting]( const T & d ) {
        return d.beta->has_ai_rule( "engagement_rule", setting );
    };
}

template<class T>
void conditional_t<T>::set_npc_cbm_reserve_rule( const JsonObject &jo )
{
    const std::string &setting = jo.get_string( "npc_cbm_reserve_rule" );
    condition = [setting]( const T & d ) {
        return d.beta->has_ai_rule( "cbm_reserve_rule", setting );
    };
}

template<class T>
void conditional_t<T>::set_npc_cbm_recharge_rule( const JsonObject &jo )
{
    const std::string &setting = jo.get_string( "npc_cbm_recharge_rule" );
    condition = [setting]( const T & d ) {
        return d.beta->has_ai_rule( "cbm_recharge_rule", setting );
    };
}

template<class T>
void conditional_t<T>::set_npc_rule( const JsonObject &jo )
{
    std::string rule = jo.get_string( "npc_rule" );
    condition = [rule]( const T & d ) {
        return d.beta->has_ai_rule( "ally_rule", rule );
    };
}

template<class T>
void conditional_t<T>::set_npc_override( const JsonObject &jo )
{
    std::string rule = jo.get_string( "npc_override" );
    condition = [rule]( const T & d ) {
        return d.beta->has_ai_rule( "ally_override", rule );
    };
}

template<class T>
void conditional_t<T>::set_days_since( const JsonObject &jo )
{
    const unsigned int days = jo.get_int( "days_since_cataclysm" );
    condition = [days]( const T & ) {
        return calendar::turn >= calendar::start_of_cataclysm + 1_days * days;
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
void conditional_t<T>::set_mission_goal( const JsonObject &jo )
{
    std::string mission_goal_str = jo.get_string( "mission_goal" );
    condition = [mission_goal_str]( const T & d ) {
        mission *miss = d.beta->selected_mission();
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
void conditional_t<T>::set_no_available_mission()
{
    condition = []( const T & d ) {
        return d.beta->available_missions().empty();
    };
}

template<class T>
void conditional_t<T>::set_has_available_mission()
{
    condition = []( const T & d ) {
        return d.beta->available_missions().size() == 1;
    };
}

template<class T>
void conditional_t<T>::set_has_many_available_missions()
{
    condition = []( const T & d ) {
        return d.beta->available_missions().size() >= 2;
    };
}

template<class T>
void conditional_t<T>::set_mission_complete()
{
    condition = []( const T & d ) {
        mission *miss = d.beta->selected_mission();
        return miss && miss->is_complete( d.beta->getID() );
    };
}

template<class T>
void conditional_t<T>::set_mission_incomplete()
{
    condition = []( const T & d ) {
        mission *miss = d.beta->selected_mission();
        return miss && !miss->is_complete( d.beta->getID() );
    };
}

template<class T>
void conditional_t<T>::set_npc_available()
{
    condition = []( const T & d ) {
        return !d.beta->has_effect( effect_currently_busy );
    };
}

template<class T>
void conditional_t<T>::set_npc_following()
{
    condition = []( const T & d ) {
        return d.beta->is_following();
    };
}

template<class T>
void conditional_t<T>::set_npc_friend()
{
    condition = []( const T & d ) {
        return d.beta->is_friendly( get_player_character() );
    };
}

template<class T>
void conditional_t<T>::set_npc_hostile()
{
    condition = []( const T & d ) {
        return d.beta->is_enemy();
    };
}

template<class T>
void conditional_t<T>::set_npc_train_skills()
{
    condition = []( const T & d ) {
        return !d.beta->skills_offered_to( *d.alpha ).empty();
    };
}

template<class T>
void conditional_t<T>::set_npc_train_styles()
{
    condition = []( const T & d ) {
        return !d.beta->styles_offered_to( *d.alpha ).empty();
    };
}

template<class T>
void conditional_t<T>::set_npc_train_spells()
{
    condition = []( const T & d ) {
        return !d.beta->spells_offered_to( *d.alpha ).empty();
    };
}

template<class T>
void conditional_t<T>::set_at_safe_space()
{
    condition = []( const T & d ) {
        return overmap_buffer.is_safe( d.beta->global_omt_location() ) && d.beta->is_safe();
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
        return d.alpha->has_stolen_item( *d.beta );
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
        return is_creature_outside( *d.actor( is_npc )->get_character() );
    };
}

template<class T>
void conditional_t<T>::set_one_in_chance( const JsonObject &jo, const std::string &member )
{
    const int one_in_chance = jo.get_int( member );
    condition = [one_in_chance]( const T & ) {
        return one_in( one_in_chance );
    };
}

template<class T>
void conditional_t<T>::set_is_temperature( const JsonObject &jo, const std::string &member )
{
    const int temp_min = jo.get_int( member );
    condition = [temp_min]( const T & ) {
        return get_weather().weather_precise->temperature >= temp_min;
    };
}

template<class T>
void conditional_t<T>::set_is_height( const JsonObject &jo, const std::string &member, bool is_npc )
{
    const int height_min = jo.get_int( member );
    condition = [height_min, is_npc]( const T & d ) {
        return d.actor( is_npc )->posz() >= height_min;
    };
}

template<class T>
void conditional_t<T>::set_is_windpower( const JsonObject &jo, const std::string &member )
{
    const int windpower_min = jo.get_int( member );
    condition = [windpower_min]( const T & ) {
        return get_weather().weather_precise->windpower >= windpower_min;
    };
}

template<class T>
void conditional_t<T>::set_is_humidity( const JsonObject &jo, const std::string &member )
{
    const int humidity_min = jo.get_int( member );
    condition = [humidity_min]( const T & ) {
        return get_weather().weather_precise->humidity >= humidity_min;
    };
}

template<class T>
void conditional_t<T>::set_is_pressure( const JsonObject &jo, const std::string &member )
{
    const int pressure_min = jo.get_int( member );
    condition = [pressure_min]( const T & ) {
        return get_weather().weather_precise->pressure >= pressure_min;
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
void conditional_t<T>::set_u_has_camp()
{
    condition = []( const T & ) {
        return !get_player_character().camps.empty();
    };
}

template<class T>
void conditional_t<T>::set_has_pickup_list()
{
    condition = []( const T & d ) {
        return d.beta->has_ai_rule( "pickup_rule", "any" );
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
void conditional_t<T>::set_has_skill( const JsonObject &jo, const std::string &member, bool is_npc )
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
        mission *miss = d.beta->selected_mission();
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
    condition = [flag, is_npc]( const T & d ) {
        return d.actor( is_npc )->worn_with_flag( flag_id( flag ) );
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
void conditional_t<T>::set_has_pain( const JsonObject &jo, const std::string &member,
                                     bool is_npc )
{
    const int min_pain = jo.get_int( member );
    condition = [min_pain, is_npc]( const T & d ) {
        return d.actor( is_npc )->pain_cur() >= min_pain;
    };
}

template<class T>
void conditional_t<T>::set_has_power( const JsonObject &jo, const std::string &member,
                                      bool is_npc )
{
    units::energy min_power;
    assign( jo, member, min_power, false, 0_kJ );
    condition = [min_power, is_npc]( const T & d ) {
        return d.actor( is_npc )->power_cur() >= min_power;
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
    } else if( jo.has_member( "u_has_trait_flag" ) ) {
        set_has_trait_flag( jo, "u_has_trait_flag" );
    } else if( jo.has_member( "npc_has_trait_flag" ) ) {
        set_has_trait_flag( jo, "npc_has_trait_flag", true );
    } else if( jo.has_member( "npc_has_class" ) ) {
        set_npc_has_class( jo );
    } else if( jo.has_string( "npc_has_activity" ) ) {
        set_has_activity( is_npc );
    } else if( jo.has_string( "npc_is_riding" ) ) {
        set_is_riding( is_npc );
    } else if( jo.has_string( "u_has_mission" ) ) {
        set_u_has_mission( jo );
    } else if( jo.has_int( "u_has_strength" ) ) {
        set_has_strength( jo, "u_has_strength" );
    } else if( jo.has_int( "npc_has_strength" ) ) {
        set_has_strength( jo, "npc_has_strength", is_npc );
    } else if( jo.has_int( "u_has_dexterity" ) ) {
        set_has_dexterity( jo, "u_has_dexterity" );
    } else if( jo.has_int( "npc_has_dexterity" ) ) {
        set_has_dexterity( jo, "npc_has_dexterity", is_npc );
    } else if( jo.has_int( "u_has_intelligence" ) ) {
        set_has_intelligence( jo, "u_has_intelligence" );
    } else if( jo.has_int( "npc_has_intelligence" ) ) {
        set_has_intelligence( jo, "npc_has_intelligence", is_npc );
    } else if( jo.has_int( "u_has_perception" ) ) {
        set_has_perception( jo, "u_has_perception" );
    } else if( jo.has_int( "npc_has_perception" ) ) {
        set_has_perception( jo, "npc_has_perception", is_npc );
    } else if( jo.has_string( "u_is_wearing" ) ) {
        set_is_wearing( jo, "u_is_wearing" );
    } else if( jo.has_string( "npc_is_wearing" ) ) {
        set_is_wearing( jo, "npc_is_wearing", is_npc );
    } else if( jo.has_string( "u_has_item" ) ) {
        set_has_item( jo, "u_has_item" );
    } else if( jo.has_string( "npc_has_item" ) ) {
        set_has_item( jo, "npc_has_item", is_npc );
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
    } else if( jo.has_string( "u_at_om_location" ) ) {
        set_at_om_location( jo, "u_at_om_location" );
    } else if( jo.has_string( "npc_at_om_location" ) ) {
        set_at_om_location( jo, "npc_at_om_location", is_npc );
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
    } else if( jo.has_int( "npc_allies" ) ) {
        set_npc_allies( jo );
    } else if( jo.get_bool( "npc_service", false ) ) {
        set_npc_available();
    } else if( jo.has_int( "u_has_cash" ) ) {
        set_u_has_cash( jo );
    } else if( jo.has_int( "u_are_owed" ) ) {
        set_u_are_owed( jo );
    } else if( jo.has_string( "npc_aim_rule" ) ) {
        set_npc_aim_rule( jo );
    } else if( jo.has_string( "npc_engagement_rule" ) ) {
        set_npc_engagement_rule( jo );
    } else if( jo.has_string( "npc_cbm_reserve_rule" ) ) {
        set_npc_cbm_reserve_rule( jo );
    } else if( jo.has_string( "npc_cbm_recharge_rule" ) ) {
        set_npc_cbm_recharge_rule( jo );
    } else if( jo.has_string( "npc_rule" ) ) {
        set_npc_rule( jo );
    } else if( jo.has_string( "npc_override" ) ) {
        set_npc_override( jo );
    } else if( jo.has_int( "days_since_cataclysm" ) ) {
        set_days_since( jo );
    } else if( jo.has_string( "is_season" ) ) {
        set_is_season( jo );
    } else if( jo.has_string( "mission_goal" ) ) {
        set_mission_goal( jo );
    } else if( jo.has_member( "u_has_skill" ) ) {
        set_has_skill( jo, "u_has_skill" );
    } else if( jo.has_member( "npc_has_skill" ) ) {
        set_has_skill( jo, "npc_has_skill", is_npc );
    } else if( jo.has_member( "u_know_recipe" ) ) {
        set_u_know_recipe( jo, "u_know_recipe" );
    } else if( jo.has_int( "one_in_chance" ) ) {
        set_one_in_chance( jo, "one_in_chance" );
    } else if( jo.has_int( "is_temperature" ) ) {
        set_is_temperature( jo, "is_temperature" );
    } else if( jo.has_int( "is_windpower" ) ) {
        set_is_windpower( jo, "is_windpower" );
    } else if( jo.has_int( "is_humidity" ) ) {
        set_is_humidity( jo, "is_humidity" );
    } else if( jo.has_member( "is_pressure" ) ) {
        set_is_pressure( jo, "is_pressure" );
    } else if( jo.has_int( "u_is_height" ) ) {
        set_is_height( jo, "u_is_height" );
    } else if( jo.has_int( "npc_is_height" ) ) {
        set_is_height( jo, "npc_is_height", is_npc );
    } else if( jo.has_string( "u_has_worn_with_flag" ) ) {
        set_has_worn_with_flag( jo, "u_has_worn_with_flag" );
    } else if( jo.has_string( "npc_has_worn_with_flag" ) ) {
        set_has_worn_with_flag( jo, "npc_has_worn_with_flag", is_npc );
    } else if( jo.has_string( "u_has_wielded_with_flag" ) ) {
        set_has_wielded_with_flag( jo, "u_has_wielded_with_flag" );
    } else if( jo.has_string( "npc_has_wielded_with_flag" ) ) {
        set_has_wielded_with_flag( jo, "npc_has_wielded_with_flag", is_npc );
    } else if( jo.has_member( "u_has_pain" ) ) {
        set_has_pain( jo, "u_has_pain" );
    } else if( jo.has_member( "npc_has_pain" ) ) {
        set_has_pain( jo, "npc_has_pain", is_npc );
    } else if( jo.has_member( "u_has_power" ) ) {
        set_has_power( jo, "u_has_power" );
    } else if( jo.has_member( "npc_has_power" ) ) {
        set_has_power( jo, "npc_has_power", is_npc );
    } else if( jo.has_string( "is_weather" ) ) {
        set_is_weather( jo );
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
    } else if( type == "has_no_available_mission" ) {
        set_no_available_mission();
    } else if( type == "has_available_mission" ) {
        set_has_available_mission();
    } else if( type == "has_many_available_missions" ) {
        set_has_many_available_missions();
    } else if( type == "mission_complete" ) {
        set_mission_complete();
    } else if( type == "mission_incomplete" ) {
        set_mission_incomplete();
    } else if( type == "npc_available" ) {
        set_npc_available();
    } else if( type == "npc_following" ) {
        set_npc_following();
    } else if( type == "npc_friend" ) {
        set_npc_friend();
    } else if( type == "npc_hostile" ) {
        set_npc_hostile();
    } else if( type == "npc_train_skills" ) {
        set_npc_train_skills();
    } else if( type == "npc_train_styles" ) {
        set_npc_train_styles();
    } else if( type == "npc_train_spells" ) {
        set_npc_train_spells();
    } else if( type == "at_safe_space" ) {
        set_at_safe_space();
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
    } else if( type == "u_has_camp" ) {
        set_u_has_camp();
    } else if( type == "has_pickup_list" ) {
        set_has_pickup_list();
    } else if( type == "is_by_radio" ) {
        set_is_by_radio();
    } else if( type == "has_reason" ) {
        set_has_reason();
    } else if( type == "mission_has_generic_rewards" ) {
        set_mission_has_generic_rewards();
    } else if( type == "u_can_see" || type == "npc_can_see" ) {
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
#if !defined(MACOSX)
template struct conditional_t<mission_goal_condition_context>;
#endif
template void read_condition<mission_goal_condition_context>( const JsonObject &jo,
        const std::string &member_name,
        std::function<bool( const mission_goal_condition_context & )> &condition, bool default_val );
