#ifndef CATA_SRC_MATH_PARSER_IMPL_H
#define CATA_SRC_MATH_PARSER_IMPL_H

#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

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
    f_t f;
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
    left = 0,
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

    double eval( dialogue const &d ) const;

    std::shared_ptr<thingie> l, r;
    binary_op::f_t op{};
};
struct func {
    explicit func( std::vector<thingie> &&params_, math_func::f_t f_ );

    double eval( dialogue const &d ) const;

    std::vector<thingie> params;
    math_func::f_t f{};
};
struct func_diag_eval {
    using eval_f = std::function<double( dialogue const & )>;
    explicit func_diag_eval( eval_f &&f_ ) : f( f_ ) {}

    double eval( dialogue const &d ) const {
        return f( d );
    }

    eval_f f;
};
struct func_diag_ass {
    using ass_f = std::function<void( dialogue const &, double )>;
    explicit func_diag_ass( ass_f &&f_ ) : f( f_ ) {}

    static double eval( dialogue const &/* d */ )  {
        debugmsg( "eval() called on assignment function" );
        return 0;
    }

    void assign( dialogue const &d, double val ) const {
        f( d, val );
    }

    ass_f f;
};
struct var {
    template<class... Args>
    explicit var( Args &&... args ) : varinfo( std::forward<Args>( args )... ) {}

    double eval( dialogue const &d ) const {
        std::string const str = read_var_value( varinfo, d );
        if( str.empty() ) {
            return 0;
        }
        return std::stod( str );
    }

    var_info varinfo;
};
struct thingie {
    thingie() = default;
    template <class U>
    explicit thingie( U u ) : data( std::in_place_type<U>, std::forward<U>( u ) ) {}
    template <class T, class... Args>
    explicit thingie( std::in_place_type_t<T> /*t*/, Args &&...args )
        : data( std::in_place_type<T>, std::forward<Args>( args )... ) {}

    constexpr double eval( dialogue const &d ) const;

    using impl_t =
        std::variant<double, std::string, oper, func, func_diag_eval, func_diag_ass, var>;
    impl_t data;
};

// overload pattern from https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
explicit overloaded( Ts... ) -> overloaded<Ts...>;
constexpr double thingie::eval( dialogue const &d ) const
{
    return std::visit( overloaded{
        []( double v )
        {
            return v;
        },
        []( std::string const & v )
        {
            debugmsg( "Unexpected string operand %.*s", v.size(), v.data() );
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
    std::variant<pbin_op, punary_op, pmath_func, scoped_diag_eval, scoped_diag_ass, paren>;

constexpr bool operator>( op_t const &lhs, binary_op const &rhs )
{
    return ( std::holds_alternative<pbin_op>( lhs ) && *std::get<pbin_op>( lhs ) > rhs ) ||
           std::holds_alternative<punary_op>( lhs );
}

constexpr bool operator>( op_t const &lhs, paren const &rhs )
{
    return !std::holds_alternative<paren>( lhs ) || std::less<paren> {}( rhs, std::get<paren>( lhs ) );
}

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

constexpr std::array<binary_op, 6> binary_ops{
    binary_op{ "+", 2, binary_op::associativity::left, add },
    binary_op{ "-", 2, binary_op::associativity::left, sub },
    binary_op{ "*", 3, binary_op::associativity::left, mul },
    binary_op{ "/", 3, binary_op::associativity::left, div },
    binary_op{ "%", 3, binary_op::associativity::right, mod },
    binary_op{ "^", 4, binary_op::associativity::right, math_pow },
};

constexpr std::array<unary_op, 2> prefix_unary_ops{
    unary_op{ "+", pos },
    unary_op{ "-", neg },
};


#endif // CATA_SRC_MATH_PARSER_IMPL_H
