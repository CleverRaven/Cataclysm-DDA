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
#include "dialogue.h"
#include "dialogue_helpers.h"
#include "math_parser_diag.h"
#include "math_parser_func.h"
#include "math_parser_type.h"
#include "type_id.h"

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
    math_type_t type = math_type_t::ret;
};
using pbin_op = binary_op const *;
struct unary_op {
    std::string_view symbol;
    binary_op::f_t f;
};
using punary_op = unary_op const *;

struct ass_op {
    std::string_view symbol;
    bool needs_mhs;
    binary_op::f_t f;
    bool unaryone = false; // FIXME: hack...
};
using pass_op = ass_op const *;

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

struct scoped_diag_proto {
    std::string_view token;
    pdiag_func df{};
    char scope = 'g';
};

struct thingie;
struct oper {
    oper( thingie l_, thingie r_, binary_op::f_t op_ );

    double eval( const_dialogue const &d ) const;

    std::shared_ptr<thingie> l, r;
    binary_op::f_t op{};
};

struct dot_oper {
    dot_oper( var_info v_, std::string_view m_ );

    double eval( const_dialogue const &d ) const;
    void assign( dialogue &d, double val ) const;

    var_info v;
    char member{};
};
struct func {
    explicit func( std::vector<thingie> &&params_, math_func::f_t f_ );

    double eval( const_dialogue const &d ) const;

    std::vector<thingie> params;
    math_func::f_t f{};
};
struct func_jmath {
    explicit func_jmath( std::vector<thingie> &&params_, jmath_func_id const &id_ );

    double eval( const_dialogue const &d ) const;

    std::vector<thingie> params;
    jmath_func_id id;
};

struct func_diag {
    using eval_f = dialogue_func::fe_t;
    using ass_f = dialogue_func::fa_t;
    explicit func_diag( eval_f const &fe_, ass_f const &fa_, char s, std::vector<thingie> p,
                        std::map<std::string, thingie> k );

    double eval( const_dialogue const &d ) const;

    void assign( dialogue &d, double val ) const;

    eval_f fe;
    ass_f fa;
    char scope;
    mutable std::vector<diag_value> params;
    mutable diag_kwargs kwargs;

    std::vector<thingie> params_dyn;
    std::map<std::string, thingie> kwargs_dyn;

    template<bool at_runtime>
    void _update_diag_args( const_dialogue const *d = nullptr ) const;
    template<bool at_runtime>
    void _update_diag_kwargs( const_dialogue const *d = nullptr ) const;
};

struct var {
    template<class... Args>
    explicit var( Args &&... args ) : varinfo( std::forward<Args>( args )... ) {}

    double eval( const_dialogue const &d ) const;
    void assign( dialogue &d, double val ) const;

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

    double eval( const_dialogue const &d ) const;
};

struct ass_oper {
    ass_oper() = default;
    explicit ass_oper( thingie lhs_, thingie mhs_, thingie rhs_, binary_op::f_t op_ );
    std::shared_ptr<thingie> lhs;
    std::shared_ptr<thingie> mhs;
    std::shared_ptr<thingie> rhs;
    binary_op::f_t op{};

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
    constexpr double eval( const_dialogue const &d ) const;

    using impl_t =
        std::variant<double, std::string, oper, dot_oper, ass_oper, func, func_jmath, func_diag, var, kwarg, ternary, array>;
    impl_t data;
};

template <typename V>
using f_eval_t = decltype( std::declval<V>().eval( std::declval<const_dialogue const &>() ) );
template <typename V>
using f_assign_t =
    decltype( std::declval<V>().assign( std::declval<dialogue &>(), std::declval<double>() ) );

template <typename V, template <typename> class F, typename = void>
constexpr bool v_has = false;

template <typename V, template <typename> class F>
constexpr bool v_has<V, F, std::void_t<F<V>>> = true;

template <typename T>
constexpr bool v_has_eval = v_has<T, f_eval_t>;
template <typename T>
constexpr bool v_has_assign = v_has<T, f_assign_t>;

constexpr double thingie::eval( const_dialogue const &d ) const
{
    return std::visit( overloaded{
        []( double v )
        {
            return v;
        },
        []( ass_oper const & /* v */ ) -> double
        {
            throw math::runtime_error( "Cannot use assignment operators from eval context" );
            return 0.0;
        },
        [&d]( auto const & v ) -> double
        {
            if constexpr( v_has_eval<decltype( v )> )
            {
                return v.eval( d );
            } else
            {
                throw math::internal_error( "math called eval() on unexpected node without eval()" );
                return 0.0;
            }
        },
    },
    data );
}

constexpr double thingie::eval( dialogue &d ) const
{
    if( std::holds_alternative<ass_oper>( data ) ) {
        return std::get<ass_oper>( data ).eval( d );
    }
    return eval( static_cast<const_dialogue const &>( d ) );
}

using op_t =
    std::variant<pbin_op, punary_op, pass_op, pmath_func, jmath_func_id, scoped_diag_proto, paren>;

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

constexpr binary_op assignment_op{ "", -10, binary_op::associativity::left };

constexpr std::array<binary_op, 15> binary_ops{
    binary_op{ "?", 0, binary_op::associativity::right },
    binary_op{ ":", 0, binary_op::associativity::right },
    binary_op{ "<", 1, binary_op::associativity::left, math_opers::lt },
    binary_op{ "<=", 1, binary_op::associativity::left, math_opers::lte, math_type_t::compare },
    binary_op{ ">", 1, binary_op::associativity::left, math_opers::gt, math_type_t::compare },
    binary_op{ ">=", 1, binary_op::associativity::left, math_opers::gte, math_type_t::compare },
    binary_op{ "==", 1, binary_op::associativity::left, math_opers::eq, math_type_t::compare },
    binary_op{ "!=", 1, binary_op::associativity::left, math_opers::neq, math_type_t::compare },
    binary_op{ "+", 2, binary_op::associativity::left, math_opers::add },
    binary_op{ "-", 2, binary_op::associativity::left, math_opers::sub },
    binary_op{ "*", 3, binary_op::associativity::left, math_opers::mul },
    binary_op{ "/", 3, binary_op::associativity::left, math_opers::div },
    binary_op{ "%", 3, binary_op::associativity::right, math_opers::mod },
    binary_op{ "^", 4, binary_op::associativity::right, math_opers::math_pow },
    binary_op{ ".", 10, binary_op::associativity::right },
};

constexpr std::array<unary_op, 3> prefix_unary_ops{
    unary_op{ "+", math_opers::pos },
    unary_op{ "-", math_opers::neg },
    unary_op{ "!", math_opers::b_neg },
};

constexpr std::array<ass_op, 8> ass_ops{
    ass_op{ "=", false, math_opers::add },
    ass_op{ "+=", true, math_opers::add },
    ass_op{ "-=", true, math_opers::sub },
    ass_op{ "*=", true, math_opers::mul },
    ass_op{ "/=", true, math_opers::div },
    ass_op{ "%=", true, math_opers::mod },
    ass_op{ "++", true, math_opers::add, true },
    ass_op{ "--", true, math_opers::sub, true },
};

#endif // CATA_SRC_MATH_PARSER_IMPL_H
