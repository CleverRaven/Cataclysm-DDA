#pragma once
#ifndef CATA_SRC_MATH_PARSER_DIAG_H
#define CATA_SRC_MATH_PARSER_DIAG_H

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "math_parser_diag_value.h"


struct dialogue;
struct const_dialogue;

using diag_assign_dbl_f = std::function<void( dialogue &, double )>;
using diag_eval_dbl_f = std::function<double( const_dialogue const & )>;

struct diag_kwargs {
    using impl_t = std::map<std::string, deref_diag_value>;

    impl_t kwargs;

    template<typename T = std::monostate>
    diag_value kwarg_or( std::string const &key, T const &default_value = {} ) const {
        if( auto it = kwargs.find( key ); it != kwargs.end() ) {
            return *( it->second );
        }
        return diag_value{ default_value };
    }
};
struct dialogue_func {
    dialogue_func( std::string_view sc_, int n_ ) :
        scopes( sc_ ), num_params( n_ ) {}
    std::string_view scopes;
    int num_params{};
};
struct dialogue_func_eval : dialogue_func {
    using f_t = diag_eval_dbl_f( * )( char scope, std::vector<diag_value> const &,
                                      diag_kwargs const & );

    dialogue_func_eval( std::string_view sc_, int n_, f_t f_ )
        : dialogue_func( sc_, n_ ), f( f_ ) {}

    f_t f;
};

struct dialogue_func_ass : dialogue_func {
    using f_t = diag_assign_dbl_f( * )( char scope, std::vector<diag_value> const &,
                                        diag_kwargs const & );

    dialogue_func_ass( std::string_view sc_, int n_, f_t f_ )
        : dialogue_func( sc_, n_ ), f( f_ ) {}

    f_t f;
};

using pdiag_func_eval = dialogue_func_eval const *;
using pdiag_func_ass = dialogue_func_ass const *;

std::map<std::string_view, dialogue_func_eval> const &get_all_diag_eval_funcs();
std::map<std::string_view, dialogue_func_ass> const &get_all_diag_ass_funcs();

#endif // CATA_SRC_MATH_PARSER_DIAG_H
