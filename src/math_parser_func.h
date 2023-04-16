#ifndef CATA_SRC_MATH_PARSER_FUNC_H
#define CATA_SRC_MATH_PARSER_FUNC_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "rng.h"

struct math_func {
    std::string_view symbol;
    int num_params;
    using f_t = double ( * )( std::vector<double> & );
    f_t f;
};
using pmath_func = math_func const *;

struct math_const {
    std::string_view symbol;
    double val;
};
using pmath_const = math_const const *;

inline double abs( std::vector<double> &params )
{
    return std::abs( params[0] );
}

inline double max( std::vector<double> &params )
{
    if( params.empty() ) {
        return 0;
    }
    return *std::max_element( params.begin(), params.end() );
}

inline double min( std::vector<double> &params )
{
    if( params.empty() ) {
        return 0;
    }
    return *std::min_element( params.begin(), params.end() );
}

inline double math_rng( std::vector<double> &params )
{
    return rng_float( params[0], params[1] );
}

inline double rand( std::vector<double> &params )
{
    return rng( 0, static_cast<int>( std::round( params[0] ) ) );
}

inline double sqrt( std::vector<double> &params )
{
    return std::sqrt( params[0] );
}

inline double log( std::vector<double> &params )
{
    return std::log( params[0] );
}

inline double sin( std::vector<double> &params )
{
    return std::sin( params[0] );
}

inline double cos( std::vector<double> &params )
{
    return std::cos( params[0] );
}

inline double tan( std::vector<double> &params )
{
    return std::tan( params[0] );
}

inline double clamp( std::vector<double> &params )
{
    if( params[2] < params[1] ) {
        debugmsg( "clamp called with hi < lo (%f < %f)", params[2], params[1] );
        return params[0];
    }
    return std::clamp( params[0], params[1], params[2] );
}

inline double floor( std::vector<double> &params )
{
    return std::floor( params[0] );
}

inline double ceil( std::vector<double> &params )
{
    return std::ceil( params[0] );
}

inline double trunc( std::vector<double> &params )
{
    return std::trunc( params[0] );
}

inline double round( std::vector<double> &params )
{
    return std::round( params[0] );
}

constexpr double test_( std::vector<double> &/* params */ )
{
    return 42;
}

constexpr std::array<math_func, 16> functions{
    math_func{ "abs", 1, abs },
    math_func{ "max", -1, max },
    math_func{ "min", -1, min },
    math_func{ "clamp", 3, clamp },
    math_func{ "floor", 1, floor },
    math_func{ "trunc", 1, trunc },
    math_func{ "ceil", 1, ceil },
    math_func{ "round", 1, round },
    math_func{ "rng", 2, math_rng },
    math_func{ "rand", 1, rand },
    math_func{ "sqrt", 1, sqrt },
    math_func{ "log", 1, log },
    math_func{ "sin", 1, sin },
    math_func{ "cos", 1, cos },
    math_func{ "tan", 1, tan },
    math_func{ "_test_", 0, test_ },
};

namespace math_constants
{
# if (defined M_PI && defined M_E)
constexpr double pi_v = M_PI;
constexpr double e_v = M_E;
#else
constexpr double pi_v = 3.14159265358979323846;
constexpr double e_v = 2.7182818284590452354;
#endif
} // namespace math_constants

constexpr std::array<math_const, 3> constants{
    math_const{ "π", math_constants::pi_v },
    math_const{ "pi", math_constants::pi_v },
    math_const{ "e", math_constants::e_v },
};

std::vector<std::string_view> tokenize( std::string_view str, std::string_view separators,
                                        bool include_seps = true );

#endif // CATA_SRC_MATH_PARSER_FUNC_H
