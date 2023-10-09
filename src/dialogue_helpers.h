#pragma once
#ifndef CATA_SRC_DIALOGUE_HELPERS_H
#define CATA_SRC_DIALOGUE_HELPERS_H

#include <optional>

#include "calendar.h"
#include "global_vars.h"
#include "math_parser.h"
#include "rng.h"
#include "translation.h"
#include "type_id.h"

struct dialogue;
class npc;

using talkfunction_ptr = std::add_pointer<void ( npc & )>::type;
using dialogue_fun_ptr = std::add_pointer<void( npc & )>::type;

using trial_mod = std::pair<std::string, int>;

struct talk_effect_fun_t {
    private:
        std::function<void( dialogue &d )> function;
        std::vector<std::pair<int, itype_id>> likely_rewards;

    public:
        talk_effect_fun_t() = default;
        explicit talk_effect_fun_t( const talkfunction_ptr & );
        explicit talk_effect_fun_t( const std::function<void( npc & )> & );
        explicit talk_effect_fun_t( const std::function<void( dialogue const &d )> & );
        void set_companion_mission( const JsonObject &jo, std::string_view member );
        void set_add_effect( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_remove_effect( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_add_trait( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_remove_trait( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_activate_trait( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_deactivate_trait( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_learn_martial_art( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_forget_martial_art( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_mutate( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_mutate_category( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_add_bionic( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_lose_bionic( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_message( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_add_wet( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_assign_activity( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_assign_mission( const JsonObject &jo, std::string_view member );
        void set_finish_mission( const JsonObject &jo, std::string_view member );
        void set_remove_active_mission( const JsonObject &jo, std::string_view member );
        void set_offer_mission( const JsonObject &jo, std::string_view member );
        void set_make_sound( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_run_eocs( const JsonObject &jo, std::string_view member );
        void set_run_eoc_with( const JsonObject &jo, std::string_view member );
        void set_run_eoc_until( const JsonObject &jo, std::string_view member );
        void set_run_eoc_selector( const JsonObject &jo, std::string_view member );
        void set_run_npc_eocs( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_run_inv_eocs( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_queue_eocs( const JsonObject &jo, std::string_view member );
        void set_queue_eoc_with( const JsonObject &jo, std::string_view member );
        void set_if( const JsonObject &jo, std::string_view member );
        void set_switch( const JsonObject &jo, std::string_view member );
        void set_roll_remainder( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_weighted_list_eocs( const JsonObject &jo, std::string_view member );
        void set_mod_healthy( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_cast_spell( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_attack( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_die( bool is_npc );
        void set_lightning();
        void set_next_weather();
        void set_hp( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_sound_effect( const JsonObject &jo, std::string_view member );
        void set_give_achievment( const JsonObject &jo, std::string_view member );
        void set_add_var( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_remove_var( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_adjust_var( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_u_spawn_item( const JsonObject &jo, std::string_view member );
        void set_u_buy_item( const JsonObject &jo, std::string_view member );
        void set_u_spend_cash( const JsonObject &jo, std::string_view member );
        void set_u_sell_item( const JsonObject &jo, std::string_view member );
        void set_consume_item( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_remove_item_with( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_npc_change_faction( const JsonObject &jo, std::string_view member );
        void set_npc_change_class( const JsonObject &jo, std::string_view member );
        void set_change_faction_rep( const JsonObject &jo, std::string_view member );
        void set_add_debt( const JsonObject &jo, std::string_view member );
        void set_toggle_npc_rule( const JsonObject &jo, std::string_view member );
        void set_set_npc_rule( const JsonObject &jo, std::string_view member );
        void set_clear_npc_rule( const JsonObject &jo, std::string_view member );
        void set_npc_engagement_rule( const JsonObject &jo, std::string_view member );
        void set_npc_aim_rule( const JsonObject &jo, std::string_view member );
        void set_npc_cbm_reserve_rule( const JsonObject &jo, std::string_view member );
        void set_npc_cbm_recharge_rule( const JsonObject &jo, std::string_view member );
        void set_location_variable( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_location_variable_adjust( const JsonObject &jo, std::string_view member );
        void set_transform_radius( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_transform_line( const JsonObject &jo, std::string_view member );
        void set_place_override( const JsonObject &jo, std::string_view member );
        void set_mapgen_update( const JsonObject &jo, std::string_view member );
        void set_alter_timed_events( const JsonObject &jo, std::string_view member );
        void set_npc_goal( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_destination( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_revert_location( const JsonObject &jo, std::string_view member );
        void set_guard_pos( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_bulk_trade_accept( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_npc_gets_item( bool to_use );
        void set_add_mission( const JsonObject &jo, std::string_view member );
        const std::vector<std::pair<int, itype_id>> &get_likely_rewards() const;
        void set_u_buy_monster( const JsonObject &jo, std::string_view member );
        void set_learn_recipe( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_forget_recipe( const JsonObject &jo, std::string_view member, bool is_npc = false );
        void set_npc_first_topic( const JsonObject &jo, std::string_view member );
        void set_add_morale( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_lose_morale( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_add_faction_trust( const JsonObject &jo, std::string_view member );
        void set_lose_faction_trust( const JsonObject &jo, std::string_view member );
        void set_arithmetic( const JsonObject &jo, std::string_view member, bool no_result );
        void set_math( const JsonObject &jo, std::string_view member );
        void set_set_string_var( const JsonObject &jo, std::string_view member );
        void set_set_condition( const JsonObject &jo, std::string_view member );
        void set_custom_light_level( const JsonObject &jo, std::string_view member );
        void set_spawn_monster( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_spawn_npc( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_field( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_teleport( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_give_equipment( const JsonObject &jo, std::string_view member );
        void set_open_dialogue( const JsonObject &jo, std::string_view member );
        void set_take_control( const JsonObject &jo, std::string_view dummy = "" );
        void set_take_control_menu();
        void set_set_flag( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_unset_flag( const JsonObject &jo, std::string_view member, bool is_npc );
        void set_activate( const JsonObject &jo, std::string_view member, bool is_npc );
        void operator()( dialogue &d ) const {
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

std::string read_var_value( const var_info &info, const dialogue &d );

var_info process_variable( const std::string &type );

template<class T>
struct abstract_str_or_var {
    std::optional<T> str_val;
    std::optional<var_info> var_val;
    std::optional<T> default_val;
    std::optional<std::function<T( const dialogue & )>> function;
    std::string evaluate( dialogue const & ) const;
};

using str_or_var = abstract_str_or_var<std::string>;
using translation_or_var = abstract_str_or_var<translation>;

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
        not_equal,
        less,
        equal_or_less,
        greater,
        equal_or_greater,

        invalid,
    };
    enum class type_t : int {
        ret = 0,
        compare,
        assign,
    };
    std::shared_ptr<math_exp> lhs;
    std::shared_ptr<math_exp> mhs;
    std::shared_ptr<math_exp> rhs;
    eoc_math::oper action = oper::invalid;

    void from_json( const JsonObject &jo, std::string_view member, type_t type_ );
    double act( dialogue &d ) const;
    void _validate_type( JsonArray const &objects, type_t type_ ) const;
};

struct dbl_or_var_part {
    std::optional<double> dbl_val;
    std::optional<var_info> var_val;
    std::optional<double> default_val;
    std::optional<talk_effect_fun_t> arithmetic_val;
    std::optional<eoc_math> math_val;
    double evaluate( dialogue &d ) const;

    bool is_constant() const {
        return dbl_val.has_value();
    }

    double constant() const {
        if( !dbl_val ) {
            debugmsg( "this dbl_or_var is not a constant" );
            return 0;
        }
        return *dbl_val;
    }

    explicit operator bool() const {
        return dbl_val || var_val || arithmetic_val || math_val;
    }

    dbl_or_var_part() = default;
    // construct from numbers
    template <class D, typename std::enable_if_t<std::is_arithmetic_v<D>>* = nullptr>
    explicit dbl_or_var_part( D d ) : dbl_val( d ) {}
};

struct dbl_or_var {
    bool pair = false;
    dbl_or_var_part min;
    dbl_or_var_part max;
    double evaluate( dialogue &d ) const;

    bool is_constant() const {
        return !max && min.is_constant();
    }

    double constant() const {
        return min.constant();
    }

    explicit operator bool() const {
        return static_cast<bool>( min );
    }

    dbl_or_var() = default;
    // construct from numbers
    template <class D, typename std::enable_if_t<std::is_arithmetic_v<D>>* = nullptr>
    explicit dbl_or_var( D d ) : min( d ) {}
};

struct duration_or_var_part {
    std::optional<time_duration> dur_val;
    std::optional<var_info> var_val;
    std::optional<time_duration> default_val;
    std::optional<talk_effect_fun_t> arithmetic_val;
    std::optional<eoc_math> math_val;
    time_duration evaluate( dialogue &d ) const;
};

struct duration_or_var {
    bool pair = false;
    duration_or_var_part min;
    duration_or_var_part max;
    time_duration evaluate( dialogue &d ) const;
};

#endif // CATA_SRC_DIALOGUE_HELPERS_H
