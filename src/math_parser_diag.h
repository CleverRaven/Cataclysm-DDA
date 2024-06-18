#pragma once
#ifndef CATA_SRC_MATH_PARSER_DIAG_H
#define CATA_SRC_MATH_PARSER_DIAG_H

#include <functional>
#include <map>
#include <string_view>
#include <vector>

struct diag_value;
struct deref_diag_value;
using diag_kwargs = std::map<std::string, deref_diag_value>;

struct dialogue;
struct dialogue_func {
    dialogue_func( std::string_view sc_, int n_ ) :
        scopes( sc_ ), num_params( n_ ) {}
    std::string_view scopes;
    int num_params{};
};
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

std::map<std::string_view, dialogue_func_eval> const &get_all_diag_eval_funcs();
std::map<std::string_view, dialogue_func_ass> const &get_all_diag_ass_funcs();

#endif // CATA_SRC_MATH_PARSER_DIAG_H
