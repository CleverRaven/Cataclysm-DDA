#include "condition.h"

#include <functional>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "dialogue.h"
#include "faction_camp.h"
#include "game.h"
#include "item_category.h"
#include "item.h"
#include "auto_pickup.h"
#include "json.h"
#include "map.h"
#include "mission.h"
#include "npc.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "recipe.h"
#include "recipe_groups.h"
#include "string_id.h"
#include "type_id.h"
#include "vehicle.h"
#include "vpart_position.h"

const efftype_id effect_currently_busy( "currently_busy" );

// throws an error on failure, so no need to return
std::string get_talk_varname( JsonObject &jo, const std::string &member, bool check_value )
{
    if( !jo.has_string( "type" ) || !jo.has_string( "context" ) ||
        ( check_value && !jo.has_string( "value" ) ) ) {
        jo.throw_error( "invalid " + member + " condition in " + jo.str() );
    }
    const std::string &var_basename = jo.get_string( member );
    const std::string &type_var = jo.get_string( "type" );
    const std::string &var_context = jo.get_string( "context" );
    return "npctalk_var_" + type_var + "_" + var_context + "_" + var_basename;
}

template<class T>
void read_condition( JsonObject &jo, const std::string &member_name,
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
void conditional_t<T>::set_has_any_trait( JsonObject &jo, const std::string &member, bool is_npc )
{
    std::vector<trait_id> traits_to_check;
    for( auto&& f : jo.get_string_array( member ) ) { // *NOPAD*
        traits_to_check.emplace_back( f );
    }
    condition = [traits_to_check, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        for( const auto &trait : traits_to_check ) {
            if( actor->has_trait( trait ) ) {
                return true;
            }
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_has_trait( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &trait_to_check = jo.get_string( member );
    condition = [trait_to_check, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->has_trait( trait_id( trait_to_check ) );
    };
}

template<class T>
void conditional_t<T>::set_has_trait_flag( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &trait_flag_to_check = jo.get_string( member );
    condition = [trait_flag_to_check, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        if( trait_flag_to_check == "MUTATION_THRESHOLD" ) {
            return actor->crossed_threshold();
        }
        return actor->has_trait_flag( trait_flag_to_check );
    };
}

template<class T>
void conditional_t<T>::set_has_activity( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            return d.beta->has_activity();
        } else {
            if( !actor->activity.is_null() ) {
                return true;
            }
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_is_riding( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        return ( is_npc ? d.alpha : d.beta )->is_mounted();
    };
}

template<class T>
void conditional_t<T>::set_npc_has_class( JsonObject &jo )
{
    const std::string &class_to_check = jo.get_string( "npc_has_class" );
    condition = [class_to_check]( const T & d ) {
        return d.beta->myclass == npc_class_id( class_to_check );
    };
}

template<class T>
void conditional_t<T>::set_u_has_mission( JsonObject &jo )
{
    const std::string &mission = jo.get_string( "u_has_mission" );
    condition = [mission]( const T & ) {
        for( auto miss_it : g->u.get_active_missions() ) {
            if( miss_it->mission_id() == mission_type_id( mission ) ) {
                return true;
            }
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_has_strength( JsonObject &jo, const std::string &member, bool is_npc )
{
    const int min_strength = jo.get_int( member );
    condition = [min_strength, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->str_cur >= min_strength;
    };
}

template<class T>
void conditional_t<T>::set_has_dexterity( JsonObject &jo, const std::string &member, bool is_npc )
{
    const int min_dexterity = jo.get_int( member );
    condition = [min_dexterity, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->dex_cur >= min_dexterity;
    };
}

template<class T>
void conditional_t<T>::set_has_intelligence( JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const int min_intelligence = jo.get_int( member );
    condition = [min_intelligence, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->int_cur >= min_intelligence;
    };
}

template<class T>
void conditional_t<T>::set_has_perception( JsonObject &jo, const std::string &member, bool is_npc )
{
    const int min_perception = jo.get_int( member );
    condition = [min_perception, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->per_cur >= min_perception;
    };
}

template<class T>
void conditional_t<T>::set_is_wearing( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &item_id = jo.get_string( member );
    condition = [item_id, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->is_wearing( item_id );
    };
}

template<class T>
void conditional_t<T>::set_has_item( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &item_id = jo.get_string( member );
    condition = [item_id, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->charges_of( item_id ) > 0 || actor->has_amount( item_id, 1 );
    };
}

template<class T>
void conditional_t<T>::set_has_items( JsonObject &jo, const std::string &member, bool is_npc )
{
    JsonObject has_items = jo.get_object( member );
    if( !has_items.has_string( "item" ) || !has_items.has_int( "count" ) ) {
        condition = []( const T & ) {
            return false;
        };
    } else {
        const std::string item_id = has_items.get_string( "item" );
        int count = has_items.get_int( "count" );
        condition = [item_id, count, is_npc]( const T & d ) {
            player *actor = d.alpha;
            if( is_npc ) {
                actor = dynamic_cast<player *>( d.beta );
            }
            return actor->has_charges( item_id, count ) || actor->has_amount( item_id, count );
        };
    }
}

template<class T>
void conditional_t<T>::set_has_item_category( JsonObject &jo, const std::string &member,
        bool is_npc )
{
    const std::string category_id = jo.get_string( member );

    size_t count = 1;
    if( jo.has_int( "count" ) ) {
        int tcount = jo.get_int( "count" );
        if( tcount > 1 && tcount < INT_MAX ) {
            count = static_cast<size_t>( tcount );
        }
    }

    condition = [category_id, count, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        const auto items_with = actor->items_with( [category_id]( const item & it ) {
            return it.get_category().id() == category_id;
        } );
        return items_with.size() >= count;
    };
}

template<class T>
void conditional_t<T>::set_has_bionics( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string bionics_id = jo.get_string( member );
    condition = [bionics_id, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        if( bionics_id == "ANY" ) {
            return actor->num_bionics() > 0 || actor->has_max_power();
        }
        return actor->has_bionic( bionic_id( bionics_id ) );
    };
}

template<class T>
void conditional_t<T>::set_has_effect( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &effect_id = jo.get_string( member );
    condition = [effect_id, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->has_effect( efftype_id( effect_id ) );
    };
}

template<class T>
void conditional_t<T>::set_need( JsonObject &jo, const std::string &member, bool is_npc )
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
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return ( actor->get_fatigue() > amount && need == "fatigue" ) ||
               ( actor->get_hunger() > amount && need == "hunger" ) ||
               ( actor->get_thirst() > amount && need == "thirst" );
    };
}

template<class T>
void conditional_t<T>::set_at_om_location( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string &location = jo.get_string( member );
    condition = [location, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        const tripoint omt_pos = actor->global_omt_location();
        const oter_id &omt_ref = overmap_buffer.ter( omt_pos );

        if( location == "FACTION_CAMP_ANY" ) {
            cata::optional<basecamp *> bcp = overmap_buffer.find_camp( omt_pos.xy() );
            if( bcp ) {
                return true;
            }
            // legacy check
            const std::string &omt_str = omt_ref.id().c_str();
            return omt_str.find( "faction_base_camp" ) != std::string::npos;
        } else if( location == "FACTION_CAMP_START" ) {
            return !recipe_group::get_recipes_by_id( "all_faction_base_types",
                    omt_ref.id().c_str() ).empty();
        } else {
            return omt_ref == oter_id( oter_no_dir( oter_id( location ) ) );
        }
    };
}

template<class T>
void conditional_t<T>::set_has_var( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string var_name = get_talk_varname( jo, member, false );
    const std::string &value = jo.get_string( "value" );
    condition = [var_name, value, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->get_value( var_name ) == value;
    };
}

template<class T>
void conditional_t<T>::set_compare_var( JsonObject &jo, const std::string &member, bool is_npc )
{
    const std::string var_name = get_talk_varname( jo, member, false );
    const std::string &op = jo.get_string( "op" );
    const int value = jo.get_int( "value" );
    condition = [var_name, op, value, is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }

        int stored_value = 0;
        const std::string &var = actor->get_value( var_name );
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
void conditional_t<T>::set_npc_role_nearby( JsonObject &jo )
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
void conditional_t<T>::set_npc_allies( JsonObject &jo )
{
    const unsigned int min_allies = jo.get_int( "npc_allies" );
    condition = [min_allies]( const T & ) {
        return g->allies().size() >= min_allies;
    };
}

template<class T>
void conditional_t<T>::set_u_has_cash( JsonObject &jo )
{
    const int min_cash = jo.get_int( "u_has_cash" );
    condition = [min_cash]( const T & d ) {
        return d.alpha->cash >= min_cash;
    };
}

template<class T>
void conditional_t<T>::set_u_are_owed( JsonObject &jo )
{
    const int min_debt = jo.get_int( "u_are_owed" );
    condition = [min_debt]( const T & d ) {
        return d.beta->op_of_u.owed >= min_debt;
    };
}

template<class T>
void conditional_t<T>::set_npc_aim_rule( JsonObject &jo )
{
    const std::string &setting = jo.get_string( "npc_aim_rule" );
    condition = [setting]( const T & d ) {
        auto rule = aim_rule_strs.find( setting );
        if( rule != aim_rule_strs.end() ) {
            return d.beta->rules.aim == rule->second;
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_npc_engagement_rule( JsonObject &jo )
{
    const std::string &setting = jo.get_string( "npc_engagement_rule" );
    condition = [setting]( const T & d ) {
        auto rule = combat_engagement_strs.find( setting );
        if( rule != combat_engagement_strs.end() ) {
            return d.beta->rules.engagement == rule->second;
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_npc_cbm_reserve_rule( JsonObject &jo )
{
    const std::string &setting = jo.get_string( "npc_cbm_reserve_rule" );
    condition = [setting]( const T & d ) {
        auto rule = cbm_reserve_strs.find( setting );
        if( rule != cbm_reserve_strs.end() ) {
            return d.beta->rules.cbm_reserve == rule->second;
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_npc_cbm_recharge_rule( JsonObject &jo )
{
    const std::string &setting = jo.get_string( "npc_cbm_recharge_rule" );
    condition = [setting]( const T & d ) {
        auto rule = cbm_recharge_strs.find( setting );
        if( rule != cbm_recharge_strs.end() ) {
            return d.beta->rules.cbm_recharge == rule->second;
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_npc_rule( JsonObject &jo )
{
    std::string rule = jo.get_string( "npc_rule" );
    condition = [rule]( const T & d ) {
        auto flag = ally_rule_strs.find( rule );
        if( flag != ally_rule_strs.end() ) {
            return d.beta->rules.has_flag( flag->second.rule );
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_npc_override( JsonObject &jo )
{
    std::string rule = jo.get_string( "npc_override" );
    condition = [rule]( const T & d ) {
        auto flag = ally_rule_strs.find( rule );
        if( flag != ally_rule_strs.end() ) {
            return d.beta->rules.has_override_enable( flag->second.rule );
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_days_since( JsonObject &jo )
{
    const unsigned int days = jo.get_int( "days_since_cataclysm" );
    condition = [days]( const T & ) {
        return to_turn<int>( calendar::turn ) >= calendar::start_of_cataclysm + 1_days * days;
    };
}

template<class T>
void conditional_t<T>::set_is_season( JsonObject &jo )
{
    std::string season_name = jo.get_string( "is_season" );
    condition = [season_name]( const T & ) {
        const auto season = season_of_year( calendar::turn );
        return ( season == SPRING && season_name == "spring" ) ||
               ( season == SUMMER && season_name == "summer" ) ||
               ( season == AUTUMN && season_name == "autumn" ) ||
               ( season == WINTER && season_name == "winter" );
    };
}

template<class T>
void conditional_t<T>::set_mission_goal( JsonObject &jo )
{
    std::string mission_goal_str = jo.get_string( "mission_goal" );
    condition = [mission_goal_str]( const T & d ) {
        mission *miss = d.beta->chatbin.mission_selected;
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
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return actor->male == is_male;
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
        return d.beta->chatbin.missions.empty();
    };
}

template<class T>
void conditional_t<T>::set_has_available_mission()
{
    condition = []( const T & d ) {
        return d.beta->chatbin.missions.size() == 1;
    };
}

template<class T>
void conditional_t<T>::set_has_many_available_missions()
{
    condition = []( const T & d ) {
        return d.beta->chatbin.missions.size() >= 2;
    };
}

template<class T>
void conditional_t<T>::set_mission_complete()
{
    condition = []( const T & d ) {
        mission *miss = d.beta->chatbin.mission_selected;
        if( !miss ) {
            return false;
        }
        return miss->is_complete( d.beta->getID() );
    };
}

template<class T>
void conditional_t<T>::set_mission_incomplete()
{
    condition = []( const T & d ) {
        mission *miss = d.beta->chatbin.mission_selected;
        if( !miss ) {
            return false;
        }
        return !miss->is_complete( d.beta->getID() );
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
        return d.beta->is_friendly( g->u );
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
void conditional_t<T>::set_at_safe_space()
{
    condition = []( const T & d ) {
        return overmap_buffer.is_safe( d.beta->global_omt_location() );
    };
}

template<class T>
void conditional_t<T>::set_can_stow_weapon( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return !actor->unarmed_attack() && actor->can_pickVolume( actor->weapon );
    };
}

template<class T>
void conditional_t<T>::set_has_weapon( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        return !actor->unarmed_attack();
    };
}

template<class T>
void conditional_t<T>::set_is_driving( bool is_npc )
{
    condition = [is_npc]( const T & d ) {
        player *actor = d.alpha;
        if( is_npc ) {
            actor = dynamic_cast<player *>( d.beta );
        }
        if( const optional_vpart_position vp = g->m.veh_at( actor->pos() ) ) {
            return vp->vehicle().is_moving() && vp->vehicle().player_in_control( *actor );
        }
        return false;
    };
}

template<class T>
void conditional_t<T>::set_has_stolen_item( bool is_npc )
{
    ( void )is_npc;
    condition = []( const T & d ) {
        player *actor = d.alpha;
        npc &p = *d.beta;
        bool found_in_inv = false;
        for( auto &elem : actor->inv_dump() ) {
            if( elem->is_old_owner( p, true ) ) {
                found_in_inv = true;
            }
        }
        return found_in_inv;
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
void conditional_t<T>::set_is_outside()
{
    condition = []( const T & d ) {
        const tripoint pos = g->m.getabs( d.beta->pos() );
        return !g->m.has_flag( TFLAG_INDOORS, pos );
    };
}

template<class T>
void conditional_t<T>::set_u_has_camp()
{
    condition = []( const T & ) {
        return !g->u.camps.empty();
    };
}

template<class T>
void conditional_t<T>::set_has_pickup_list()
{
    condition = []( const T & d ) {
        return !d.beta->rules.pickup_whitelist->empty();
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
void conditional_t<T>::set_has_skill( JsonObject &jo, const std::string &member, bool is_npc )
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
            player *actor = d.alpha;
            if( is_npc ) {
                actor = dynamic_cast<player *>( d.beta );
            }
            return actor->get_skill_level( skill ) >= level;
        };
    }
}

template<class T>
void conditional_t<T>::set_u_know_recipe( JsonObject &jo, const std::string &member )
{
    const std::string &known_recipe_id = jo.get_string( member );
    condition = [known_recipe_id]( const T & d ) {
        player *actor = d.alpha;
        const recipe &r = recipe_id( known_recipe_id ).obj();
        return actor->knows_recipe( &r );
    };
}

template<class T>
void conditional_t<T>::set_mission_has_generic_rewards()
{
    condition = []( const T & d ) {
        mission *miss = d.beta->chatbin.mission_selected;
        if( miss == nullptr ) {
            debugmsg( "mission_has_generic_rewards: mission_selected == nullptr" );
            return true;
        }
        return miss->has_generic_rewards();
    };
}

template<class T>
conditional_t<T>::conditional_t( JsonObject &jo )
{
    // improve the clarity of NPC setter functions
    const bool is_npc = true;
    bool found_sub_member = false;
    const auto parse_array = []( JsonObject jo, const std::string & type ) {
        std::vector<conditional_t> conditionals;
        JsonArray ja = jo.get_array( type );
        while( ja.has_more() ) {
            if( ja.test_string() ) {
                conditional_t<T> type_condition( ja.next_string() );
                conditionals.emplace_back( type_condition );
            } else if( ja.test_object() ) {
                JsonObject cond = ja.next_object();
                conditional_t<T> type_condition( cond );
                conditionals.emplace_back( type_condition );
            } else {
                ja.skip_value();
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
    } else if( jo.has_string( "npc_role_nearby" ) ) {
        set_npc_role_nearby( jo );
    } else if( jo.has_int( "npc_allies" ) ) {
        set_npc_allies( jo );
    } else if( jo.has_int( "npc_service" ) ) {
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
    } else if( type == "is_outside" ) {
        set_is_outside();
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
    } else {
        condition = []( const T & ) {
            return false;
        };
    }
}

template struct conditional_t<dialogue>;
template void read_condition<dialogue>( JsonObject &jo, const std::string &member_name,
                                        std::function<bool( const dialogue & )> &condition, bool default_val );
template void read_condition<mission_goal_condition_context>( JsonObject &jo,
        const std::string &member_name,
        std::function<bool( const mission_goal_condition_context & )> &condition, bool default_val );
