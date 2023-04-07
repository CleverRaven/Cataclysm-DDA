#include "math_parser.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <optional>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "cata_assert.h"
#include "condition.h"
#include "debug.h"
#include "dialogue.h"
#include "dialogue_helpers.h"
#include "global_vars.h"
#include "math_parser_func.h"
#include "math_parser_impl.h"
#include "mission.h"
#include "string_formatter.h"

namespace
{
template<typename T, class C>
constexpr std::optional<T> _get_common( C const &cnt, std::string_view token )
{
    auto const it =
    std::find_if( cnt.cbegin(), cnt.cend(), [&token]( typename C::value_type const & c ) {
        return c.symbol == token;
    } );
    if( it != cnt.end() ) {
        return { & *it };
    }

    return std::nullopt;
}

constexpr std::optional<double> get_constant( std::string_view token )
{
    if( std::optional<math_const const *> ret = _get_common<pmath_const>( constants, token ); ret ) {
        return ( *ret )->val;
    }
    return std::nullopt;
}

std::optional<double> get_number( std::string_view token )
{
    char *pEnd = nullptr;
    double const val = std::strtod( token.data(), &pEnd );
    if( pEnd == token.data() + token.size() ) {
        return { val };
    }
    errno = 0;

    return get_constant( token );
}

constexpr std::optional<std::string_view> get_string( std::string_view token )
{
    if( token.front() == '\'' && token.back() == '\'' ) {
        return { token.substr( 1, token.size() - 2 ) };
    }

    return std::nullopt;
}

constexpr std::optional<pmath_func> get_function( std::string_view token )
{
    return _get_common<pmath_func>( functions, token );
}

template<class S, typename P, class C>
std::optional<S> _get_dialogue_func( C const &cnt, std::string_view token )
{
    char scope = 'g';
    std::string_view scoped = token;
    if( token.size() > 2 && token[1] == '_' ) {
        scope = token[0];
        scoped = token.substr( 2, token.size() - 2 );
    }
    std::optional<P> df = _get_common<P>( cnt, scoped );
    if( df ) {
        if( ( *df )->scopes.find( scope ) == std::string_view::npos ) {
            throw std::invalid_argument( string_format(
                                             "Scope %c is not valid for dialogue function %s() (%s)",
                                             scope, ( *df )->symbol.data(), ( *df )->scopes.data() ) );
        }
        return { { *df, scope } };
    }

    return std::nullopt;
}

template<class D>
std::optional<scoped_diag_eval<D>> get_dialogue_eval( std::string_view token )
{
    return _get_dialogue_func<scoped_diag_eval<D>, pdiag_func_eval<D>>( dialogue_eval_f<D>, token );
}

template<class D>
std::optional<scoped_diag_ass<D>> get_dialogue_ass( std::string_view token )
{
    return _get_dialogue_func<scoped_diag_ass<D>, pdiag_func_ass<D>>( dialogue_assign_f<D>, token );
}

constexpr std::optional<pbin_op> get_binary_op( std::string_view token )
{
    return _get_common<pbin_op>( binary_ops, token );
}

constexpr std::optional<punary_op> get_unary_op( std::string_view token )
{
    return _get_common<punary_op>( prefix_unary_ops, token );
}

struct parse_state {
    enum class expect : int {
        oper = 0,
        operand,
        lparen,  // operand alias used for functions
        rparen,
        eof,     // oper alias used for EOF
    };
    static constexpr std::string_view expect_to_string( expect e ) {
        switch( e ) {
            // *INDENT-OFF*
            case expect::oper: return "operator";
            case expect::operand: return "operand";
            case expect::lparen: return "left parenthesis";
            case expect::rparen: return "right parenthesis";
            case expect::eof: ;
            // *INDENT-ON*
        }
        return "EOF";
    }

    void validate( expect next ) const {
        if( previous == expect::lparen && next == expect::rparen ) {
            return;
        }
        expect const alias = next == expect::rparen ? expect::oper : next;
        if( expected != alias && ( expected != expect::operand || alias != expect::lparen ) &&
            ( expected != expect::oper || alias != expect::eof ) ) {
            throw std::invalid_argument( string_format(
                                             "Expected %s, got %s",
                                             expect_to_string( expected ).data(),
                                             expect_to_string( next ).data() ) );
        }
    }
    void set( expect current, bool unary_ok ) {
        previous = expected;
        expected = current;
        allows_prefix_unary = unary_ok;
    }

    expect expected;
    expect previous;
    bool allows_prefix_unary;
};

template<class D>
bool is_function( op_t<D> const &op )
{
    return std::holds_alternative<pmath_func>( op ) ||
           std::holds_alternative<scoped_diag_eval<D>>( op ) ||
           std::holds_alternative<scoped_diag_ass<D>>( op );
}

template<class D>
bool is_assign_target( thingie<D> const &thing )
{
    return std::holds_alternative<var<D>>( thing.data ) ||
           std::holds_alternative<func_diag_ass<D>>( thing.data );
}

} // namespace

template<class D>
class math_exp<D>::math_exp_impl
{
    public:
        bool parse( std::string_view str, bool assignment ) {
            if( str.empty() ) {
                return false;
            }
            try {
                _parse( str, assignment );
            } catch( std::invalid_argument const &ex ) {
                std::ptrdiff_t const offset =
                    str.empty()
                    ? 0
                    : std::clamp<std::ptrdiff_t>( last_token.data() - str.data(), 0, 80 );
                debugmsg( "Expression parsing failed: %s\n\n%.80s\n%*s^\n", ex.what(),
                          str.data(), offset, " " );
                ops = {};
                output = {};
                arity = {};
                tree = thingie<D> { 0.0 };
                return false;
            }
            return true;
        }
        double eval( D const &d ) const {
            return tree.eval( d );
        }

        void assign( D const &d, double val ) const {
            std::visit( overloaded{
                [&d, val]( func_diag_ass<D> const & v ) {
                    v.assign( d, val );
                },
                [&d, val]( var<D> const & v ) {
                    write_var_value( v.varinfo.type, v.varinfo.name,
                                     d.actor( v.varinfo.type == var_type::npc ),
                                     std::to_string( val ) );
                },
                []( auto &/* v */ ) {
                    debugmsg( "Assignment called on eval tree" );
                },
            },
            tree.data );
        }

    private:
        std::stack<op_t<D>> ops;
        std::stack<thingie<D>> output;
        struct arity_t {
            std::string_view sym;
            int current{};
            int expected{};
            bool stringy = false;
        };
        std::stack<arity_t> arity;
        thingie<D> tree{ 0.0 };
        std::string_view last_token;

        void _parse( std::string_view str, bool assignment );
        void parse_string( std::string_view str, parse_state &state );
        void parse_bin_op( pbin_op const &op, parse_state &state );
        template<typename T>
        void parse_diag_f( T const &token, parse_state &state );
        void parse_comma( parse_state &state );
        void parse_lparen( parse_state &state );
        void parse_rparen( parse_state &state );
        void new_func();
        void new_oper();
        void new_var( std::string_view str );
        void maybe_first_argument();
        std::vector<std::string> _get_strings( std::vector<thingie<D>> const &params,
                                               size_t nparams ) const;
};

template<class D>
void math_exp<D>::math_exp_impl::maybe_first_argument()
{
    if( !arity.empty() && !arity.top().sym.empty() && arity.top().current == 0 ) {
        arity.top().current++;
    }
}

template<class D>
void math_exp<D>::math_exp_impl::_parse( std::string_view str, bool assignment )
{
    constexpr std::string_view expression_separators = "+-*/^,()%";
    parse_state state{ parse_state::expect::operand, parse_state::expect::eof, true };
    for( std::string_view const token : tokenize( str, expression_separators ) ) {
        last_token = token;
        if( std::optional<std::string_view> str = get_string( token ); str ) {
            parse_string( *str, state );

        } else if( std::optional<double> val = get_number( token ); val ) {
            state.validate( parse_state::expect::operand );
            maybe_first_argument();
            output.emplace( *val );
            state.set( parse_state::expect::oper, false );

        } else if( std::optional<pmath_func> ftoken = get_function( token ); ftoken ) {
            state.validate( parse_state::expect::operand );
            maybe_first_argument();
            ops.emplace( *ftoken );
            arity.emplace( arity_t{ ( *ftoken )->symbol, 0, ( *ftoken )->num_params } );
            state.set( parse_state::expect::lparen, false );

        } else if( std::optional<scoped_diag_eval<D>> feval = get_dialogue_eval<D>( token ); feval &&
                   !assignment ) {
            parse_diag_f( *feval, state );

        } else if( std::optional<scoped_diag_ass<D>> fass = get_dialogue_ass<D>( token ); fass &&
                   assignment ) {
            parse_diag_f( *fass, state );

        } else if( std::optional<punary_op> op = get_unary_op( token ); op && state.allows_prefix_unary ) {
            state.validate( parse_state::expect::operand );
            ops.emplace( *op );
            state.set( parse_state::expect::operand, false );

        } else if( std::optional<pbin_op> op = get_binary_op( token ); op ) {
            parse_bin_op( *op, state );

        } else if( token == "," ) {
            parse_comma( state );

        } else if( token == "(" ) {
            parse_lparen( state );

        } else if( token == ")" ) {
            parse_rparen( state );

        } else {
            state.validate( parse_state::expect::operand );
            maybe_first_argument();
            new_var( token );
            state.set( parse_state::expect::oper, false );
        }
    }
    state.validate( parse_state::expect::eof );
    while( !ops.empty() ) {
        if( std::holds_alternative<paren>( ops.top() ) && std::get<paren>( ops.top() ) == paren::left ) {
            throw std::invalid_argument( "Unterminated left paranthesis" );
        }
        new_oper();
    }

    if( assignment && !is_assign_target( output.top() ) ) {
        throw std::invalid_argument( "Assignment tree can only contain one variable name or dialogue assignment function" );
    }

    if( output.size() != 1 ) {
        throw std::invalid_argument( "Invalid expression.  That's all we know.  Blame andrei." );
    }
    tree = std::move( output.top() );
    output.pop();
}

template<class D>
void math_exp<D>::math_exp_impl::parse_string( std::string_view str, parse_state &state )
{
    state.validate( parse_state::expect::operand );
    maybe_first_argument();
    if( arity.empty() || !arity.top().stringy ) {
        throw std::invalid_argument( "String arguments can only be used in dialogue functions" );
    }
    output.emplace( std::string{ str } );
    state.set( parse_state::expect::oper, false );
}

template<class D>
void math_exp<D>::math_exp_impl::parse_bin_op( pbin_op const &op, parse_state &state )
{
    state.validate( parse_state::expect::oper );
    while( !ops.empty() && ops.top() > *op ) {
        new_oper();
    }
    ops.emplace( op );
    state.set( parse_state::expect::operand, true );
}

template<class D>
template<typename T>
void math_exp<D>::math_exp_impl::parse_diag_f( T const &token, parse_state &state )
{
    state.validate( parse_state::expect::operand );
    ops.emplace( token );
    arity.emplace( arity_t{ token.df->symbol, 0, token.df->num_params, true } );
    state.set( parse_state::expect::lparen, false );
}

template<class D>
void math_exp<D>::math_exp_impl::parse_comma( parse_state &state )
{
    state.validate( parse_state::expect::oper );
    if( arity.empty() || arity.top().sym.empty() ) {
        throw std::invalid_argument( "Misplaced comma" );
    }
    while( ops.top() > paren::left ) {
        new_oper();
    }
    arity.top().current++;
    if( arity.top().expected > 0 && arity.top().current > arity.top().expected ) {
        throw std::invalid_argument( string_format(
                                         "Too many arguments for function %s()",
                                         arity.top().sym.data() ) );
    }
    state.set( parse_state::expect::operand, true );
}

template<class D>
void math_exp<D>::math_exp_impl::parse_lparen( parse_state &state )
{
    state.validate( parse_state::expect::lparen );
    if( ops.empty() || !is_function( ops.top() ) ) {
        arity.emplace( arity_t{ {}, 0, 0 } );
    }
    ops.emplace( paren::left );
    state.set( parse_state::expect::operand, true );
}

template<class D>
void math_exp<D>::math_exp_impl::parse_rparen( parse_state &state )
{
    state.validate( parse_state::expect::rparen );
    while( !ops.empty() && ops.top() > paren::left ) {
        new_oper();
    }
    if( ops.empty() || !std::holds_alternative<paren>( ops.top() ) ||
        std::get<paren>( ops.top() ) != paren::left ) {
        throw std::invalid_argument( "Misplaced right parenthesis" );
    }
    ops.pop();
    new_func();
    arity.pop();
    maybe_first_argument();
    state.set( parse_state::expect::oper, false );
}

template<class D>
void math_exp<D>::math_exp_impl::new_func()
{
    if( !ops.empty() && is_function( ops.top() ) ) {
        typename std::vector<thingie<D>>::size_type const nparams = arity.top().current;
        if( arity.top().expected > 0 && arity.top().current < arity.top().expected ) {
            throw std::invalid_argument( string_format(
                                             "Not enough arguments for function %s()",
                                             arity.top().sym.data() ) );
        }
        std::vector<thingie<D>> params( nparams );
        for( typename std::vector<thingie<D>>::size_type i = 0; i < nparams; i++ ) {
            params[i] = std::move( output.top() );
            output.pop();
        }
        std::visit( overloaded{
            [&params, nparams, this]( scoped_diag_eval<D> const & v )
            {
                std::vector<std::string> const strings = _get_strings( params, nparams );
                output.emplace( std::in_place_type_t<func_diag_eval<D>>(), v.df->f( v.scope, strings ) );
            },
            [&params, nparams, this]( scoped_diag_ass<D> const & v )
            {
                std::vector<std::string> const strings = _get_strings( params, nparams );
                output.emplace( std::in_place_type_t<func_diag_ass<D>>(), v.df->f( v.scope, strings ) );
            },
            [&params, this]( pmath_func v )
            {
                output.emplace( std::in_place_type_t<func<D>>(), std::move( params ), v->f );
            },
            []( auto /* v */ )
            {
                throw std::invalid_argument( "Internal error.  That's all we know." );
            },
        },
        ops.top() );
        ops.pop();
    }
}

template<class D>
std::vector<std::string> math_exp<D>::math_exp_impl::_get_strings( std::vector<thingie<D>> const
        &params, size_t nparams ) const
{
    std::vector<std::string> strings( nparams );
    std::transform( params.begin(), params.end(), strings.begin(), [this]( thingie<D> const & e ) {
        if( std::holds_alternative<std::string>( e.data ) ) {
            return std::get<std::string>( e.data );
        }
        throw std::invalid_argument( string_format(
                                         "Parameters for %s() must be strings contained in single quotes",
                                         arity.top().sym.data() ) );
        return std::string{};
    } );
    return strings;
}

template<class D>
void math_exp<D>::math_exp_impl::new_oper()
{
    std::visit( overloaded{
        [this]( pbin_op v )
        {
            cata_assert( output.size() >= 2 );
            thingie rhs = std::move( output.top() );
            output.pop();
            thingie lhs = std::move( output.top() );
            output.pop();
            output.emplace( std::in_place_type_t<oper<D>>(), lhs, rhs, v->f );
        },
        [this]( punary_op v )
        {
            cata_assert( !output.empty() );
            thingie rhs = std::move( output.top() );
            output.pop();
            output.emplace( std::in_place_type_t<oper<D>>(), thingie<D> { 0.0 }, rhs, v->f );
        },
        []( pmath_func v )
        {
            throw std::invalid_argument( string_format(
                                             "Unterminated math function %s()",
                                             v->symbol.data() ) );
        },
        []( scoped_diag_eval<D> const & v )
        {
            throw std::invalid_argument( string_format(
                                             "Unterminated dialogue function %s()",
                                             v.df->symbol.data() ) );
        },
        []( scoped_diag_ass<D> const & v )
        {
            throw std::invalid_argument( string_format(
                                             "Unterminated dialogue function %s()",
                                             v.df->symbol.data() ) );
        },
        []( paren /* v */ ) {}
    },
    ops.top() );

    ops.pop();
}

template<class D>
void math_exp<D>::math_exp_impl::new_var( std::string_view str )
{
    std::string_view scoped = str;
    var_type type = var_type::global;
    if( str.size() > 2 && str[1] == '_' ) {
        scoped = scoped.substr( 2 );
        switch( str[0] ) {
            case 'u':
                type = var_type::u;
                break;
            case 'n':
                type = var_type::npc;
                break;
            default:
                debugmsg( "Unknown scope %c in variable %.*s", str[0], str.size(), str.data() );
        }
    }
    output.emplace( std::in_place_type_t<var<D>>(), type, "npctalk_var_" + std::string{ scoped } );
}

template<class D>
bool math_exp<D>::parse( std::string_view str, bool assignment )
{
    impl = std::make_unique<math_exp_impl>();
    return impl->parse( str, assignment );
}

template<class D>
math_exp<D>::math_exp( math_exp<D> const &other ) :
    impl( other.impl ? std::make_unique<math_exp_impl>( *other.impl ) :
          std::make_unique<math_exp_impl>() ) {}
template<class D>
math_exp<D> &math_exp<D>::operator=( math_exp<D> const &other )
{
    impl = other.impl ? std::make_unique<math_exp_impl>( *other.impl ) :
           std::make_unique<math_exp_impl>();
    return *this;
}
template<class D>
math_exp<D>::math_exp() = default;
template<class D>
math_exp<D>::~math_exp() = default;
template<class D>
math_exp<D>::math_exp( math_exp<D> &&/* other */ ) noexcept = default;
template<class D>
math_exp<D> &math_exp<D>::operator=( math_exp<D> &&/* other */ )  noexcept = default;

template<class D>
double math_exp<D>::eval( D const &d ) const
{
    return impl->eval( d );
}

template<class D>
void math_exp<D>::assign( D const &d, double val ) const
{
    return impl->assign( d, val );
}

template class math_exp<dialogue>;
template class math_exp<mission_goal_condition_context>;
