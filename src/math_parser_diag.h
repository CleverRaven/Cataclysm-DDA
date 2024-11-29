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
    using fe_t = diag_eval_dbl_f( * )( char scope, std::vector<diag_value> const &,
                                       diag_kwargs const & );
    using fa_t = diag_assign_dbl_f( * )( char scope, std::vector<diag_value> const &,
                                         diag_kwargs const & );

    dialogue_func( std::string_view sc_, int n_, fe_t fe_, fa_t fa_ = {} )
        : scopes( sc_ ), num_params( n_ ), fe( fe_ ), fa( fa_ )
    {}
    std::string_view scopes;
    int num_params{};

    fe_t fe;
    fa_t fa;
};
using pdiag_func = dialogue_func const *;

std::map<std::string_view, dialogue_func> const &get_all_diag_funcs();

#endif // CATA_SRC_MATH_PARSER_DIAG_H
