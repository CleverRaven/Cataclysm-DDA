#pragma once
#ifndef CATA_SRC_MATH_PARSER_IMPL_H
#define CATA_SRC_MATH_PARSER_IMPL_H

#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "cata_utility.h"
#include "debug.h"
#include "dialogue_helpers.h"
#include "math_parser_diag.h"
#include "math_parser_func.h"

struct binary_op {
    enum class associativity {
        right = 0,
        left,
    };
    std::string_view symbol;
    int precedence;
    associativity assoc;
    using f_t = double ( * )( double, double );
    f_t f = nullptr;
};
using pbin_op = binary_op const *;
struct unary_op {
    std::string_view symbol;
    binary_op::f_t f;
};
using punary_op = unary_op const *;

constexpr bool operator>( binary_op const &lhs, binary_op const &rhs )
{
    return lhs.precedence > rhs.precedence ||
           ( lhs.precedence == rhs.precedence && lhs.assoc == binary_op::associativity::left );
}
enum class paren {
    left_sq = 0,
    right_sq,
    left,
    right,
};

struct scoped_diag_eval {
    pdiag_func_eval df{};
    char scope = 'g';
};

struct scoped_diag_ass {
    pdiag_func_ass df{};
    char scope = 'g';
};

struct thingie;
struct oper {
    oper( thingie l_, thingie r_, binary_op::f_t op_ );

    double eval( dialogue &d ) const;

    std::shared_ptr<thingie> l, r;
    binary_op::f_t op{};
};
struct func {
    explicit func( std::vector<thingie> &&params_, math_func::f_t f_ );

    double eval( dialogue &d ) const;

    std::vector<thingie> params;
    math_func::f_t f{};
};
struct func_jmath {
    explicit func_jmath( std::vector<thingie> &&params_, jmath_func_id const &id_ );

    double eval( dialogue &d ) const;

    std::vector<thingie> params;
    jmath_func_id id;
};

struct func_diag_eval {
    using eval_f = std::function<double( dialogue & )>;
    explicit func_diag_eval( eval_f &&f_ ) : f( f_ ) {}

    double eval( dialogue &d ) const {
        return f( d );
    }

    eval_f f;
};
struct func_diag_ass {
    using ass_f = std::function<void( dialogue &, double )>;
    explicit func_diag_ass( ass_f &&f_ ) : f( f_ ) {}

    static double eval( dialogue &/* d */ )  {
        debugmsg( "eval() called on assignment function" );
        return 0;
    }

    void assign( dialogue &d, double val ) const {
        f( d, val );
    }

    ass_f f;
};
struct var {
    template<class... Args>
    explicit var( Args &&... args ) : varinfo( std::forward<Args>( args )... ) {}

    double eval( dialogue &d ) const;

    var_info varinfo;
};
struct kwarg {
    kwarg() = default;
    explicit kwarg( std::string_view key_, thingie val_ );
    std::string key;
    std::shared_ptr<thingie> val;
};
struct array {
    explicit array( std::vector<thingie> &&params_ ): params( params_ ) {}
    std::vector<thingie> params;
};
struct ternary {
    ternary() = default;
    explicit ternary( thingie cond_, thingie mhs_, thingie rhs_ );
    std::shared_ptr<thingie> cond;
    std::shared_ptr<thingie> mhs;
    std::shared_ptr<thingie> rhs;

    double eval( dialogue &d ) const;
};
struct thingie {
    thingie() = default;
    template <class U>
    explicit thingie( U u ) : data( std::in_place_type<U>, std::forward<U>( u ) ) {}
    template <class T, class... Args>
    explicit thingie( std::in_place_type_t<T> /*t*/, Args &&...args )
        : data( std::in_place_type<T>, std::forward<Args>( args )... ) {}

    constexpr double eval( dialogue &d ) const;

    using impl_t =
        std::variant<double, std::string, oper, func, func_jmath, func_diag_eval, func_diag_ass, var, kwarg, ternary, array>;
    impl_t data;
};

constexpr double thingie::eval( dialogue &d ) const
{
    return std::visit( overloaded{
        []( double v )
        {
            return v;
        },
        // NOLINTNEXTLINE(cata-use-string_view)
        []( std::string const & v )
        {
            debugmsg( "Unexpected string operand %s", v );
            return 0.0;
        },
        []( kwarg const & v )
        {
            debugmsg( "Unexpected kwarg %s", v.key );
            return 0.0;
        },
        []( array const & /* v */ )
        {
            debugmsg( "Unexpected array" );
            return 0.0;
        },
        [&d]( auto const & v ) -> double
        {
            return v.eval( d );
        },
    },
    data );
}

using op_t =
    std::variant<pbin_op, punary_op, pmath_func, jmath_func_id, scoped_diag_eval, scoped_diag_ass, paren>;

constexpr bool operator>( op_t const &lhs, binary_op const &rhs )
{
    return ( std::holds_alternative<pbin_op>( lhs ) && *std::get<pbin_op>( lhs ) > rhs ) ||
           std::holds_alternative<punary_op>( lhs );
}

constexpr bool operator>( op_t const &lhs, paren const &rhs )
{
    return !std::holds_alternative<paren>( lhs ) || std::less<paren> {}( rhs, std::get<paren>( lhs ) );
}

namespace math_opers
{
constexpr double neg( double /* zero */, double r )
{
    return -r;
}

constexpr double pos( double /* zero */, double r )
{
    return r;
}

constexpr double add( double l, double r )
{
    return l + r;
}

constexpr double sub( double l, double r )
{
    return l - r;
}

constexpr double mul( double l, double r )
{
    return l * r;
}

constexpr double div( double l, double r )
{
    return l / r;
}

inline double mod( double l, double r )
{
    return std::fmod( l, r );
}

inline double math_pow( double l, double r )
{
    return std::pow( l, r );
}

inline double lt( double l, double r )
{
    return static_cast<double>( l < r );
}

inline double lte( double l, double r )
{
    return static_cast<double>( l <= r );
}

inline double gt( double l, double r )
{
    return static_cast<double>( l > r );
}

inline double gte( double l, double r )
{
    return static_cast<double>( l >= r );
}

inline double eq( double l, double r )
{
    return static_cast<double>( l == r );
}

inline double neq( double l, double r )
{
    return static_cast<double>( l != r );
}

inline double b_neg( double /* zero */, double r )
{
    return static_cast<double>( float_equals( r, 0 ) );
}

} // namespace math_opers

constexpr std::array<binary_op, 15> binary_ops{
    binary_op{ "?", 0, binary_op::associativity::right },
    binary_op{ ":", 0, binary_op::associativity::right },
    binary_op{ "<", 1, binary_op::associativity::left, math_opers::lt },
    binary_op{ "<=", 1, binary_op::associativity::left, math_opers::lte },
    binary_op{ ">", 1, binary_op::associativity::left, math_opers::gt },
    binary_op{ ">=", 1, binary_op::associativity::left, math_opers::gte },
    binary_op{ "==", 1, binary_op::associativity::left, math_opers::eq },
    binary_op{ "!=", 1, binary_op::associativity::left, math_opers::neq },
    binary_op{ "+", 2, binary_op::associativity::left, math_opers::add },
    binary_op{ "-", 2, binary_op::associativity::left, math_opers::sub },
    binary_op{ "*", 3, binary_op::associativity::left, math_opers::mul },
    binary_op{ "/", 3, binary_op::associativity::left, math_opers::div },
    binary_op{ "%", 3, binary_op::associativity::right, math_opers::mod },
    binary_op{ "^", 4, binary_op::associativity::right, math_opers::math_pow },
};

constexpr std::array<unary_op, 3> prefix_unary_ops{
    unary_op{ "+", math_opers::pos },
    unary_op{ "-", math_opers::neg },
    unary_op{ "!", math_opers::b_neg },
};

#endif // CATA_SRC_MATH_PARSER_IMPL_H
