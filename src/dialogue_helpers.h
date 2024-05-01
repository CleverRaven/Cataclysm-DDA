#pragma once
#ifndef CATA_SRC_DIALOGUE_HELPERS_H
#define CATA_SRC_DIALOGUE_HELPERS_H

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "calendar.h"
#include "debug.h"
#include "global_vars.h"
#include "translation.h"

class JsonArray;
class JsonObject;
class math_exp;
class npc;
struct dialogue;

using talkfunction_ptr = std::add_pointer_t<void ( npc & )>;
using dialogue_fun_ptr = std::add_pointer_t<void( npc & )>;

using trial_mod = std::pair<std::string, int>;
struct dbl_or_var;

template<class T>
struct abstract_var_info {
    abstract_var_info( var_type in_type, std::string in_name ): type( in_type ),
        name( std::move( in_name ) ) {}
    abstract_var_info( var_type in_type, std::string in_name, T in_default_val ): type( in_type ),
        name( std::move( in_name ) ), default_val( std::move( in_default_val ) ) {}
    abstract_var_info() : type( var_type::global ) {}
    var_type type;
    std::string name;
    T default_val;
};

using var_info = abstract_var_info<std::string>;
using translation_var_info = abstract_var_info<translation>;

template<class T>
struct abstract_str_or_var {
    std::optional<T> str_val;
    std::optional<abstract_var_info<T>> var_val;
    std::optional<T> default_val;
    std::optional<std::function<T( const dialogue & )>> function;
    std::string evaluate( dialogue const & ) const;
};

using str_or_var = abstract_str_or_var<std::string>;
using translation_or_var = abstract_str_or_var<translation>;

struct str_translation_or_var {
    std::variant<str_or_var, translation_or_var> val;
    std::string evaluate( dialogue const & ) const;
};

struct talk_effect_fun_t {
    public:
        using likely_reward_t = std::pair<dbl_or_var, str_or_var>;
        using likely_rewards_t = std::vector<likely_reward_t>;
        using func = std::function<void( dialogue &d )>;
    private:
        func function;
        likely_rewards_t likely_rewards;

    public:
        talk_effect_fun_t() = default;
        explicit talk_effect_fun_t( const talkfunction_ptr & );
        explicit talk_effect_fun_t( const std::function<void( npc & )> & );
        explicit talk_effect_fun_t( func && );
        static talk_effect_fun_t from_arithmetic( const JsonObject &jo, std::string_view member,
                bool no_result );

        likely_rewards_t &get_likely_rewards();
        void operator()( dialogue &d ) const {
            if( !function ) {
                return;
            }
            return function( d );
        }
};

template<class T>
std::string read_var_value( const abstract_var_info<T> &info, const dialogue &d );
template<class T>
std::optional<std::string> maybe_read_var_value(
    const abstract_var_info<T> &info, const dialogue &d, int call_depth = 0 );

var_info process_variable( const std::string &type );

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
        return dbl_val || var_val || math_val;
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
