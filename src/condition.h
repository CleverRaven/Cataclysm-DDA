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
const std::unordered_set<std::string> &simple_string_conds();
const std::unordered_set<std::string> &complex_conds();
} // namespace dialogue_data

enum class jarg {
    member = 1,
    object = 1 << 1,
    string = 1 << 2,
    array = 1 << 3
};

template<>
struct enum_traits<jarg> {
    static constexpr bool is_flag_enum = true;
};

str_or_var get_str_or_var( const JsonValue &jv, std::string_view member, bool required = true,
                           std::string_view default_val = "" );
translation_or_var get_translation_or_var( const JsonValue &jv, std::string_view member,
        bool required = true, const translation &default_val = {} );
dbl_or_var get_dbl_or_var( const JsonObject &jo, std::string_view member, bool required = true,
                           double default_val = 0.0 );
dbl_or_var_part get_dbl_or_var_part( const JsonValue &jv, std::string_view member,
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
std::string get_talk_varname( const JsonObject &jo, std::string_view member,
                              bool check_value, dbl_or_var &default_val );
std::string get_talk_var_basename( const JsonObject &jo, std::string_view member,
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
        explicit conditional_t( std::string_view type );
        explicit conditional_t( const JsonObject &jo );

        void set_has_any_trait( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_trait( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_visible_trait( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_martial_art( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_flag( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_species( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_bodytype( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_var( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_expects_vars( const JsonObject &jo, std::string_view member );
        void set_compare_var( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_compare_time_since_var( const JsonObject &jo, std::string_view member,
                                         bool is_npc = false );
        void set_has_activity( bool is_npc = false );
        void set_has_activity( const JsonObject &, std::string_view, bool is_npc = false ) {
            set_has_activity( is_npc );
        }
        void set_is_riding( bool is_npc = false );
        void set_is_riding( const JsonObject &, std::string_view, bool is_npc = false ) {
            set_is_riding( is_npc );
        }
        void set_npc_has_class( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_u_has_mission( const JsonObject &jo, std::string_view member );
        void set_u_monsters_in_direction( const JsonObject &jo, std::string_view member );
        void set_u_safe_mode_trigger( const JsonObject &jo, std::string_view member );
        void set_has_strength( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_dexterity( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_intelligence( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_perception( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_hp( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_part_temp( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_is_deaf( bool is_npc = false );
        void set_is_on_terrain( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_is_on_terrain_with_flag( const JsonObject &jo, std::string_view member,
                                          bool is_npc = false );
        void set_is_in_field( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_one_in_chance( const JsonObject &jo, std::string_view member );
        void set_query( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_query_tile( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_x_in_y_chance( const JsonObject &jo, std::string_view member );
        void set_has_worn_with_flag( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_wielded_with_flag( const JsonObject &jo, std::string_view member,
                                        bool is_npc = false );
        void set_has_wielded_with_weapon_category( const JsonObject &jo, std::string_view member,
                bool is_npc = false );
        void set_is_wearing( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_item( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_items( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_item_with_flag( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_item_category( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_bionics( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_effect( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_need( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_at_om_location( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_near_om_location( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_has_move_mode( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_using_martial_art( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_npc_role_nearby( const JsonObject &jo, std::string_view member );
        void set_npc_allies( const JsonObject &jo, std::string_view member );
        void set_npc_allies_global( const JsonObject &jo, std::string_view member );
        void set_u_has_cash( const JsonObject &jo, std::string_view member );
        void set_u_are_owed( const JsonObject &jo, std::string_view member );
        void set_npc_aim_rule( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_npc_engagement_rule( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_npc_cbm_reserve_rule( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_npc_cbm_recharge_rule( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_npc_rule( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_npc_override( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_days_since( const JsonObject &jo, std::string_view member );
        void set_is_season( const JsonObject &jo, std::string_view member );
        void set_is_weather( const JsonObject &jo, std::string_view member );
        void set_map_ter_furn_with_flag( const JsonObject &jo, std::string_view member );
        void set_map_in_city( const JsonObject &jo, std::string_view member );
        void set_mod_is_loaded( const JsonObject &jo, std::string_view member );
        void set_mission_goal( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_has_faction_trust( const JsonObject &jo, std::string_view member );
        void set_no_assigned_mission();
        void set_has_assigned_mission();
        void set_has_many_assigned_missions();
        void set_no_available_mission( bool is_npc );
        void set_has_available_mission( bool is_npc );
        void set_has_many_available_missions( bool is_npc );
        void set_mission_complete( bool is_npc );
        void set_mission_incomplete( bool is_npc );
        void set_mission_failed( bool is_npc );
        void set_npc_service( const JsonObject &, std::string_view, bool is_npc );
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
        void set_is_male( bool is_npc = false ) {
            set_is_gender( true, is_npc );
        }
        void set_is_female( bool is_npc = false ) {
            set_is_gender( false, is_npc );
        }
        void set_has_skill( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_roll_contested( const JsonObject &jo, std::string_view member );
        void set_u_know_recipe( const JsonObject &jo, std::string_view member );
        void set_mission_has_generic_rewards();
        void set_can_see( bool is_npc = false );
        void set_get_option( const JsonObject &jo, std::string_view member );
        void set_compare_string( const JsonObject &jo, std::string_view member );
        void set_get_condition( const JsonObject &jo, std::string_view member );
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
