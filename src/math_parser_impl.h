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

template<class D>
struct scoped_diag_eval {
    pdiag_func_eval<D> df{};
    char scope = 'g';
};

template<class D>
struct scoped_diag_ass {
    pdiag_func_ass<D> df{};
    char scope = 'g';
};

template<class D>
struct oper;
template<class D>
struct func;
template<class D>
struct func_diag_eval;
template<class D>
struct func_diag_ass;
template<class D>
struct var;
template<class D>
struct thingie {
    thingie() = default;
    template <class U>
    explicit thingie( U u ) : data( std::in_place_type<U>, std::forward<U>( u ) ) {}
    template <class T, class... Args>
    explicit thingie( std::in_place_type_t<T> /*t*/, Args &&...args )
        : data( std::in_place_type<T>, std::forward<Args>( args )... ) {}

    constexpr double eval( D const &d ) const;

    using impl_t =
        std::variant<double, std::string, oper<D>, func<D>, func_diag_eval<D>, func_diag_ass<D>, var<D>>;
    impl_t data;
};

template<class D>
struct var {
    template<class... Args>
    explicit var( Args &&... args ) : varinfo( std::forward<Args>( args )... ) {}

    double eval( D const &d ) const {
        std::string const str = read_var_value( varinfo, d );
        if( str.empty() ) {
            return 0;
        }
        return std::stod( str );
    }

    var_info varinfo;
};

template<class D>
struct func {
    explicit func( std::vector<thingie<D>> &&params_, math_func::f_t f_ ) : params( params_ ),
        f( f_ ) {}

    double eval( D const &d ) const {
        std::vector<double> elems( params.size() );
        std::transform( params.begin(), params.end(), elems.begin(),
        [&d]( thingie<D> const & e ) {
            return e.eval( d );
        } );
        return f( elems );
    }

    std::vector<thingie<D>> params;
    math_func::f_t f{};
};

template<class D>
struct func_diag_eval {
    using eval_f = std::function<double( D const & )>;
    explicit func_diag_eval( eval_f &&f_ ) : f( f_ ) {}

    double eval( D const &d ) const {
        return f( d );
    }

    eval_f f;
};

template<class D>
struct func_diag_ass {
    using ass_f = std::function<void( D const &, double )>;
    explicit func_diag_ass( ass_f &&f_ ) : f( f_ ) {}

    double eval( D const &/* d */ ) const {
        debugmsg( "eval() called on assignment function" );
        return 0;
    }

    void assign( D const &d, double val ) const {
        f( d, val );
    }

    ass_f f;
};

template<class D>
struct oper {
    oper( thingie<D> l_, thingie<D> r_, binary_op::f_t op_ ) :
        l( std::make_shared<thingie<D>>( std::move( l_ ) ) ),
        r( std::make_shared<thingie<D>>( std::move( r_ ) ) ),
        op( op_ ) {}

    constexpr double eval( D const &d ) const {
        return ( *op )( l->eval( d ), r->eval( d ) );
    }

    std::shared_ptr<thingie<D>> l, r;
    binary_op::f_t op{};
};

// overload pattern from https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
explicit overloaded( Ts... ) -> overloaded<Ts...>;
template<class D>
constexpr double thingie<D>::eval( D const &d ) const
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

template<class D>
using op_t =
    std::variant<pbin_op, punary_op, pmath_func, scoped_diag_eval<D>, scoped_diag_ass<D>, paren>;

template<class D>
constexpr bool operator>( op_t<D> const &lhs, binary_op const &rhs )
{
    return ( std::holds_alternative<pbin_op>( lhs ) && *std::get<pbin_op>( lhs ) > rhs ) ||
           std::holds_alternative<punary_op>( lhs );
}

template<class D>
constexpr bool operator>( op_t<D> const &lhs, paren const &rhs )
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
