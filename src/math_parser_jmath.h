#pragma once
#ifndef CATA_SRC_MATH_PARSER_JMATH_H
#define CATA_SRC_MATH_PARSER_JMATH_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "math_parser.h"
#include "type_id.h"

class JsonObject;
struct const_dialogue;

struct jmath_func {
    jmath_func_id id;
    std::vector<std::pair<jmath_func_id, mod_id>> src;
    bool was_loaded = false;
    int num_params{};

    double eval( const_dialogue const &d ) const;
    double eval( const_dialogue const &d, std::vector<double> const &params ) const;

    void load( const JsonObject &jo, std::string_view src );
    static void load_func( const JsonObject &jo, std::string const &src );
    static void finalize();
    static void reset();
    static const std::vector<jmath_func> &get_all();

    mutable std::string _str;
    mutable math_exp _exp;
};

#endif // CATA_SRC_MATH_PARSER_JMATH_H
