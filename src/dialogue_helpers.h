#pragma once
#ifndef CATA_SRC_DIALOGUE_HELPERS_H
#define CATA_SRC_DIALOGUE_HELPERS_H

#include <optional>

#include "calendar.h"
#include "global_vars.h"
#include "math_parser.h"
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
        void set_learn_martial_art( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_forget_martial_art( const JsonObject &jo, const std::string &member, bool is_npc = false );
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
        void set_give_achievment( const JsonObject &jo, const std::string &member );
        void set_add_var( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_remove_var( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_adjust_var( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_u_spawn_item( const JsonObject &jo, const std::string &member );
        void set_u_buy_item( const JsonObject &jo, const std::string &member );
        void set_u_spend_cash( const JsonObject &jo, const std::string &member );
        void set_u_sell_item( const JsonObject &jo, const std::string &member );
        void set_consume_item( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_remove_item_with( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_npc_change_faction( const JsonObject &jo, const std::string &member );
        void set_npc_change_class( const JsonObject &jo, const std::string &member );
        void set_change_faction_rep( const JsonObject &jo, const std::string &member );
        void set_add_debt( const std::vector<trial_mod> &debt_modifiers );
        void set_toggle_npc_rule( const JsonObject &jo, const std::string &member );
        void set_set_npc_rule( const JsonObject &jo, const std::string &member );
        void set_clear_npc_rule( const JsonObject &jo, const std::string &member );
        void set_npc_engagement_rule( const JsonObject &jo, const std::string &member );
        void set_npc_aim_rule( const JsonObject &jo, const std::string &member );
        void set_npc_cbm_reserve_rule( const JsonObject &jo, const std::string &member );
        void set_npc_cbm_recharge_rule( const JsonObject &jo, const std::string &member );
        void set_location_variable( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_transform_radius( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_transform_line( const JsonObject &jo, const std::string &member );
        void set_place_override( const JsonObject &jo, const std::string &member );
        void set_mapgen_update( const JsonObject &jo, const std::string &member );
        void set_alter_timed_events( const JsonObject &jo, const std::string &member );
        void set_revert_location( const JsonObject &jo, const std::string &member );
        void set_npc_goal( const JsonObject &jo, const std::string &member );
        void set_bulk_trade_accept( const JsonObject &jo, const std::string &member, bool is_npc = false );
        void set_npc_gets_item( bool to_use );
        void set_add_mission( const JsonObject &jo, const std::string &member );
        const std::vector<std::pair<int, itype_id>> &get_likely_rewards() const;
        void set_u_buy_monster( const JsonObject &jo, const std::string &member );
        void set_u_learn_recipe( const JsonObject &jo, const std::string &member );
        void set_npc_first_topic( const JsonObject &jo, const std::string &member );
        void set_add_morale( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_lose_morale( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_add_faction_trust( const JsonObject &jo, const std::string &member );
        void set_lose_faction_trust( const JsonObject &jo, const std::string &member );
        void set_arithmetic( const JsonObject &jo, const std::string &member, bool no_result );
        void set_math( const JsonObject &jo, const std::string &member );
        void set_set_string_var( const JsonObject &jo, const std::string &member );
        void set_custom_light_level( const JsonObject &jo, const std::string &member );
        void set_spawn_monster( const JsonObject &jo, const std::string &member, bool is_npc );
        void set_spawn_npc( const JsonObject &jo, const std::string &member, bool is_npc );
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
static std::string read_var_value( const var_info &info, const T &d )
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
    std::optional<std::string> str_val;
    std::optional<var_info> var_val;
    std::optional<std::string> default_val;
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
                std::string var_name = var_val.value().name;
                if( var_name.find( "npctalk_var" ) != std::string::npos ) {
                    var_name = var_name.substr( 12 );
                }
                debugmsg( "No default value provided for str_or_var_part while encountering unused variable %s.  Add a \"default_str\" member to prevent this.",
                          var_name );
                return "";
            }
        } else {
            debugmsg( "No valid value for str_or_var_part." );
            return "";
        }
    }
};

template<typename D>
struct eoc_math {
    enum class oper : int {
        ret = 0,
        assign,

        // these need mhs
        plus_assign,
        minus_assign,
        mult_assign,
        div_assign,
        mod_assign,
        increase,
        decrease,

        equal,
        less,
        equal_or_less,
        greater,
        equal_or_greater,
    };
    math_exp<D> lhs;
    math_exp<D> mhs;
    math_exp<D> rhs;
    eoc_math::oper action;

    void from_json( const JsonObject &jo, std::string const &member );
    double act( D const &d ) const;
};

template<class T>
struct dbl_or_var_part {
    std::optional<double> dbl_val;
    std::optional<var_info> var_val;
    std::optional<double> default_val;
    std::optional<talk_effect_fun_t<T>> arithmetic_val;
    std::optional<eoc_math<T>> math_val;
    double evaluate( const T &d ) const {
        if( dbl_val.has_value() ) {
            return dbl_val.value();
        } else if( var_val.has_value() ) {
            std::string val = read_var_value( var_val.value(), d );
            if( !val.empty() ) {
                return std::stof( val );
            }
            if( default_val.has_value() ) {
                return default_val.value();
            } else {
                std::string var_name = var_val.value().name;
                if( var_name.find( "npctalk_var" ) != std::string::npos ) {
                    var_name = var_name.substr( 12 );
                }
                debugmsg( "No default value provided for dbl_or_var_part while encountering unused variable %s.  Add a \"default\" member to prevent this.",
                          var_name );
                return 0;
            }
        } else if( arithmetic_val.has_value() ) {
            arithmetic_val.value()( d );
            var_info info = var_info( var_type::global, "temp_var" );
            std::string val = read_var_value( info, d );
            if( !val.empty() ) {
                return std::stof( val );
            } else {
                debugmsg( "No valid arithmetic value for dbl_or_var_part." );
                return 0;
            }
        } else if( math_val ) {
            return math_val->act( d );
        } else {
            debugmsg( "No valid value for dbl_or_var_part." );
            return 0;
        }
    }
};

template<class T>
struct dbl_or_var {
    bool pair = false;
    dbl_or_var_part<T> min;
    dbl_or_var_part<T> max;
    double evaluate( const T &d ) const {
        if( pair ) {
            return rng( min.evaluate( d ), max.evaluate( d ) );
        } else {
            return min.evaluate( d );
        }
    }
};

template<class T>
struct duration_or_var_part {
    std::optional<time_duration> dur_val;
    std::optional<var_info> var_val;
    std::optional<time_duration> default_val;
    std::optional<talk_effect_fun_t<T>> arithmetic_val;
    std::optional<eoc_math<T>> math_val;
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
                std::string var_name = var_val.value().name;
                if( var_name.find( "npctalk_var" ) != std::string::npos ) {
                    var_name = var_name.substr( 12 );
                }
                debugmsg( "No default value provided for duration_or_var_part while encountering unused variable %s.  Add a \"default\" member to prevent this.",
                          var_name );
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
        } else if( math_val ) {
            return time_duration::from_turns( math_val->act( d ) );
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
