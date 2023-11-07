#pragma once
#ifndef CATA_SRC_MATH_PARSER_DIAG_H
#define CATA_SRC_MATH_PARSER_DIAG_H

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "dialogue_helpers.h"

class math_exp;
struct dialogue;
struct dialogue_func {
    dialogue_func( std::string_view sc_, int n_ ) :
        scopes( sc_ ), num_params( n_ ) {}
    std::string_view scopes;
    int num_params{};
};

struct diag_value {
    diag_value() = default;
    template <class U>
    explicit diag_value( U u ) : data( std::in_place_type<U>, std::forward<U>( u ) ) {}
    template <class T, class... Args>
    explicit diag_value( std::in_place_type_t<T> /*t*/, Args &&...args )
        : data( std::in_place_type<T>, std::forward<Args>( args )... ) {}

    // these functions can be used at parse time if the parameter needs to be of exactly this type
    // with no conversion. These throw so they should *NOT* be used at runtime.
    double dbl() const;
    std::string_view str() const;
    var_info var() const;

    // evaluate and possibly convert the parameter to this type
    double dbl( dialogue const &d ) const;
    std::string str( dialogue const &d ) const;

    using impl_t = std::variant<double, std::string, var_info, math_exp>;
    impl_t data;
};

constexpr bool operator==( diag_value const &lhs, std::string_view rhs )
{
    return std::holds_alternative<std::string>( lhs.data ) && std::get<std::string>( lhs.data ) == rhs;
}

// helper struct that makes it easy to determine whether a kwarg's value has been used
struct deref_diag_value {
    public:
        deref_diag_value() = default;
        explicit deref_diag_value( diag_value &&dv ) : _val( dv ) {}
        diag_value const *operator->() const {
            _used = true;
            return &_val;
        }
        diag_value const &operator*() const {
            _used = true;
            return _val;
        }
        bool was_used() const {
            return _used;
        }
    private:
        bool mutable _used = false;
        diag_value _val;
};

using diag_kwargs = std::map<std::string, deref_diag_value>;

struct dialogue_func_eval : dialogue_func {
    using f_t = std::function<double( dialogue & )> ( * )( char scope,
                std::vector<diag_value> const &, diag_kwargs const & );

    dialogue_func_eval( std::string_view sc_, int n_, f_t f_ )
        : dialogue_func( sc_, n_ ), f( f_ ) {}

    f_t f;
};

struct dialogue_func_ass : dialogue_func {
    using f_t = std::function<void( dialogue &, double )> ( * )( char scope,
                std::vector<diag_value> const &, diag_kwargs const & );

    dialogue_func_ass( std::string_view sc_, int n_, f_t f_ )
        : dialogue_func( sc_, n_ ), f( f_ ) {}

    f_t f;
};

using pdiag_func_eval = dialogue_func_eval const *;
using pdiag_func_ass = dialogue_func_ass const *;

using decl_diag_eval = std::function<double( dialogue & )> ( char scope,
                       std::vector<diag_value> const &params, diag_kwargs const &kwargs );
using decl_diag_ass = std::function<void( dialogue &, double )> ( char scope,
                      std::vector<diag_value> const &params, diag_kwargs const &kwargs );

decl_diag_eval armor_eval;
decl_diag_eval attack_speed_eval;
decl_diag_eval dodge_eval;
decl_diag_eval effect_intensity_eval;
decl_diag_eval hp_eval;
decl_diag_ass hp_ass;
decl_diag_eval hp_max_eval;
decl_diag_eval monsters_nearby_eval;
decl_diag_eval num_input_eval;
decl_diag_eval option_eval;
decl_diag_eval pain_eval;
decl_diag_ass pain_ass;
decl_diag_eval skill_eval;
decl_diag_ass skill_ass;
decl_diag_eval skill_exp_eval;
decl_diag_ass skill_exp_ass;
decl_diag_eval spell_exp_eval;
decl_diag_ass spell_exp_ass;
decl_diag_eval test_diag;
decl_diag_eval u_val;
decl_diag_ass u_val_ass;
decl_diag_eval warmth_eval;
decl_diag_eval weather_eval;
decl_diag_ass weather_ass;

inline std::map<std::string_view, dialogue_func_eval> const dialogue_eval_f{
    { "_test_diag_", { "g", -1, test_diag } },
    { "armor", { "un", 2, armor_eval } },
    { "attack_speed", { "un", 0, attack_speed_eval } },
    { "effect_intensity", { "un", 1, effect_intensity_eval } },
    { "game_option", { "g", 1, option_eval } },
    { "hp", { "un", -1, hp_eval } },
    { "hp_max", { "un", 1, hp_max_eval } },
    { "monsters_nearby", { "ung", -1, monsters_nearby_eval } },
    { "num_input", { "g", 2, num_input_eval } },
    { "pain", { "un", 0, pain_eval } },
    { "skill", { "un", 1, skill_eval } },
    { "skill_exp", { "un", -1, skill_exp_eval } },
    { "spell_exp", { "un", 1, spell_exp_eval}},
    { "val", { "un", -1, u_val } },
    { "warmth", { "un", 1, warmth_eval } },
    { "weather", { "g", 1, weather_eval } },
};

inline std::map<std::string_view, dialogue_func_ass> const dialogue_assign_f{
    { "hp", { "un", -1, hp_ass } },
    { "pain", { "un", 0, pain_ass } },
    { "skill", { "un", 1, skill_ass } },
    { "skill_exp", { "un", -1, skill_exp_ass } },
    { "spell_exp", { "un", 1, spell_exp_ass}},
    { "val", { "un", -1, u_val_ass } },
    { "weather", { "g", 1, weather_ass } },
};

#endif // CATA_SRC_MATH_PARSER_DIAG_H
