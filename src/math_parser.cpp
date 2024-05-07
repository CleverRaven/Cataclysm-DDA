#include "math_parser.h"

#include <algorithm>
#include <cstddef>
#include <locale>
#include <map>
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
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "condition.h"
#include "debug.h"
#include "dialogue.h"
#include "dialogue_helpers.h"
#include "global_vars.h"
#include "math_parser_diag_value.h"
#include "math_parser_func.h"
#include "math_parser_impl.h"
#include "math_parser_jmath.h"
#include "string_formatter.h"
#include "type_id.h"

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
    if( std::optional<double> ret = svtod( token ); ret ) {
        return *ret;
    }

    return get_constant( token );
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
    auto df = cnt.find( scoped );
    if( df != cnt.end() ) {
        if( df->second.scopes.find( scope ) == std::string_view::npos ) {
            throw std::invalid_argument( string_format(
                                             "Scope %c is not valid for dialogue function %s() (valid scopes: %s)",
                                             scope, scoped, df->second.scopes ) );
        }
        return { { &df->second, scope } };
    }

    return std::nullopt;
}

std::optional<scoped_diag_eval> get_dialogue_eval( std::string_view token )
{
    return _get_dialogue_func<scoped_diag_eval, pdiag_func_eval>( get_all_diag_eval_funcs(), token );
}

std::optional<scoped_diag_ass> get_dialogue_ass( std::string_view token )
{
    return _get_dialogue_func<scoped_diag_ass, pdiag_func_ass>( get_all_diag_ass_funcs(), token );
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
        lbracket,
        rbracket,
        string,
        eof,     // oper alias used for EOF
    };
    static constexpr std::string_view expect_to_string( expect e ) {
        switch( e ) {
            // *INDENT-OFF*
            case expect::oper: return "operator";
            case expect::operand: return "operand";
            case expect::lparen: return "left parenthesis";
            case expect::rparen: return "right parenthesis";
            case expect::lbracket: return "left bracket";
            case expect::rbracket: return "right bracket";
            case expect::string: return "string delimiter";
            case expect::eof: return "EOF";
            // *INDENT-ON*
        }
        return "cookies";
    }

    void validate( expect next ) const {
        if( ( previous == expect::lparen && next == expect::rparen ) ||
            ( previous == expect::operand && next == expect::rbracket ) ) {
            return;
        }
        expect const alias =
            next == expect::rparen || next == expect::rbracket ? expect::oper : next;
        if( expected != alias &&
            ( expected != expect::operand ||
              ( alias != expect::lparen && alias != expect::lbracket ) ) &&
            ( expected != expect::oper || alias != expect::eof ) ) {
            throw std::invalid_argument( string_format(
                                             "Expected %s, got %s",
                                             expect_to_string( expected ),
                                             expect_to_string( next ) ) );
        }
    }
    void set( expect current, bool unary_ok = false ) {
        previous = expected;
        expected = current;
        allows_prefix_unary = unary_ok;
    }

    expect expected = expect::operand;
    expect previous = expect::eof;
    bool allows_prefix_unary = true;
    bool instring = false;
    std::string_view strpos;
};

bool is_function( op_t const &op )
{
    return std::holds_alternative<pmath_func>( op ) ||
           std::holds_alternative<jmath_func_id>( op ) ||
           std::holds_alternative<scoped_diag_eval>( op ) ||
           std::holds_alternative<scoped_diag_ass>( op );
}

bool is_assign_target( thingie const &thing )
{
    return std::holds_alternative<var>( thing.data ) ||
           std::holds_alternative<func_diag_ass>( thing.data );
}

std::vector<double> _eval_params( std::vector<thingie> const &params, dialogue &d )
{
    std::vector<double> elems( params.size() );
    std::transform( params.begin(), params.end(), elems.begin(),
    [&d]( thingie const & e ) {
        return e.eval( d );
    } );
    return elems;
}

constexpr void _validate_operand( thingie const &thing, std::string_view symbol )
{
    if( std::holds_alternative<std::string>( thing.data ) ) {
        throw std::invalid_argument( string_format(
                                         R"(Operator "%s" does not support string operands)", symbol ) );
    }
    if( std::holds_alternative<array>( thing.data ) ) {
        throw std::invalid_argument( string_format(
                                         R"(Operator "%s" does not support array operands)", symbol ) );
    }
}

void _validate_unused_kwargs( diag_kwargs const &kwargs )
{
    for( diag_kwargs::value_type const &v : kwargs ) {
        if( !v.second.was_used() ) {
            throw std::invalid_argument( string_format( R"(Unused kwarg "%s")", v.first ) );
        }
    }
}

} // namespace

func::func( std::vector<thingie> &&params_, math_func::f_t f_ ) : params( params_ ),
    f( f_ ) {}
func_jmath::func_jmath( std::vector<thingie> &&params_,
                        jmath_func_id const &id_ ) : params( params_ ),
    id( id_ ) {}

double func::eval( dialogue &d ) const
{
    return f( _eval_params( params, d ) );
}

double func_jmath::eval( dialogue &d ) const
{
    return id->eval( d, _eval_params( params, d ) );
}

double var::eval( dialogue &d ) const
{
    std::string const str = read_var_value( varinfo, d );
    if( str.empty() ) {
        return 0;
    }
    if( std::optional<double> ret = svtod( str ); ret ) {
        return *ret;
    }
    debugmsg( R"(failed to convert variable "%s" with value "%s" to a number)", varinfo.name, str );
    return 0;
}

oper::oper( thingie l_, thingie r_, binary_op::f_t op_ ):
    l( std::make_shared<thingie>( std::move( l_ ) ) ),
    r( std::make_shared<thingie>( std::move( r_ ) ) ),
    op( op_ ) {}

double oper::eval( dialogue &d ) const
{
    return ( *op )( l->eval( d ), r->eval( d ) );
}

kwarg::kwarg( std::string_view key_, thingie val_ )
    : key( key_ ),
      val( std::make_shared<thingie>( std::move( val_ ) ) ) {}

ternary::ternary( thingie cond_, thingie mhs_, thingie rhs_ )
    : cond( std::make_shared<thingie>( std::move( cond_ ) ) ),
      mhs( std::make_shared<thingie>( std::move( mhs_ ) ) ),
      rhs( std::make_shared<thingie>( std::move( rhs_ ) ) ) {}

double ternary::eval( dialogue &d ) const
{
    return cond->eval( d ) > 0 ? mhs->eval( d ) : rhs->eval( d );
}

class math_exp::math_exp_impl
{
    public:
        math_exp_impl() = default;
        explicit math_exp_impl( thingie &&t ): tree( t ) {}

        bool parse( std::string_view str, bool assignment ) {
            if( str.empty() ) {
                return false;
            }
            std::locale const &oldloc = std::locale::global( std::locale::classic() );
            on_out_of_scope reset_loc( [&oldloc]() {
                std::locale::global( oldloc );
            } );
            try {
                _parse( str, assignment );
            } catch( std::invalid_argument const &ex ) {
                error( str, ex.what() );
                ops = {};
                output = {};
                arity = {};
                tree = thingie { 0.0 };
                return false;
            }
            return true;
        }
        double eval( dialogue &d ) const {
            return tree.eval( d );
        }

        void assign( dialogue &d, double val ) const {
            std::visit( overloaded{
                [&d, val]( func_diag_ass const & v ) {
                    v.assign( d, val );
                },
                [&d, val]( var const & v ) {
                    write_var_value( v.varinfo.type, v.varinfo.name,
                                     &d, val );
                },
                []( auto &/* v */ ) {
                    debugmsg( "Assignment called on eval tree" );
                },
            },
            tree.data );
        }

    private:
        std::stack<op_t> ops;
        std::stack<thingie> output;
        struct arity_t {
            enum class type_t {
                func = 0,
                parens,
                array,
                ternary,
            };
            arity_t( std::string_view sym_, int expected_, type_t type_, bool stringy_ = false ) : sym( sym_ ),
                expected( expected_ ), type( type_ ), stringy( stringy_ ) {}
            std::string_view sym;
            int current{};
            size_t nkwargs{};
            int expected{};
            type_t type{};
            bool stringy{};

            bool allows_comma() const {
                return type == type_t::func || type == type_t::array;
            }
        };
        std::stack<arity_t> arity;
        thingie tree{ 0.0 };
        std::string_view last_token;
        parse_state state;

        void _parse( std::string_view str, bool assignment );
        void parse_string( std::string_view token, std::string_view full );
        void parse_bin_op( pbin_op const &op );
        template<typename T>
        void parse_diag_f( std::string_view symbol, T const &token );
        void parse_comma();
        void parse_lparen( arity_t::type_t type = arity_t::type_t::parens );
        void parse_rparen();
        void parse_lbracket();
        void parse_rbracket();
        void new_func();
        void new_oper();
        void new_var( std::string_view str );
        void new_kwarg( thingie &lhs, thingie &rhs );
        void new_ternary( thingie &lhs, thingie &rhs );
        void new_array();
        void maybe_first_argument();
        void error( std::string_view str, std::string_view what );
        void validate_string( std::string_view str, std::string_view label, std::string_view badlist );
        static std::vector<diag_value> _get_diag_vals( std::vector<thingie> &params );
        static diag_value _get_diag_value( thingie &param );
};

void math_exp::math_exp_impl::maybe_first_argument()
{
    if( !arity.empty() && arity.top().allows_comma() && arity.top().current == 0 ) {
        arity.top().current++;
    }
}

void math_exp::math_exp_impl::_parse( std::string_view str, bool assignment )
{
    constexpr std::string_view expression_separators = "+-*/^,()[]%':><=!?";
    state = {};
    for( std::string_view const token : tokenize( str, expression_separators ) ) {
        last_token = token;
        if( state.instring || token == "'" ) {
            parse_string( token, str );

        } else if( std::optional<double> val = get_number( token ); val ) {
            state.validate( parse_state::expect::operand );
            maybe_first_argument();
            output.emplace( *val );
            state.set( parse_state::expect::oper );

        } else if( std::optional<pmath_func> ftoken = get_function( token ); ftoken ) {
            state.validate( parse_state::expect::operand );
            maybe_first_argument();
            ops.emplace( *ftoken );
            arity.emplace( ( *ftoken )->symbol, ( *ftoken )->num_params, arity_t::type_t::func );
            state.set( parse_state::expect::lparen );

        } else if( jmath_func_id jmfid( token ); jmfid.is_valid() ) {
            state.validate( parse_state::expect::operand );
            maybe_first_argument();
            ops.emplace( jmfid );
            arity.emplace( token, jmfid->num_params, arity_t::type_t::func );
            state.set( parse_state::expect::lparen );

        } else if( std::optional<scoped_diag_eval> feval = get_dialogue_eval( token ); feval &&
                   !assignment ) {
            parse_diag_f( token, *feval );

        } else if( std::optional<scoped_diag_ass> fass = get_dialogue_ass( token ); fass &&
                   assignment ) {
            parse_diag_f( token, *fass );

        } else if( std::optional<punary_op> op = get_unary_op( token ); op && state.allows_prefix_unary ) {
            state.validate( parse_state::expect::operand );
            ops.emplace( *op );
            state.set( parse_state::expect::operand );

        } else if( std::optional<pbin_op> op = get_binary_op( token ); op ) {
            parse_bin_op( *op );

        } else if( token == "," ) {
            parse_comma();

        } else if( token == "(" ) {
            parse_lparen();

        } else if( token == ")" ) {
            parse_rparen();

        } else if( token == "[" ) {
            parse_lbracket();

        } else if( token == "]" ) {
            parse_rbracket();

        } else if( token == "=" ) {
            throw std::invalid_argument( R"(Misplaced "=".  Did you mean "=="?)" );

        } else {
            state.validate( parse_state::expect::operand );
            maybe_first_argument();
            new_var( token );
            state.set( parse_state::expect::oper );
        }
    }
    state.validate( parse_state::expect::eof );
    while( !ops.empty() ) {
        if( std::holds_alternative<paren>( ops.top() ) && std::get<paren>( ops.top() ) == paren::left ) {
            throw std::invalid_argument( "Unterminated left paranthesis" );
        }
        if( std::holds_alternative<paren>( ops.top() ) && std::get<paren>( ops.top() ) == paren::left_sq ) {
            throw std::invalid_argument( "Unterminated left bracket" );
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

void math_exp::math_exp_impl::parse_string( std::string_view token, std::string_view full )
{
    if( !state.instring ) {
        state.validate( parse_state::expect::operand );
        maybe_first_argument();
        if( arity.empty() || !arity.top().stringy ) {
            throw std::invalid_argument( "String arguments can only be used in dialogue functions" );
        }
        state.instring = true;
        state.strpos = token;
        state.set( parse_state::expect::string, false );
    } else if( token == "'" ) {
        // FIXME: write a better tokenizer that returns the entire quoted string as a single token
        output.emplace( std::in_place_type_t<std::string>(), full, state.strpos.data() - full.data() + 1,
                        token.data() - state.strpos.data() - 1 );
        state.instring = false;
        state.set( parse_state::expect::oper, false );
    } else {
        // skip tokens inside string
    }
}

void math_exp::math_exp_impl::parse_bin_op( pbin_op const &op )
{
    if( op->symbol == ":" && !arity.empty() && arity.top().type == arity_t::type_t::ternary ) {
        // insert a pair of parens to help resolve nested ternaries like 0?0?-1:-2:1
        parse_rparen();
    }
    state.validate( parse_state::expect::oper );
    while( !ops.empty() && ops.top() > *op ) {
        new_oper();
    }
    ops.emplace( op );
    state.set( parse_state::expect::operand, true );
    if( op->symbol == "?" ) {
        parse_lparen( arity_t::type_t::ternary );
    }
}

template<typename T>
void math_exp::math_exp_impl::parse_diag_f( std::string_view symbol, T const &token )
{
    state.validate( parse_state::expect::operand );
    ops.emplace( token );
    arity.emplace( symbol, token.df->num_params, arity_t::type_t::func, true );
    state.set( parse_state::expect::lparen, false );
}

void math_exp::math_exp_impl::parse_comma()
{
    state.validate( parse_state::expect::oper );
    if( arity.empty() || !arity.top().allows_comma() ) {
        throw std::invalid_argument( "Misplaced comma" );
    }
    while( ops.top() > paren::left ) {
        new_oper();
    }
    arity.top().current++;
    state.set( parse_state::expect::operand, true );
}

void math_exp::math_exp_impl::parse_lparen( arity_t::type_t type )
{
    state.validate( parse_state::expect::lparen );
    if( ops.empty() || !is_function( ops.top() ) ) {
        arity.emplace( std::string_view{}, 0, type, !arity.empty() && arity.top().stringy );
    }
    ops.emplace( paren::left );
    state.set( parse_state::expect::operand, true );
}

void math_exp::math_exp_impl::parse_rparen()
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

void math_exp::math_exp_impl::parse_lbracket()
{
    state.validate( parse_state::expect::lbracket );
    if( arity.empty() || !arity.top().stringy ) {
        throw std::invalid_argument( "Arrays can only be used as arguments for dialogue functions" );
    }
    arity.emplace( std::string_view{}, 0, arity_t::type_t::array,
                   !arity.empty() && arity.top().stringy );
    ops.emplace( paren::left_sq );
    state.set( parse_state::expect::operand, true );
}

void math_exp::math_exp_impl::parse_rbracket()
{
    state.validate( parse_state::expect::rbracket );
    if( arity.empty() || arity.top().type != arity_t::type_t::array ) {
        throw std::invalid_argument( "Misplaced right bracket" );
    }
    while( !ops.empty() && ops.top() > paren::left_sq ) {
        new_oper();
    }
    ops.pop();
    new_array();
    arity.pop();
    maybe_first_argument();
    state.set( parse_state::expect::oper );
}

void math_exp::math_exp_impl::new_func()
{
    if( !ops.empty() && is_function( ops.top() ) ) {
        std::vector<thingie>::size_type const nparams = arity.top().current;
        if( arity.top().expected >= 0 ) {
            if( arity.top().current < arity.top().expected ) {
                throw std::invalid_argument(
                    string_format( "Not enough arguments for function %s()", arity.top().sym ) );
            }
            if( arity.top().current > arity.top().expected ) {
                throw std::invalid_argument(
                    string_format( "Too many arguments for function %s()", arity.top().sym ) );
            }
        }

        std::vector<thingie> params( nparams );
        diag_kwargs kwargs;
        for( std::vector<kwarg>::size_type i = 0; i < arity.top().nkwargs; i++ ) {
            if( !std::holds_alternative<kwarg>( output.top().data ) ) {
                throw std::invalid_argument(
                    "All positional arguments must precede keyword-value pairs" );
            }
            kwarg &kw = std::get<kwarg>( output.top().data );
            kwargs.emplace( kw.key, _get_diag_value( *kw.val ) );
            output.pop();
        }
        for( std::vector<thingie>::size_type i = 0; i < nparams; i++ ) {
            params[nparams - i - 1] = std::move( output.top() );
            output.pop();
        }
        std::visit( overloaded{
            [&params, &kwargs, this]( scoped_diag_eval const & v )
            {
                std::vector<diag_value> const args = _get_diag_vals( params );
                output.emplace( std::in_place_type_t<func_diag_eval>(), v.df->f( v.scope, args, kwargs ) );
            },
            [&params, &kwargs, this]( scoped_diag_ass const & v )
            {
                std::vector<diag_value> const args = _get_diag_vals( params );
                output.emplace( std::in_place_type_t<func_diag_ass>(), v.df->f( v.scope, args, kwargs ) );
            },
            [&params, this]( pmath_func v )
            {
                output.emplace( std::in_place_type_t<func>(), std::move( params ), v->f );
            },
            [&params, this]( jmath_func_id const & v )
            {
                output.emplace( std::in_place_type_t<func_jmath>(), std::move( params ), v );
            },
            []( auto /* v */ )
            {
                throw std::invalid_argument( "Internal func error.  That's all we know." );
            },
        },
        ops.top() );
        _validate_unused_kwargs( kwargs );
        ops.pop();
    }
}

diag_value math_exp::math_exp_impl::_get_diag_value( thingie &param )
{
    diag_value val;
    std::visit( overloaded{
        [&val]( double v )
        {
            val.data.emplace<double>( v );
        },
        [&val]( std::string & v )
        {
            val.data.emplace<std::string>( std::move( v ) );
        },
        [&val]( var & v )
        {
            val.data.emplace<var_info>( std::move( v.varinfo ) );
        },
        [&val]( array & v )
        {
            diag_array arr;
            arr.reserve( v.params.size() );
            for( thingie &t : v.params ) {
                arr.emplace_back( _get_diag_value( t ) );
            }
            val.data.emplace<diag_array>( std::move( arr ) );
        },
        [&val, &param]( auto const &/* v */ )
        {
            val.data.emplace<math_exp>( math_exp_impl{ std::move( param ) } );
        },
    },
    param.data );
    return val;
}

std::vector<diag_value> math_exp::math_exp_impl::_get_diag_vals( std::vector<thingie> &params )
{
    std::vector<diag_value> vals( params.size() );
    for( decltype( vals )::size_type i = 0; i < params.size(); i++ ) {
        vals[i] = _get_diag_value( params[i] );
    }
    return vals;
}

void math_exp::math_exp_impl::new_kwarg( thingie &lhs, thingie &rhs )
{
    if( arity.top().type != arity_t::type_t::func || !arity.top().stringy ) {
        throw std::invalid_argument( "kwargs are not supported in this scope" );
    }
    if( !std::holds_alternative<std::string>( lhs.data ) ) {
        throw std::invalid_argument( "kwarg key must be a string" );
    }
    output.emplace( std::in_place_type_t<kwarg>(), std::get<std::string>( lhs.data ), rhs );
    arity.top().current--;
    arity.top().nkwargs++;
}

void math_exp::math_exp_impl::new_ternary( thingie &lhs, thingie &rhs )
{
    _validate_operand( lhs, "?:" );
    _validate_operand( rhs, "?:" );
    ops.pop();
    thingie cond = std::move( output.top() );
    _validate_operand( cond, "?:" );
    output.pop();
    output.emplace( std::in_place_type_t<ternary>(), cond, lhs, rhs );
}

void math_exp::math_exp_impl::new_array()
{
    std::vector<thingie>::size_type const nparams = arity.top().current;
    std::vector<thingie> params( nparams );
    for( std::vector<thingie>::size_type i = 0; i < nparams; i++ ) {
        params[nparams - i - 1] = std::move( output.top() );
        output.pop();
    }
    output.emplace( std::in_place_type_t<array>(), std::move( params ) );
}

void math_exp::math_exp_impl::new_oper()
{
    op_t op( ops.top() );
    ops.pop();
    std::visit( overloaded{
        [this]( pbin_op v )
        {
            cata_assert( output.size() >= 2 );
            thingie rhs = std::move( output.top() );
            output.pop();
            thingie lhs = std::move( output.top() );
            output.pop();
            if( v->symbol == "?" ) {
                throw std::invalid_argument( "Unterminated ternary" );
            }
            if( v->symbol == ":" ) {
                std::string_view top_sym =
                    !ops.empty() && std::holds_alternative<pbin_op>( ops.top() )
                    ? std::get<pbin_op>( ops.top() )->symbol
                    : std::string_view{};

                if( !output.empty() && top_sym == "?" ) {
                    new_ternary( lhs, rhs );

                } else if( !arity.empty() && arity.top().current > 0 ) {
                    new_kwarg( lhs, rhs );

                } else {
                    throw std::invalid_argument( "Misplaced colon" );
                }
            } else {
                _validate_operand( lhs, v->symbol );
                _validate_operand( rhs, v->symbol );
                output.emplace( std::in_place_type_t<oper>(), lhs, rhs, v->f );
            }
        },
        [this]( punary_op v )
        {
            cata_assert( !output.empty() );
            thingie rhs = std::move( output.top() );
            output.pop();
            _validate_operand( rhs, v->symbol );
            output.emplace( std::in_place_type_t<oper>(), thingie { 0.0 }, rhs, v->f );
        },
        []( auto /* v */ )
        {
            // we should never get here due to paren validation
            throw std::invalid_argument( "Internal oper error.  That's all we know." );
        }
    },
    op );
}

void math_exp::math_exp_impl::new_var( std::string_view str )
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
            case 'v':
                type = var_type::var;
                break;
            default:
                debugmsg( "Unknown scope %c in variable %s", str[0], str );
        }
    } else if( str.size() > 1 && str[0] == '_' ) {
        type = var_type::context;
        scoped = scoped.substr( 1 );
    }
    validate_string( scoped, "variable", " \'" );
    output.emplace( std::in_place_type_t<var>(), type, "npctalk_var_" + std::string{ scoped } );
}

void math_exp::math_exp_impl::error( std::string_view str, std::string_view what )
{
    std::ptrdiff_t offset =
        std::max<std::ptrdiff_t>( 0, last_token.data() - str.data() );
    // center the problematic token on screen if the expression is too long
    if( offset > 80 ) {
        str.remove_prefix( offset - 40 );
        offset = 40;
    }
    // NOLINTNEXTLINE(cata-translate-string-literal): debug message
    std::string mess = string_format( "Expression parsing failed: %s", what );
    if( last_token == "(" && state.expected == parse_state::expect::oper &&
        std::holds_alternative<var>( output.top().data ) ) {
        // NOLINTNEXTLINE(cata-translate-string-literal): debug message
        mess = string_format( "%s (or unknown function %s)", mess,
                              std::get<var>( output.top().data ).varinfo.name.substr( 12 ) );
    }

    offset = std::max<std::ptrdiff_t>( 0, offset - 1 );
    debugmsg( "%s\n\n%.80s\n%*s▲▲▲\n", mess, str, offset, " " );
}

void math_exp::math_exp_impl::validate_string( std::string_view str, std::string_view label,
        std::string_view badlist )
{
    std::string_view::size_type const pos = str.find_first_of( badlist );
    if( pos != std::string_view::npos ) {
        last_token.remove_prefix( pos + ( label == "string" ? 1 : 0 ) );
        // NOLINTNEXTLINE(cata-translate-string-literal): debug message
        throw std::invalid_argument( string_format( R"(Stray " %c " inside %s operand "%s")",
                                     str[pos], label, str ) );
    }
}

math_exp::math_exp( math_exp_impl impl_ )
    : impl( std::make_unique<math_exp_impl>( std::move( impl_ ) ) )
{
}

bool math_exp::parse( std::string_view str, bool assignment )
{
    impl = std::make_unique<math_exp_impl>();
    return impl->parse( str, assignment );
}

math_exp::math_exp( math_exp const &other ) :
    impl( other.impl ? std::make_unique<math_exp_impl>( *other.impl ) :
          std::make_unique<math_exp_impl>() ) {}
math_exp &math_exp::operator=( math_exp const &other )
{
    impl = other.impl ? std::make_unique<math_exp_impl>( *other.impl ) :
           std::make_unique<math_exp_impl>();
    return *this;
}
math_exp::math_exp() = default;
math_exp::~math_exp() = default;
math_exp::math_exp( math_exp &&/* other */ ) noexcept = default;
math_exp &math_exp::operator=( math_exp &&/* other */ )  noexcept = default;

double math_exp::eval( dialogue &d ) const
{
    return impl->eval( d );
}

void math_exp::assign( dialogue &d, double val ) const
{
    return impl->assign( d, val );
}
