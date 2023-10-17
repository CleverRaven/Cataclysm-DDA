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

struct condition_parser {
    using f_t = void ( conditional_t::* )( const JsonObject &, std::string_view );
    using f_t_beta = void ( conditional_t::* )( const JsonObject &, std::string_view, bool );
    using f_t_simple = void ( conditional_t::* )();
    using f_t_beta_simple = void ( conditional_t::* )( bool );

    condition_parser( std::string_view key_alpha_, jarg arg_, f_t f_ ) : key_alpha( key_alpha_ ),
        arg( arg_ ), f( f_ ) {}
    condition_parser( std::string_view key_alpha_, std::string_view key_beta_, jarg arg_,
                      f_t_beta f_ ) : key_alpha( key_alpha_ ), key_beta( key_beta_ ), arg( arg_ ), f_beta( f_ ) {
        has_beta = true;
    }
    condition_parser( std::string_view key_alpha_, f_t_simple f_ ) : key_alpha( key_alpha_ ),
        f_simple( f_ ) {
        has_beta = true;
    }
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
        explicit conditional_t( const std::string &type );
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
        void set_npc_available( const JsonObject &, std::string_view, bool is_npc ) {
            set_npc_available( is_npc );
        }
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

// When updating this, please also update `dynamic_line_string_keys` in
// `lang/string_extractor/parsers/talk_topic.py` so the lines are properly
// extracted for translation
namespace dialogue_data
{
inline const
std::vector<condition_parser>
parsers = {
    {"u_has_any_trait", "npc_has_any_trait", jarg::array, &conditional_t::set_has_any_trait },
    {"u_has_trait", "npc_has_trait", jarg::member, &conditional_t::set_has_trait },
    {"u_has_visible_trait", "npc_has_visible_trait", jarg::member, &conditional_t::set_has_visible_trait },
    {"u_has_martial_art", "npc_has_martial_art", jarg::member, &conditional_t::set_has_martial_art },
    {"u_has_flag", "npc_has_flag", jarg::member, &conditional_t::set_has_flag },
    {"u_has_species", "npc_has_species", jarg::member, &conditional_t::set_has_species },
    {"u_bodytype", "npc_bodytype", jarg::member, &conditional_t::set_bodytype },
    {"u_has_class", "npc_has_class", jarg::member, &conditional_t::set_npc_has_class },
    {"u_has_activity", "npc_has_activity", jarg::string, &conditional_t::set_has_activity },
    {"u_is_riding", "npc_is_riding", jarg::string, &conditional_t::set_is_riding },
    {"u_has_mission", jarg::string, &conditional_t::set_u_has_mission },
    {"u_monsters_in_direction", jarg::string, &conditional_t::set_u_monsters_in_direction },
    {"u_safe_mode_trigger", jarg::member, &conditional_t::set_u_safe_mode_trigger },
    {"u_has_strength", "npc_has_strength", jarg::member | jarg::array, &conditional_t::set_has_strength },
    {"u_has_dexterity", "npc_has_dexterity", jarg::member | jarg::array, &conditional_t::set_has_dexterity },
    {"u_has_intelligence", "npc_has_intelligence", jarg::member | jarg::array, &conditional_t::set_has_intelligence },
    {"u_has_perception", "npc_has_perception", jarg::member | jarg::array, &conditional_t::set_has_perception },
    {"u_has_hp", "npc_has_hp", jarg::member | jarg::array, &conditional_t::set_has_hp },
    {"u_has_part_temp", "npc_has_part_temp", jarg::member | jarg::array, &conditional_t::set_has_part_temp },
    {"u_is_wearing", "npc_is_wearing", jarg::member, &conditional_t::set_is_wearing },
    {"u_has_item", "npc_has_item", jarg::member, &conditional_t::set_has_item },
    {"u_has_item_with_flag", "npc_has_item_with_flag", jarg::member, &conditional_t::set_has_item_with_flag },
    {"u_has_items", "npc_has_items", jarg::member, &conditional_t::set_has_items },
    {"u_has_item_category", "npc_has_item_category", jarg::member, &conditional_t::set_has_item_category },
    {"u_has_bionics", "npc_has_bionics", jarg::member, &conditional_t::set_has_bionics },
    {"u_has_effect", "npc_has_effect", jarg::member, &conditional_t::set_has_effect },
    {"u_need", "npc_need", jarg::member, &conditional_t::set_need },
    {"u_query", "npc_query", jarg::member, &conditional_t::set_query },
    {"u_at_om_location", "npc_at_om_location", jarg::member, &conditional_t::set_at_om_location },
    {"u_near_om_location", "npc_near_om_location", jarg::member, &conditional_t::set_near_om_location },
    {"u_has_var", "npc_has_var", jarg::string, &conditional_t::set_has_var },
    {"expects_vars", jarg::member, &conditional_t::set_expects_vars },
    {"u_compare_var", "npc_compare_var", jarg::string, &conditional_t::set_compare_var },
    {"u_compare_time_since_var", "npc_compare_time_since_var", jarg::string, &conditional_t::set_compare_time_since_var },
    {"npc_role_nearby", jarg::string, &conditional_t::set_npc_role_nearby },
    {"npc_allies", jarg::member | jarg::array, &conditional_t::set_npc_allies },
    {"npc_allies_global", jarg::member | jarg::array, &conditional_t::set_npc_allies_global },
    {"u_service", "npc_service", jarg::member, &conditional_t::set_npc_available },
    {"u_has_cash", jarg::member | jarg::array, &conditional_t::set_u_has_cash },
    {"u_are_owed", jarg::member | jarg::array, &conditional_t::set_u_are_owed },
    {"u_aim_rule", "npc_aim_rule", jarg::member, &conditional_t::set_npc_aim_rule },
    {"u_engagement_rule", "npc_engagement_rule", jarg::member, &conditional_t::set_npc_engagement_rule },
    {"u_cbm_reserve_rule", "npc_cbm_reserve_rule", jarg::member, &conditional_t::set_npc_cbm_reserve_rule },
    {"u_cbm_recharge_rule", "npc_cbm_recharge_rule", jarg::member, &conditional_t::set_npc_cbm_recharge_rule },
    {"u_rule", "npc_rule", jarg::member, &conditional_t::set_npc_rule },
    {"u_override", "npc_override", jarg::member, &conditional_t::set_npc_override },
    {"days_since_cataclysm", jarg::member | jarg::array, &conditional_t::set_days_since },
    {"is_season", jarg::member, &conditional_t::set_is_season },
    {"u_mission_goal", "mission_goal", jarg::member, &conditional_t::set_mission_goal },
    {"u_mission_goal", "npc_mission_goal", jarg::member, &conditional_t::set_mission_goal },
    {"roll_contested", jarg::member, &conditional_t::set_roll_contested },
    {"u_know_recipe", jarg::member, &conditional_t::set_u_know_recipe },
    {"one_in_chance", jarg::member | jarg::array, &conditional_t::set_one_in_chance },
    {"x_in_y_chance", jarg::object, &conditional_t::set_x_in_y_chance },
    {"u_has_worn_with_flag", "npc_has_worn_with_flag", jarg::member, &conditional_t::set_has_worn_with_flag },
    {"u_has_wielded_with_flag", "npc_has_wielded_with_flag", jarg::member, &conditional_t::set_has_wielded_with_flag },
    {"u_has_wielded_with_weapon_category", "npc_has_wielded_with_weapon_category", jarg::member, &conditional_t::set_has_wielded_with_weapon_category },
    {"u_is_on_terrain", "npc_is_on_terrain", jarg::member, &conditional_t::set_is_on_terrain },
    {"u_is_on_terrain_with_flag", "npc_is_on_terrain_with_flag", jarg::member, &conditional_t::set_is_on_terrain_with_flag },
    {"u_is_in_field", "npc_is_in_field", jarg::member, &conditional_t::set_is_in_field },
    {"u_has_move_mode", "npc_has_move_mode", jarg::member, &conditional_t::set_has_move_mode },
    {"is_weather", jarg::member, &conditional_t::set_is_weather },
    {"mod_is_loaded", jarg::member, &conditional_t::set_mod_is_loaded },
    {"u_has_faction_trust", jarg::member | jarg::array, &conditional_t::set_has_faction_trust },
    {"compare_int", jarg::member, &conditional_t::set_compare_num },
    {"compare_num", jarg::member, &conditional_t::set_compare_num },
    {"math", jarg::member, &conditional_t::set_math },
    {"compare_string", jarg::member, &conditional_t::set_compare_string },
    {"get_condition", jarg::member, &conditional_t::set_get_condition },
    {"get_game_option", jarg::member, &conditional_t::set_get_option },
};

inline const
std::vector<condition_parser>
parsers_simple = {
    {"u_male", "npc_male", &conditional_t::set_is_male },
    {"u_female", "npc_female", &conditional_t::set_is_female },
    {"has_no_assigned_mission", &conditional_t::set_no_assigned_mission },
    {"has_assigned_mission", &conditional_t::set_has_assigned_mission },
    {"has_many_assigned_missions", &conditional_t::set_has_many_assigned_missions },
    {"u_has_no_available_mission", "has_no_available_mission", &conditional_t::set_no_available_mission },
    {"u_has_no_available_mission", "npc_has_no_available_mission", &conditional_t::set_no_available_mission },
    {"u_has_available_mission", "has_available_mission", &conditional_t::set_has_available_mission },
    {"u_has_available_mission", "npc_has_available_mission", &conditional_t::set_has_available_mission },
    {"u_has_many_available_missions", "has_many_available_missions", &conditional_t::set_has_many_available_missions },
    {"u_has_many_available_missions", "npc_has_many_available_missions", &conditional_t::set_has_many_available_missions },
    {"u_mission_complete", "mission_complete", &conditional_t::set_mission_complete },
    {"u_mission_complete", "npc_mission_complete", &conditional_t::set_mission_complete },
    {"u_mission_incomplete", "mission_incomplete", &conditional_t::set_mission_incomplete },
    {"u_mission_incomplete", "npc_mission_incomplete", &conditional_t::set_mission_incomplete },
    {"u_mission_failed", "mission_failed", &conditional_t::set_mission_failed },
    {"u_mission_failed", "npc_mission_failed", &conditional_t::set_mission_failed },
    {"u_available", "npc_available", &conditional_t::set_npc_available },
    {"u_following", "npc_following", &conditional_t::set_npc_following },
    {"u_friend", "npc_friend", &conditional_t::set_npc_friend },
    {"u_hostile", "npc_hostile", &conditional_t::set_npc_hostile },
    {"u_train_skills", "npc_train_skills", &conditional_t::set_npc_train_skills },
    {"u_train_styles", "npc_train_styles", &conditional_t::set_npc_train_styles },
    {"u_train_spells", "npc_train_spells", &conditional_t::set_npc_train_spells },
    {"u_at_safe_space", "at_safe_space", &conditional_t::set_at_safe_space },
    {"u_at_safe_space", "npc_at_safe_space", &conditional_t::set_at_safe_space },
    {"u_can_stow_weapon", "npc_can_stow_weapon", &conditional_t::set_can_stow_weapon },
    {"u_can_drop_weapon", "npc_can_drop_weapon", &conditional_t::set_can_drop_weapon },
    {"u_has_weapon", "npc_has_weapon", &conditional_t::set_has_weapon },
    {"u_driving", "npc_driving", &conditional_t::set_is_driving },
    {"u_has_activity", "npc_has_activity", &conditional_t::set_has_activity },
    {"u_is_riding", "npc_is_riding", &conditional_t::set_is_riding },
    {"is_day", &conditional_t::set_is_day },
    {"u_has_stolen_item", "npc_has_stolen_item", &conditional_t::set_has_stolen_item },
    {"u_is_outside", "is_outside", &conditional_t::set_is_outside },
    {"u_is_outside", "npc_is_outside", &conditional_t::set_is_outside },
    {"u_is_underwater", "npc_is_underwater", &conditional_t::set_is_underwater },
    {"u_has_camp", &conditional_t::set_u_has_camp },
    {"u_has_pickup_list", "has_pickup_list", &conditional_t::set_has_pickup_list },
    {"u_has_pickup_list", "npc_has_pickup_list", &conditional_t::set_has_pickup_list },
    {"is_by_radio", &conditional_t::set_is_by_radio },
    {"has_reason", &conditional_t::set_has_reason },
    {"mission_has_generic_rewards", &conditional_t::set_mission_has_generic_rewards },
    {"u_can_see", "npc_can_see", &conditional_t::set_can_see },
    {"u_is_deaf", "npc_is_deaf", &conditional_t::set_is_deaf },
    {"u_is_alive", "npc_is_alive", &conditional_t::set_is_alive },
};
} // namespace dialogue_data

#endif // CATA_SRC_CONDITION_H
