#pragma once
#ifndef CONDITION_H
#define CONDITION_H

#include <functional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "json.h"

class player;
class npc;
class mission;

namespace dialogue_data
{
const std::unordered_set<std::string> simple_string_conds = { {
        "u_male", "u_female", "npc_male", "npc_female",
        "has_no_assigned_mission", "has_assigned_mission", "has_many_assigned_missions",
        "has_no_available_mission", "has_available_mission", "has_many_available_missions",
        "mission_complete", "mission_incomplete",
        "npc_available", "npc_following", "npc_friend", "npc_hostile",
        "npc_train_skills", "npc_train_styles",
        "at_safe_space", "is_day", "npc_has_activity", "is_outside", "u_has_camp",
        "u_can_stow_weapon", "npc_can_stow_weapon", "u_has_weapon", "npc_has_weapon",
        "u_driving", "npc_driving",
        "has_pickup_list", "is_by_radio", "has_reason"
    }
};
const std::unordered_set<std::string> complex_conds = { {
        "u_has_any_trait", "npc_has_any_trait", "u_has_trait", "npc_has_trait",
        "u_has_trait_flag", "npc_has_trait_flag", "npc_has_class", "u_has_mission",
        "u_has_strength", "npc_has_strength", "u_has_dexterity", "npc_has_dexterity",
        "u_has_intelligence", "npc_has_intelligence", "u_has_perception", "npc_has_perception",
        "u_is_wearing", "npc_is_wearing", "u_has_item", "npc_has_item",
        "u_has_items", "npc_has_items", "u_has_item_category", "npc_has_item_category",
        "u_has_bionics", "npc_has_bionics", "u_has_effect", "npc_has_effect", "u_need", "npc_need",
        "u_at_om_location", "npc_at_om_location", "npc_role_nearby", "npc_allies", "npc_service",
        "u_has_cash", "u_are_owed",
        "npc_aim_rule", "npc_engagement_rule", "npc_rule", "npc_override",
        "npc_cbm_reserve_rule", "npc_cbm_recharge_rule",
        "days_since_cataclysm", "is_season", "mission_goal", "u_has_var", "npc_has_var"
    }
};
} // namespace dialogue_data

std::string get_talk_varname( JsonObject jo, const std::string &member, bool check_value = true );

// the truly awful declaration for the conditional_t loading helper_function
template<class T>
void read_condition( JsonObject &jo, const std::string &member_name,
                     std::function<bool( const T & )> &condition, bool default_val );

/**
 * A condition for a response spoken by the player.
 * This struct only adds the constructors which will load the data from json
 * into a lambda, stored in the std::function object.
 * Invoking the function operator with a dialog reference (so the function can access the NPC)
 * returns whether the response is allowed.
 */
template<class T>
struct conditional_t {
    private:
        std::function<bool( const T & )> condition;

    public:
        conditional_t() = default;
        conditional_t( const std::string &type );
        conditional_t( JsonObject jo );

        void set_has_any_trait( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_trait( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_trait_flag( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_var( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_activity( bool is_npc = false );
        void set_npc_has_class( JsonObject &jo );
        void set_u_has_mission( JsonObject &jo );
        void set_has_strength( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_dexterity( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_intelligence( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_perception( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_is_wearing( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_item( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_items( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_item_category( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_bionics( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_effect( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_need( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_at_om_location( JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_npc_role_nearby( JsonObject &jo );
        void set_npc_allies( JsonObject &jo );
        void set_u_has_cash( JsonObject &jo );
        void set_u_are_owed( JsonObject &jo );
        void set_npc_aim_rule( JsonObject &jo );
        void set_npc_engagement_rule( JsonObject &jo );
        void set_npc_cbm_reserve_rule( JsonObject &jo );
        void set_npc_cbm_recharge_rule( JsonObject &jo );
        void set_npc_rule( JsonObject &jo );
        void set_npc_override( JsonObject &jo );
        void set_days_since( JsonObject &jo );
        void set_is_season( JsonObject &jo );
        void set_mission_goal( JsonObject &jo );
        void set_no_assigned_mission();
        void set_has_assigned_mission();
        void set_has_many_assigned_missions();
        void set_no_available_mission();
        void set_has_available_mission();
        void set_has_many_available_missions();
        void set_mission_complete();
        void set_mission_incomplete();
        void set_npc_available();
        void set_npc_following();
        void set_npc_friend();
        void set_npc_hostile();
        void set_npc_train_skills();
        void set_npc_train_styles();
        void set_at_safe_space();
        void set_can_stow_weapon( bool is_npc = false );
        void set_has_weapon( bool is_npc = false );
        void set_is_driving( bool is_npc = false );
        void set_is_day();
        void set_has_stolen_item( bool is_npc = false );
        void set_is_outside();
        void set_is_by_radio();
        void set_u_has_camp();
        void set_has_pickup_list();
        void set_has_reason();
        void set_is_gender( bool is_male, bool is_npc = false );

        bool operator()( const T &d ) const {
            if( !condition ) {
                return false;
            }
            return condition( d );
        }
};

#endif
