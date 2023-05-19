#ifndef CATA_SRC_MATH_PARSER_DIAG_H
#define CATA_SRC_MATH_PARSER_DIAG_H

#include <array>
#include <functional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "dialogue_helpers.h"

struct dialogue;
struct dialogue_func {
    dialogue_func( std::string_view s_, std::string_view sc_, int n_ ) : symbol( s_ ),
        scopes( sc_ ), num_params( n_ ) {}
    std::string_view symbol;
    std::string_view scopes;
    int num_params{};
};

struct diag_value {
    std::string_view sv() const;
    std::string str() const;
    std::string eval( dialogue const &d ) const;

    using impl_t = std::variant<std::string, var_info>;
    impl_t data;
};

constexpr bool operator==( diag_value const &lhs, std::string_view rhs )
{
    return std::holds_alternative<std::string>( lhs.data ) && std::get<std::string>( lhs.data ) == rhs;
}

struct dialogue_func_eval : dialogue_func {
    using f_t = std::function<double( dialogue & )> ( * )( char scope,
                std::vector<diag_value> const & );

    dialogue_func_eval( std::string_view s_, std::string_view sc_, int n_, f_t f_ )
        : dialogue_func( s_, sc_, n_ ), f( f_ ) {}

    f_t f;
};

struct dialogue_func_ass : dialogue_func {
    using f_t = std::function<void( dialogue &, double )> ( * )( char scope,
                std::vector<diag_value> const & );

    dialogue_func_ass( std::string_view s_, std::string_view sc_, int n_, f_t f_ )
        : dialogue_func( s_, sc_, n_ ), f( f_ ) {}

    f_t f;
};

using pdiag_func_eval = dialogue_func_eval const *;
using pdiag_func_ass = dialogue_func_ass const *;

std::function<double( dialogue & )> u_val( char scope,
        std::vector<diag_value> const &params );
std::function<void( dialogue &, double )> u_val_ass( char scope,
        std::vector<diag_value> const &params );

std::function<double( dialogue & )> option_eval( char scope,
        std::vector<diag_value> const &params );

std::function<double( dialogue & )> armor_eval( char scope,
        std::vector<diag_value> const &params );

std::function<double( dialogue & )> pain_eval( char scope,
        std::vector<diag_value> const &/* params */ );

std::function<void( dialogue &, double )> pain_ass( char scope,
        std::vector<diag_value> const &/* params */ );

std::function<double( dialogue & )> skill_eval( char scope,
        std::vector<diag_value> const &params );
std::function<void( dialogue &, double )> skill_ass( char scope,
        std::vector<diag_value> const &params );

std::function<double( dialogue & )> weather_eval( char /* scope */,
        std::vector<diag_value> const &params );
std::function<void( dialogue &, double )> weather_ass( char /* scope */,
        std::vector<diag_value> const &params );

inline std::array<dialogue_func_eval, 6> const dialogue_eval_f{
    dialogue_func_eval{ "val", "un", -1, u_val },
    dialogue_func_eval{ "game_option", "g", -1, option_eval },
    dialogue_func_eval{ "pain", "un", 0, pain_eval },
    dialogue_func_eval{ "skill", "un", 1, skill_eval },
    dialogue_func_eval{ "weather", "g", 1, weather_eval },
    dialogue_func_eval{ "armor", "un", 2, armor_eval }
};

inline std::array<dialogue_func_ass, 4> const dialogue_assign_f{
    dialogue_func_ass{ "val", "un", -1, u_val_ass },
    dialogue_func_ass{ "pain", "un", 0, pain_ass },
    dialogue_func_ass{ "skill", "un", 1, skill_ass },
    dialogue_func_ass{ "weather", "g", 1, weather_ass },
};

#endif // CATA_SRC_MATH_PARSER_DIAG_H
