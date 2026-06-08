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
#include "translation.h"

class JsonObject;
class JsonValue;
class math_exp;
class npc;
enum class math_type_t : int;
struct const_dialogue;
struct diag_value;
struct dialogue;

using talkfunction_ptr = std::add_pointer_t<void ( npc & )>;
using dialogue_fun_ptr = std::add_pointer_t<void( npc & )>;

using trial_mod = std::pair<std::string, int>;

enum class var_type : int {
    u,
    npc,
    global,
    context,
    var,
    last
};

#pragma GCC diagnostic push
#ifndef __clang__
# pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
struct var_info {
    var_info( var_type in_type, std::string in_name ): type( in_type ),
        name( std::move( in_name ) ) {}
    var_info() : type( var_type::last ) {}
    var_type type;
    std::string name;

    void _deserialize( JsonObject const &jo );
    void deserialize( JsonValue const &jsin );
};
#pragma GCC diagnostic pop

diag_value const &read_var_value( const var_info &info, const_dialogue const &d );
diag_value const *maybe_read_var_value( const var_info &info, const_dialogue const &d );

var_info process_variable( const std::string &type );

struct eoc_math {
    std::shared_ptr<math_exp> exp;

    void from_json( const JsonObject &jo, std::string_view member, math_type_t type_ );
    // needed for value_or_var. assumes math_type_t = ret. use from_json() instead of this
    void deserialize( JsonValue const &jsin );

    template<typename D>
    double act( D &d ) const;
};

template<typename valueT, typename... funcT>
struct value_or_var {
    std::variant<valueT, var_info, funcT...> val;
    std::optional<valueT> default_val;

    void deserialize( JsonValue const &jsin );
    valueT constant() const;
    valueT evaluate( const_dialogue const &d ) const;

    bool is_constant() const {
        return std::holds_alternative<valueT>( val );
    }

    value_or_var() = default;

    template <typename D>
    explicit value_or_var( D d ) : val( std::in_place_type<valueT>, std::forward<D>( d ) ) {}

    value_or_var &operator=( valueT const &rhs ) {
        val = rhs;
        return *this;
    }
};

template<typename valueT, typename... funcT>
struct value_or_var_pair {
    using part_t = value_or_var<valueT, funcT...>;

    part_t min;
    std::optional<part_t> max;
    void deserialize( JsonValue const &jsin );
    valueT evaluate( const_dialogue const &d ) const;

    bool is_constant() const {
        return !max && min.is_constant();
    }

    valueT constant() const {
        return min.constant();
    }

    value_or_var_pair() = default;
    template <class D>
    explicit value_or_var_pair( D d ) : min( d ) {}

    value_or_var_pair &operator=( valueT const &rhs ) {
        min = rhs;
        max.reset();
        return *this;
    }
};

template<typename T>
struct string_mutator {
    std::function<T( const_dialogue const &/* d */ )> ret_func;

    T operator()( const_dialogue const &d ) const {
        return ret_func( d );
    }

    void deserialize( JsonValue const &jsin );
};

extern template struct value_or_var<double, eoc_math>;
extern template struct value_or_var_pair<double, eoc_math>;
using dbl_or_var = value_or_var_pair<double, eoc_math>;

extern template struct value_or_var<time_duration, eoc_math>;
extern template struct value_or_var_pair<time_duration, eoc_math>;
using duration_or_var = value_or_var_pair<time_duration, eoc_math>;

extern template struct value_or_var<std::string, string_mutator<std::string>>;
extern template struct value_or_var<translation, string_mutator<translation>>;
using str_or_var = value_or_var<std::string, string_mutator<std::string>>;
using translation_or_var = value_or_var<translation, string_mutator<translation>>;

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
            function( d );
        }
};

#endif // CATA_SRC_DIALOGUE_HELPERS_H
