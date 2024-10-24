#pragma once
#ifndef CATA_SRC_MATH_PARSER_DIAG_H
#define CATA_SRC_MATH_PARSER_DIAG_H

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

struct diag_value;
struct deref_diag_value;
using diag_kwargs = std::map<std::string, deref_diag_value>;

struct dialogue;
struct dialogue_func {
    using fe_t = std::function<double( dialogue & )> ( * )( char scope,
                 std::vector<diag_value> const &, diag_kwargs const & );
    using fa_t = std::function<void( dialogue &, double )> ( * )( char scope,
                 std::vector<diag_value> const &, diag_kwargs const & );

    dialogue_func( std::string_view sc_, int n_, fe_t fe_, fa_t fa_ = {} )
        : scopes( sc_ ), num_params( n_ ), fe( fe_ ), fa( fa_ ) {}
    std::string_view scopes;
    int num_params{};

    fe_t fe;
    fa_t fa;
};
using pdiag_func = dialogue_func const *;

std::map<std::string_view, dialogue_func> const &get_all_diag_funcs();

#endif // CATA_SRC_MATH_PARSER_DIAG_H
