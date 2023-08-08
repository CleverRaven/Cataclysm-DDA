#pragma once
#ifndef CATA_SRC_CONDITION_H
#define CATA_SRC_CONDITION_H

#include <functional>
#include <iosfwd>
#include <string>
#include <unordered_set>

#include "dialogue.h"
#include "dialogue_helpers.h"
#include "math_parser_shim.h"
#include "mission.h"

class JsonObject;
namespace dialogue_data
{
// When updating this, please also update `dynamic_line_string_keys` in
// `lang/string_extractor/parsers/talk_topic.py` so the lines are properly
// extracted for translation
const std::unordered_set<std::string> simple_string_conds = { {
        "u_male", "u_female", "npc_male", "npc_female",
        "has_no_assigned_mission", "has_assigned_mission",
        "has_many_assigned_missions", "has_no_available_mission",
        "has_available_mission", "has_many_available_missions",
        "mission_complete", "mission_incomplete", "mission_has_generic_rewards",
        "npc_available", "npc_following", "npc_friend", "npc_hostile",
        "npc_train_skills", "npc_train_styles", "npc_train_spells",
        "at_safe_space", "is_day", "npc_has_activity",
        "is_outside", "u_is_outside", "npc_is_outside", "u_has_camp",
        "u_can_stow_weapon", "npc_can_stow_weapon", "u_can_drop_weapon",
        "npc_can_drop_weapon", "u_has_weapon", "npc_has_weapon",
        "u_driving", "npc_driving", "has_pickup_list", "is_by_radio", "has_reason"
    }
};
const std::unordered_set<std::string> complex_conds = { {
        "u_has_any_trait", "npc_has_any_trait", "u_has_trait", "npc_has_trait", "u_has_visible_trait", "npc_has_visible_trait",
        "u_has_flag", "npc_has_flag", "u_has_species", "npc_has_species", "u_bodytype", "npc_bodytype", "npc_has_class", "u_has_mission", "u_monsters_in_direction", "u_safe_mode_trigger",
        "u_has_strength", "npc_has_strength", "u_has_dexterity", "npc_has_dexterity",
        "u_has_intelligence", "npc_has_intelligence", "u_has_perception", "npc_has_perception",
        "u_is_wearing", "npc_is_wearing", "u_has_item", "npc_has_item", "u_has_move_mode", "npc_has_move_mode",
        "u_has_items", "npc_has_items", "u_has_item_category", "npc_has_item_category",
        "u_has_bionics", "npc_has_bionics", "u_has_effect", "npc_has_effect", "u_need", "npc_need",
        "u_at_om_location", "u_near_om_location", "npc_at_om_location", "npc_near_om_location",
        "npc_role_nearby", "npc_allies", "npc_allies_global", "npc_service",
        "u_has_cash", "u_are_owed", "u_query", "npc_query", "u_has_item_with_flag", "npc_has_item_with_flag",
        "npc_aim_rule", "npc_engagement_rule", "npc_rule", "npc_override", "u_has_hp", "npc_has_hp",
        "u_has_part_temp", "npc_has_part_temp", "npc_cbm_reserve_rule", "npc_cbm_recharge_rule", "u_has_faction_trust",
        "days_since_cataclysm", "is_season", "mission_goal", "u_has_var", "npc_has_var", "expects_vars",
        "u_has_skill", "npc_has_skill", "u_know_recipe", "u_compare_var", "npc_compare_var",
        "u_compare_time_since_var", "npc_compare_time_since_var", "is_weather", "mod_is_loaded", "one_in_chance", "x_in_y_chance",
        "u_is_height", "npc_is_height", "math",
        "u_has_worn_with_flag", "npc_has_worn_with_flag", "u_has_wielded_with_flag", "npc_has_wielded_with_flag",
        "u_has_pain", "npc_has_pain", "u_has_power", "npc_has_power", "u_has_focus", "npc_has_focus", "u_has_morale",
        "npc_has_morale", "u_is_on_terrain", "npc_is_on_terrain", "u_is_on_terrain_with_flag", "npc_is_on_terrain_with_flag", "u_is_in_field", "npc_is_in_field", "compare_int",
        "compare_string", "roll_contested", "compare_num", "u_has_martial_art", "npc_has_martial_art", "get_condition", "get_game_option"
    }
};
} // namespace dialogue_data

str_or_var get_str_or_var( const JsonValue &jv, const std::string &member, bool required = true,
                           const std::string &default_val = "" );
translation_or_var get_translation_or_var( const JsonValue &jv, const std::string &member,
        bool required = true, const translation &default_val = {} );
dbl_or_var get_dbl_or_var( const JsonObject &jo, const std::string &member, bool required = true,
                           double default_val = 0.0 );
dbl_or_var_part get_dbl_or_var_part( const JsonValue &jv, const std::string &member,
                                     bool required = true,
                                     double default_val = 0.0 );
duration_or_var get_duration_or_var( const JsonObject &jo, const std::string &member,
                                     bool required = true,
                                     time_duration default_val = 0_seconds );
duration_or_var_part get_duration_or_var_part( const JsonValue &jv, const std::string &member,
        bool required = true,
        time_duration default_val = 0_seconds );
tripoint_abs_ms get_tripoint_from_var( std::optional<var_info> var, dialogue const &d );
var_info read_var_info( const JsonObject &jo );
void write_var_value( var_type type, const std::string &name, talker *talk, dialogue *d,
                      const std::string &value );
void write_var_value( var_type type, const std::string &name, talker *talk, dialogue *d,
                      double value );
std::string get_talk_varname( const JsonObject &jo, const std::string &member,
                              bool check_value, dbl_or_var &default_val );
std::string get_talk_var_basename( const JsonObject &jo, const std::string &member,
                                   bool check_value );
// the truly awful declaration for the conditional_t loading helper_function
void read_condition( const JsonObject &jo, const std::string &member_name,
                     std::function<bool( dialogue & )> &condition, bool default_val );

void finalize_conditions();

/**
 * A condition for a response spoken by the player.
 * This struct only adds the constructors which will load the data from json
 * into a lambda, stored in the std::function object.
 * Invoking the function operator with a dialog reference (so the function can access the NPC)
 * returns whether the response is allowed.
 */
struct conditional_t {
    private:
        std::function<bool( dialogue & )> condition;

    public:
        conditional_t() = default;
        explicit conditional_t( const std::string &type );
        explicit conditional_t( const JsonObject &jo );

        void set_has_any_trait( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_trait( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_visible_trait( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_martial_art( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_flag( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_species( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_bodytype( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_var( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_expects_vars( const JsonObject &jo, const std::string &member );
        void set_compare_var( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_compare_time_since_var( const JsonObject &jo, const std::string &member,
                                         bool is_npc = false );
        void set_has_activity( bool is_npc = false );
        void set_is_riding( bool is_npc = false );
        void set_npc_has_class( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_u_has_mission( const JsonObject &jo, const std::string &member );
        void set_u_monsters_in_direction( const JsonObject &jo, const std::string &member );
        void set_u_safe_mode_trigger( const JsonObject &jo, const std::string &member );
        void set_has_strength( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_dexterity( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_intelligence( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_perception( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_hp( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_part_temp( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_is_deaf( bool is_npc = false );
        void set_is_on_terrain( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_is_on_terrain_with_flag( const JsonObject &jo, const std::string &member,
                                          bool is_npc = false );
        void set_is_in_field( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_one_in_chance( const JsonObject &jo, const std::string &member );
        void set_query( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_x_in_y_chance( const JsonObject &jo, std::string_view member );
        void set_has_worn_with_flag( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_wielded_with_flag( const JsonObject &jo, const std::string &member,
                                        bool is_npc = false );
        void set_is_wearing( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_item( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_items( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_item_with_flag( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_item_category( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_bionics( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_effect( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_need( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_at_om_location( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_near_om_location( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_has_move_mode( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_npc_role_nearby( const JsonObject &jo, const std::string &member );
        void set_npc_allies( const JsonObject &jo, const std::string &member );
        void set_npc_allies_global( const JsonObject &jo, const std::string &member );
        void set_u_has_cash( const JsonObject &jo, const std::string &member );
        void set_u_are_owed( const JsonObject &jo, const std::string &member );
        void set_npc_aim_rule( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_npc_engagement_rule( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_npc_cbm_reserve_rule( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_npc_cbm_recharge_rule( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_npc_rule( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_npc_override( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_days_since( const JsonObject &jo, const std::string &member );
        void set_is_season( const JsonObject &jo, const std::string &member );
        void set_is_weather( const JsonObject &jo, const std::string &member );
        void set_mod_is_loaded( const JsonObject &jo, const std::string &member );
        void set_mission_goal( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_has_faction_trust( const JsonObject &jo, const std::string &member );
        void set_no_assigned_mission();
        void set_has_assigned_mission();
        void set_has_many_assigned_missions();
        void set_no_available_mission( bool is_npc );
        void set_has_available_mission( bool is_npc );
        void set_has_many_available_missions( bool is_npc );
        void set_mission_complete( bool is_npc );
        void set_mission_incomplete( bool is_npc );
        void set_mission_failed( bool is_npc );
        void set_npc_available( bool is_npc );
        void set_npc_following( bool is_npc );
        void set_npc_friend( bool is_npc );
        void set_npc_hostile( bool is_npc );
        void set_npc_train_skills( bool is_npc );
        void set_npc_train_styles( bool is_npc );
        void set_npc_train_spells( bool is_npc );
        void set_at_safe_space( bool is_npc );
        void set_can_stow_weapon( bool is_npc = false );
        void set_can_drop_weapon( bool is_npc = false );
        void set_has_weapon( bool is_npc = false );
        void set_is_driving( bool is_npc = false );
        void set_is_day();
        void set_has_stolen_item( bool is_npc = false );
        void set_is_outside( bool is_npc = false );
        void set_is_underwater( bool is_npc = false );
        void set_is_by_radio();
        void set_u_has_camp();
        void set_has_pickup_list( bool is_npc );
        void set_has_reason();
        void set_is_alive( bool is_npc = false );
        void set_is_gender( bool is_male, bool is_npc = false );
        void set_has_skill( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_roll_contested( const JsonObject &jo, std::string_view member );
        void set_u_know_recipe( const JsonObject &jo, const std::string &member );
        void set_mission_has_generic_rewards();
        void set_can_see( bool is_npc = false );
        void set_get_option( const JsonObject &jo, const std::string &member );
        void set_compare_string( const JsonObject &jo, const std::string &member );
        void set_get_condition( const JsonObject &jo, const std::string &member );
        void set_compare_num( const JsonObject &jo, std::string_view member );
        void set_math( const JsonObject &jo, std::string_view member );
        static std::function<std::string( const dialogue & )> get_get_string( const JsonObject &jo );
        static std::function<translation( const dialogue & )> get_get_translation( const JsonObject &jo );
        template<class J>
        static std::function<double( dialogue & )> get_get_dbl( J const &jo );
        static std::function<double( dialogue & )> get_get_dbl( const std::string &value,
                const JsonObject &jo );
        template <class J>
        std::function<void( dialogue &, double )>
        static get_set_dbl( const J &jo, const std::optional<dbl_or_var_part> &min,
                            const std::optional<dbl_or_var_part> &max, bool temp_var );
        bool operator()( dialogue &d ) const {
            if( !condition ) {
                return false;
            }
            return condition( d );
        }
};

extern template std::function<double( dialogue & )>
conditional_t::get_get_dbl<>( kwargs_shim const & );

extern template std::function<void( dialogue &, double )>
conditional_t::get_set_dbl<>( const kwargs_shim &,
                              const std::optional<dbl_or_var_part> &,
                              const std::optional<dbl_or_var_part> &, bool );

#endif // CATA_SRC_CONDITION_H
