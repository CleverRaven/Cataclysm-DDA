#pragma once
#ifndef CATA_SRC_DIALOGUE_HELPERS_H
#define CATA_SRC_DIALOGUE_HELPERS_H

#include "global_vars.h"
#include "optional.h"
#include "rng.h"
#include "type_id.h"

struct dialogue;
class npc;

using talkfunction_ptr = std::add_pointer<void ( npc & )>::type;
using dialogue_fun_ptr = std::add_pointer<void( npc & )>::type;

using trial_mod = std::pair<std::string, int>;


template<class T>
struct talk_effect_fun_t {
    private:
        std::function<void( const T &d )> function;
        std::vector<std::pair<int, itype_id>> likely_rewards;

    public:
        talk_effect_fun_t() = default;
        explicit talk_effect_fun_t( const talkfunction_ptr & );
        explicit talk_effect_fun_t( const std::function<void( npc & )> & );
        explicit talk_effect_fun_t( const std::function<void( const T &d )> & );
        void set_companion_mission( const std::string &role_id );
        void set_add_effect( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_remove_effect( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_add_trait( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_remove_trait( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_mutate( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_mutate_category( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_add_bionic( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_lose_bionic( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_message( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_add_wet( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_assign_activity( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_assign_mission( const JsonObject &jo, const std::string &member );
        void set_finish_mission( const JsonObject &jo, const std::string &member );
        void set_remove_active_mission( const JsonObject &jo, const std::string &member );
        void set_offer_mission( const JsonObject &jo, const std::string &member );
        void set_make_sound( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_run_eocs( const JsonObject &jo, const std::string &member );
        void set_run_npc_eocs( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_queue_eocs( const JsonObject &jo, const std::string &member );
        void set_switch( const JsonObject &jo, const std::string &member );
        void set_roll_remainder( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_weighted_list_eocs( const JsonObject &jo, const std::string &member );
        void set_mod_healthy( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_cast_spell( const JsonObject &jo, const std::string &member, bool is_npc,
                             bool targeted = false );
        void set_lightning();
        void set_next_weather();
        void set_hp( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_sound_effect( const JsonObject &jo, const std::string &member );
        void set_add_var( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_remove_var( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_adjust_var( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_u_spawn_item( const JsonObject &jo, const std::string &member, int count,
                               const std::string &container_name );
        void set_u_buy_item( const itype_id &item_name, int cost, int count,
                             const std::string &container_name, const JsonObject &jo );
        void set_u_spend_cash( int amount, const JsonObject &jo );
        void set_u_sell_item( const itype_id &item_name, int cost, int count, const JsonObject &jo );
        void set_consume_item( const JsonObject &jo, const std::string &member, int count, int charges,
                               bool is_npc = false );
        void set_remove_item_with( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_npc_change_faction( const std::string &faction_name );
        void set_npc_change_class( const std::string &class_name );
        void set_change_faction_rep( int rep_change );
        void set_add_debt( const std::vector<trial_mod> &debt_modifiers );
        void set_toggle_npc_rule( const std::string &rule );
        void set_set_npc_rule( const std::string &rule );
        void set_clear_npc_rule( const std::string &rule );
        void set_npc_engagement_rule( const std::string &setting );
        void set_npc_aim_rule( const std::string &setting );
        void set_npc_cbm_reserve_rule( const std::string &setting );
        void set_npc_cbm_recharge_rule( const std::string &setting );
        void set_location_variable( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_transform_radius( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_transform_line( const JsonObject &jo, const std::string &member );
        void set_place_override( const JsonObject &jo, const std::string &member );
        void set_mapgen_update( const JsonObject &jo, const std::string &member );
        void set_remove_npc( const JsonObject &jo, const std::string &member );
        void set_alter_timed_events( const JsonObject &jo, const std::string &member );
        void set_revert_location( const JsonObject &jo, const std::string &member );
        void set_npc_goal( const JsonObject &jo, const std::string &member );
        void set_bulk_trade_accept( bool is_trade, int quantity, bool is_npc = false );
        void set_npc_gets_item( bool to_use );
        void set_add_mission( const std::string &mission_id );
        const std::vector<std::pair<int, itype_id>> &get_likely_rewards() const;
        void set_u_buy_monster( const std::string &monster_type_id, int cost, int count, bool pacified,
                                const translation &name, const JsonObject &jo );
        void set_u_learn_recipe( const std::string &learned_recipe_id );
        void set_npc_first_topic( const std::string &chat_topic );
        void set_add_morale( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_lose_morale( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_add_faction_trust( const JsonObject &jo, const std::string &member );
        void set_lose_faction_trust( const JsonObject &jo, const std::string &member );
        void set_arithmetic( const JsonObject &jo, const std::string &member, bool no_result );
        void set_set_string_var( const JsonObject &jo, const std::string &member );
        void set_custom_light_level( const JsonObject &jo, const std::string &member );
        void set_spawn_monster( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_field( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_teleport( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_give_equipment( const JsonObject &jo, const std::string &member );
        void set_open_dialogue( const JsonObject &jo, const std::string &member );
        void set_take_control( const JsonObject &jo );
        void set_take_control_menu();
        void operator()( const T &d ) const {
            if( !function ) {
                return;
            }
            return function( d );
        }
};


struct var_info {
    var_info( var_type in_type, std::string in_name ): type( in_type ),
        name( std::move( in_name ) ) {}
    var_info( var_type in_type, std::string in_name, std::string in_default_val ): type( in_type ),
        name( std::move( in_name ) ), default_val( std::move( in_default_val ) ) {}
    var_type type;
    std::string name;
    std::string default_val;
};

template<class T>
static std::string read_var_value( var_info info, const T &d )
{
    std::string ret_val;
    global_variables &globvars = get_globals();
    switch( info.type ) {
        case var_type::global:
            ret_val = globvars.get_global_value( info.name );
            break;
        case var_type::u:
            ret_val = d.actor( false )->get_value( info.name );
            break;
        case var_type::npc:
            ret_val = d.actor( true )->get_value( info.name );
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
    if( ret_val.empty() ) {
        ret_val = info.default_val;
    }
    return ret_val;
}


template<class T>
struct str_or_var {
    cata::optional<std::string> str_val;
    cata::optional<var_info> var_val;
    cata::optional<std::string> default_val;
    std::string evaluate( const T &d ) const {
        if( str_val.has_value() ) {
            return str_val.value();
        } else if( var_val.has_value() ) {
            std::string val = read_var_value( var_val.value(), d );
            if( !val.empty() ) {
                return std::string( val );
            }
            if( default_val.has_value() ) {
                return default_val.value();
            } else {
                debugmsg( "No default provided for str_or_var_part" );
                return "";
            }
        } else {
            debugmsg( "No valid value for str_or_var_part." );
            return "";
        }
    }
};

template<class T>
struct int_or_var_part {
    cata::optional<int> int_val;
    cata::optional<var_info> var_val;
    cata::optional<int> default_val;
    cata::optional<talk_effect_fun_t<T>> arithmetic_val;
    int evaluate( const T &d ) const {
        if( int_val.has_value() ) {
            return int_val.value();
        } else if( var_val.has_value() ) {
            std::string val = read_var_value( var_val.value(), d );
            if( !val.empty() ) {
                return std::stoi( val );
            }
            if( default_val.has_value() ) {
                return default_val.value();
            } else {
                debugmsg( "No default provided for int_or_var_part" );
                return 0;
            }
        } else if( arithmetic_val.has_value() ) {
            arithmetic_val.value()( d );
            var_info info = var_info( var_type::global, "temp_var" );
            std::string val = read_var_value( info, d );
            if( !val.empty() ) {
                return std::stoi( val );
            } else {
                debugmsg( "No valid arithmetic value for int_or_var_part." );
                return 0;
            }
        } else {
            debugmsg( "No valid value for int_or_var_part." );
            return 0;
        }
    }
};

template<class T>
struct int_or_var {
    bool pair = false;
    int_or_var_part<T> min;
    int_or_var_part<T> max;
    int evaluate( const T &d ) const {
        if( pair ) {
            return rng( min.evaluate( d ), max.evaluate( d ) );
        } else {
            return min.evaluate( d );
        }
    }
};

template<class T>
struct duration_or_var_part {
    cata::optional<time_duration> dur_val;
    cata::optional<var_info> var_val;
    cata::optional<time_duration> default_val;
    cata::optional<talk_effect_fun_t<T>> arithmetic_val;
    time_duration evaluate( const T &d ) const {
        if( dur_val.has_value() ) {
            return dur_val.value();
        } else if( var_val.has_value() ) {
            std::string val = read_var_value( var_val.value(), d );
            if( !val.empty() ) {
                time_duration ret_val;
                ret_val = time_duration::from_turns( std::stoi( val ) );
                return ret_val;
            }
            if( default_val.has_value() ) {
                return default_val.value();
            } else {
                debugmsg( "No default provided for duration_or_var_part" );
                return 0_seconds;
            }
        } else if( arithmetic_val.has_value() ) {
            arithmetic_val.value()( d );
            var_info info = var_info( var_type::global, "temp_var" );
            std::string val = read_var_value( info, d );
            if( !val.empty() ) {
                time_duration ret_val;
                ret_val = time_duration::from_turns( std::stoi( val ) );
                return ret_val;
            } else {
                debugmsg( "No valid arithmetic value for duration_or_var_part." );
                return 0_seconds;
            }
        } else {
            debugmsg( "No valid value for duration_or_var_part." );
            return 0_seconds;
        }
    }
};

template<class T>
struct duration_or_var {
    bool pair = false;
    duration_or_var_part<dialogue> min;
    duration_or_var_part<dialogue> max;
    time_duration evaluate( const T &d ) const {
        if( pair ) {
            return rng( min.evaluate( d ), max.evaluate( d ) );
        } else {
            return min.evaluate( d );
        }
    }
};

#endif // CATA_SRC_DIALOGUE_HELPERS_H
