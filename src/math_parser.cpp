#include "math_parser.h"

#include <algorithm>
#include <cstddef>
#include <locale>
#include <map>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "cata_assert.h"
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
#include "math_parser_type.h"
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
    // FIXME: port to std::from_chars once double conversion is supported
    std::istringstream conv( std::string{ token } );
    conv.imbue( std::locale::classic() );
    double val{};
    conv >> val;
    if( conv && conv.eof() ) {
        return val;
    }

    return get_constant( token );
}

constexpr std::optional<pmath_func> get_function( std::string_view token )
{
    return _get_common<pmath_func>( functions, token );
}

std::optional<scoped_diag_proto> _get_dialogue_func( std::string_view token )
{
    char scope = 'g';
    std::string_view scoped = token;
    if( token.size() > 2 && token[1] == '_' ) {
        scope = token[0];
        scoped = token.substr( 2, token.size() - 2 );
    }
    auto const &cnt_eval = get_all_diag_funcs();
    auto dfe = cnt_eval.find( scoped );
    if( dfe != cnt_eval.end() ) {
        if( dfe->second.scopes.find( scope ) == std::string_view::npos ) {
            throw math::syntax_error( "Scope %c is not valid for dialogue function %s() (valid scopes: %s)",
                                      scope, scoped, dfe->second.scopes );
        }

        return { { scoped, &dfe->second, scope } };
    }

    return std::nullopt;
}

constexpr std::optional<pbin_op> get_binary_op( std::string_view token )
{
    return _get_common<pbin_op>( binary_ops, token );
}

constexpr std::optional<punary_op> get_unary_op( std::string_view token )
{
    return _get_common<punary_op>( prefix_unary_ops, token );
}

constexpr std::optional<pass_op> get_ass_op( std::string_view token )
{
    return _get_common<pass_op>( ass_ops, token );
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
            ( previous == expect::operand && next == expect::rbracket && allows_prefix_unary ) ) {
            return;
        }
        expect const alias =
            next == expect::rparen || next == expect::rbracket ? expect::oper : next;
        if( expected != alias &&
            ( expected != expect::operand ||
              ( alias != expect::lparen && alias != expect::lbracket ) ) &&
            ( expected != expect::oper || alias != expect::eof ) ) {
            throw math::syntax_error( "Expected %s, got %s",
                                      expect_to_string( expected ),
                                      expect_to_string( next ) );
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
           std::holds_alternative<scoped_diag_proto>( op );
}

bool is_assign_target( thingie const &thing )
{
    return std::holds_alternative<var>( thing.data ) ||
           std::holds_alternative<func_diag>( thing.data );
}

std::vector<double> _eval_params( std::vector<thingie> const &params, const_dialogue const &d )
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
        throw math::syntax_error( R"(Operator "%s" does not support string operands)", symbol );
    }
    if( std::holds_alternative<array>( thing.data ) ) {
        throw math::syntax_error( R"(Operator "%s" does not support array operands)", symbol );
    }
}

void _validate_unused_kwargs( diag_kwargs const &kwargs )
{
    for( diag_kwargs::impl_t::value_type const &v : kwargs.kwargs ) {
        if( !v.second.was_used() ) {
            throw math::syntax_error( R"(Unused kwarg "%s")", v.first );
        }
    }
}

} // namespace

func::func( std::vector<thingie> &&params_, math_func::f_t f_ ) : params( params_ ),
    f( f_ ) {}
func_jmath::func_jmath( std::vector<thingie> &&params_,
                        jmath_func_id const &id_ ) : params( params_ ),
    id( id_ ) {}

double func::eval( const_dialogue const &d ) const
{
    return f( _eval_params( params, d ) );
}

double func_jmath::eval( const_dialogue const &d ) const
{
    return id->eval( d, _eval_params( params, d ) );
}

double var::eval( const_dialogue const &d ) const
{
    std::string const str = read_var_value( varinfo, d );
    if( str.empty() ) {
        return 0;
    }
    if( std::optional<double> ret = svtod( str ); ret ) {
        return *ret;
    }
    throw math::runtime_error( R"(failed to convert variable "%s" with value "%s" to a number)",
                               varinfo.name, str );
    return 0;
}

void var::assign( dialogue &d, double val ) const
{
    write_var_value( varinfo.type, varinfo.name, &d, val );
}

oper::oper( thingie l_, thingie r_, binary_op::f_t op_ ):
    l( std::make_shared<thingie>( std::move( l_ ) ) ),
    r( std::make_shared<thingie>( std::move( r_ ) ) ),
    op( op_ ) {}

double oper::eval( const_dialogue const &d ) const
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

double ternary::eval( const_dialogue const &d ) const
{
    return cond->eval( d ) > 0 ? mhs->eval( d ) : rhs->eval( d );
}

ass_oper::ass_oper( thingie lhs_, thingie mhs_, thingie rhs_, binary_op::f_t op_ )
    : lhs( std::make_shared<thingie>( std::move( lhs_ ) ) ),
      mhs( std::make_shared<thingie>( std::move( mhs_ ) ) ),
      rhs( std::make_shared<thingie>( std::move( rhs_ ) ) ),
      op( op_ ) {}

double ass_oper::eval( dialogue &d ) const
{
    std::visit( overloaded{
        [&d, this]( auto const & v ) -> void
        {
            if constexpr( v_has_assign<decltype( v )> )
            {
                double const val = op( mhs->eval( d ), rhs->eval( d ) ) ;
                v.assign( d, val );
            } else
            {
                throw math::internal_error( "math called assign() on unexpected node without assign()" );
            }
        },
    },
    lhs->data );

    return 0;
}

class math_exp::math_exp_impl
{
    public:
        math_exp_impl() = default;
        explicit math_exp_impl( thingie &&t ): tree( t ) {}

        bool parse( std::string_view str, bool handle_errors ) {
            if( str.empty() ) {
                return false;
            }
            try {
                _parse( str );
            } catch( math::syntax_error const &ex ) {
                if( handle_errors ) {
                    debugmsg( error( str, ex.what() ) );
                    ops = {};
                    output = {};
                    arity = {};
                    tree = thingie { 0.0 };
                    return false;
                }

                throw math::exception( error( str, ex.what() ) );
            }
            return true;
        }
        double eval( const_dialogue const &d ) const {
            return tree.eval( d );
        }
        double eval( dialogue &d ) const {
            return tree.eval( d );
        }

        math_type_t get_type() const {
            return type;
        }

    private:
        struct op_ctxt {
            op_t op;
            std::string_view pos;

            op_ctxt( op_t op_, std::string_view pos_ ): op( op_ ), pos( pos_ ) {};
        };
        std::stack<op_ctxt> ops;
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
        std::string_view parse_position;
        parse_state state;
        math_type_t type = math_type_t::ret;

        void _parse( std::string_view str );
        void parse_string( std::string_view token, std::string_view full );
        void parse_bin_op( pbin_op const &op, std::string_view pos );
        void parse_ass_op( pass_op const &op, std::string_view pos );
        void parse_diag_f( std::string_view symbol, scoped_diag_proto const &token );
        void parse_comma();
        void parse_lparen( std::string_view pos, arity_t::type_t type = arity_t::type_t::parens );
        void parse_rparen( std::string_view pos );
        void parse_lbracket( std::string_view pos );
        void parse_rbracket( std::string_view pos );
        void new_func();
        void new_oper();
        void new_var( std::string_view str );
        void new_kwarg( thingie &lhs, thingie &rhs );
        void new_ternary( thingie &lhs, thingie &rhs );
        void new_array();
        void maybe_first_argument();
        std::string error( std::string_view str, std::string_view what );
        static void validate_string( std::string_view str, std::string_view badlist );
        static std::vector<diag_value> _get_diag_vals( thingie &thing );
        thingie _resolve_proto( thingie &thing, bool assignment = false,
                                std::vector<diag_value> const *args_ = nullptr );
        static diag_value _get_diag_value( thingie &param );
};

void math_exp::math_exp_impl::maybe_first_argument()
{
    if( !arity.empty() && arity.top().allows_comma() && arity.top().current == 0 ) {
        arity.top().current++;
    }
}

void math_exp::math_exp_impl::_parse( std::string_view str )
{
    constexpr std::string_view expression_separators = "+-*/^,()[]%':><=!?";
    state = {};
    for( std::string_view const token : tokenize( str, expression_separators ) ) {
        parse_position = token;
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
            ops.emplace( *ftoken, token );
            arity.emplace( ( *ftoken )->symbol, ( *ftoken )->num_params, arity_t::type_t::func );
            state.set( parse_state::expect::lparen );

        } else if( jmath_func_id jmfid( token ); jmfid.is_valid() ) {
            state.validate( parse_state::expect::operand );
            maybe_first_argument();
            ops.emplace( jmfid, token );
            arity.emplace( token, jmfid->num_params, arity_t::type_t::func );
            state.set( parse_state::expect::lparen );

        } else if( std::optional<scoped_diag_proto> fproto = _get_dialogue_func( token ); fproto ) {
            parse_diag_f( token, *fproto );

        } else if( std::optional<punary_op> op = get_unary_op( token ); op && state.allows_prefix_unary ) {
            state.validate( parse_state::expect::operand );
            ops.emplace( *op, token );
            state.set( parse_state::expect::operand );

        } else if( std::optional<pbin_op> op = get_binary_op( token ); op ) {
            parse_bin_op( *op, token );

        } else if( std::optional<pass_op> op = get_ass_op( token ); op ) {
            parse_ass_op( *op, token );

        } else if( token == "," ) {
            parse_comma();

        } else if( token == "(" ) {
            parse_lparen( token );

        } else if( token == ")" ) {
            parse_rparen( token );

        } else if( token == "[" ) {
            parse_lbracket( token );

        } else if( token == "]" ) {
            parse_rbracket( token );

        } else {
            state.validate( parse_state::expect::operand );
            maybe_first_argument();
            new_var( token );
            state.set( parse_state::expect::oper );
        }
    }
    state.validate( parse_state::expect::eof );
    while( !ops.empty() ) {
        op_t const &op = ops.top().op;
        parse_position = ops.top().pos;
        if( std::holds_alternative<paren>( op ) && std::get<paren>( op ) == paren::left ) {
            throw math::syntax_error( "Unterminated left paranthesis" );
        }
        if( std::holds_alternative<paren>( op ) && std::get<paren>( op ) == paren::left_sq ) {
            throw math::syntax_error( "Unterminated left bracket" );
        }
        new_oper();
    }

    tree = _resolve_proto( output.top() );

    if( output.size() != 1 ) {
        throw math::internal_error( "Invalid expression.  That's all we know.  Blame andrei." );
    }
    output.pop();
}

void math_exp::math_exp_impl::parse_string( std::string_view token, std::string_view full )
{
    if( !state.instring ) {
        state.validate( parse_state::expect::operand );
        maybe_first_argument();
        if( arity.empty() || !arity.top().stringy ) {
            throw math::syntax_error( "String arguments can only be used in dialogue functions" );
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

void math_exp::math_exp_impl::parse_bin_op( pbin_op const &op, std::string_view pos )
{
    if( op->symbol == ":" && !arity.empty() && arity.top().type == arity_t::type_t::ternary ) {
        // insert a pair of parens to help resolve nested ternaries like 0?0?-1:-2:1
        parse_rparen( pos );
    }
    state.validate( parse_state::expect::oper );
    while( !ops.empty() && ops.top().op > *op ) {
        new_oper();
    }
    ops.emplace( op, pos );
    state.set( parse_state::expect::operand, true );
    if( op->symbol == "?" ) {
        parse_lparen( pos, arity_t::type_t::ternary );
    }
}

void math_exp::math_exp_impl::parse_ass_op( pass_op const &op, std::string_view pos )
{
    state.validate( parse_state::expect::oper );
    while( !ops.empty() && ops.top().op > assignment_op ) {
        new_oper();
    }
    ops.emplace( op, pos );
    if( op->unaryone ) {
        state.set( parse_state::expect::eof, true );
    } else {
        state.set( parse_state::expect::operand, true );
    }
}

void math_exp::math_exp_impl::parse_diag_f(
    std::string_view symbol, scoped_diag_proto const &token )
{
    state.validate( parse_state::expect::operand );
    ops.emplace( token, symbol );
    arity.emplace( symbol, token.df->num_params, arity_t::type_t::func, true );
    state.set( parse_state::expect::lparen, false );
}

void math_exp::math_exp_impl::parse_comma()
{
    state.validate( parse_state::expect::oper );
    if( arity.empty() || !arity.top().allows_comma() ) {
        throw math::syntax_error( "Misplaced comma" );
    }
    while( ops.top().op > paren::left ) {
        new_oper();
    }
    arity.top().current++;
    state.set( parse_state::expect::operand, true );
}

void math_exp::math_exp_impl::parse_lparen( std::string_view pos, arity_t::type_t type )
{
    state.validate( parse_state::expect::lparen );
    if( ops.empty() || !is_function( ops.top().op ) ) {
        arity.emplace( std::string_view{}, 0, type, !arity.empty() && arity.top().stringy );
    }
    ops.emplace( paren::left, pos );
    state.set( parse_state::expect::operand, true );
}

void math_exp::math_exp_impl::parse_rparen( std::string_view pos )
{
    state.validate( parse_state::expect::rparen );
    while( !ops.empty() && ops.top().op > paren::left ) {
        new_oper();
    }
    if( ops.empty() || !std::holds_alternative<paren>( ops.top().op ) ||
        std::get<paren>( ops.top().op ) != paren::left ) {
        parse_position = pos;
        throw math::syntax_error( "Misplaced right parenthesis" );
    }
    ops.pop();
    new_func();
    arity.pop();
    maybe_first_argument();
    state.set( parse_state::expect::oper, false );
}

void math_exp::math_exp_impl::parse_lbracket( std::string_view pos )
{
    state.validate( parse_state::expect::lbracket );
    if( arity.empty() || !arity.top().stringy ) {
        throw math::syntax_error( "Arrays can only be used as arguments for dialogue functions" );
    }
    arity.emplace( std::string_view{}, 0, arity_t::type_t::array,
                   !arity.empty() && arity.top().stringy );
    ops.emplace( paren::left_sq, pos );
    state.set( parse_state::expect::operand, true );
}

void math_exp::math_exp_impl::parse_rbracket( std::string_view pos )
{
    state.validate( parse_state::expect::rbracket );
    if( arity.empty() || arity.top().type != arity_t::type_t::array ) {
        parse_position = pos;
        throw math::syntax_error( "Misplaced right bracket" );
    }
    while( !ops.empty() && ops.top().op > paren::left_sq ) {
        new_oper();
    }
    ops.pop();
    new_array();
    arity.pop();
    maybe_first_argument();
    state.set( parse_state::expect::oper );
}

thingie math_exp::math_exp_impl::_resolve_proto( thingie &thing, bool assignment,
        std::vector<diag_value> const *args_ )
{

    if( std::holds_alternative<func_diag_proto>( thing.data ) ) {
        func_diag_proto &proto = std::get<func_diag_proto>( thing.data );
        std::vector<diag_value> const &args = args_ == nullptr ? _get_diag_vals( thing ) : *args_;
        parse_position = proto.token;

        if( !assignment && proto.f->fe == nullptr ) {
            throw math::syntax_error( "Function prototype %s() cannot be evaluated", proto.token );
        }
        if( assignment && proto.f->fa == nullptr ) {
            throw math::syntax_error( "Function prototype %s() cannot be used as an assignment target",
                                      proto.token );
        }

        func_diag::eval_f fe =
            assignment ? func_diag::eval_f{} :
            proto.f->fe( proto.scope, args, proto.kwargs );
        func_diag::ass_f fa =
            assignment ? proto.f->fa( proto.scope, args, proto.kwargs ) : func_diag::ass_f{};

        _validate_unused_kwargs( proto.kwargs );

        return thingie{ std::in_place_type_t<func_diag>{}, fe, fa };
    }

    return thing;
}

void math_exp::math_exp_impl::new_func()
{
    if( !ops.empty() && is_function( ops.top().op ) ) {
        std::vector<thingie>::size_type const nparams = arity.top().current;
        if( arity.top().expected >= 0 ) {
            if( arity.top().current < arity.top().expected ) {
                throw math::syntax_error( "Not enough arguments for function %s()", arity.top().sym );
            }
            if( arity.top().current > arity.top().expected ) {
                throw math::syntax_error( "Too many arguments for function %s()", arity.top().sym );
            }
        }

        std::vector<thingie> params( nparams );
        diag_kwargs kwargs;
        for( std::vector<kwarg>::size_type i = 0; i < arity.top().nkwargs; i++ ) {
            if( !std::holds_alternative<kwarg>( output.top().data ) ) {
                throw math::syntax_error( "All positional arguments must precede keyword-value pairs" );
            }
            kwarg &kw = std::get<kwarg>( output.top().data );
            kwargs.kwargs.emplace( kw.key, _get_diag_value( *kw.val ) );
            output.pop();
        }
        for( std::vector<thingie>::size_type i = 0; i < nparams; i++ ) {
            params[nparams - i - 1] = _resolve_proto( output.top() );
            output.pop();
        }
        std::visit( overloaded{
            [&params, &kwargs, this]( scoped_diag_proto const & v )
            {
                output.emplace( std::in_place_type_t<func_diag_proto>(), v.token, v.scope,
                                v.df, params, kwargs );
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
                throw math::internal_error( "Internal func error.  That's all we know." );
            },
        },
        ops.top().op );
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

std::vector<diag_value> math_exp::math_exp_impl::_get_diag_vals( thingie &thing )
{
    if( std::holds_alternative<func_diag_proto>( thing.data ) ) {
        func_diag_proto &proto = std::get<func_diag_proto>( thing.data );
        std::vector<thingie> &params = proto.args;
        std::vector<diag_value> vals( params.size() );
        for( decltype( vals )::size_type i = 0; i < params.size(); i++ ) {
            vals[i] = _get_diag_value( params[i] );
        }
        return vals;
    }
    return {};
}

void math_exp::math_exp_impl::new_kwarg( thingie &lhs, thingie &rhs )
{
    if( arity.top().type != arity_t::type_t::func || !arity.top().stringy ) {
        throw math::syntax_error( "kwargs are not supported in this scope" );
    }
    if( !std::holds_alternative<std::string>( lhs.data ) ) {
        throw math::syntax_error( "kwarg key must be a string" );
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
    thingie cond = _resolve_proto( output.top() );
    _validate_operand( cond, "?:" );
    output.pop();
    output.emplace( std::in_place_type_t<ternary>(), cond, lhs, rhs );
}

void math_exp::math_exp_impl::new_array()
{
    std::vector<thingie>::size_type const nparams = arity.top().current;
    std::vector<thingie> params( nparams );
    for( std::vector<thingie>::size_type i = 0; i < nparams; i++ ) {
        params[nparams - i - 1] = _resolve_proto( output.top() );
        output.pop();
    }
    output.emplace( std::in_place_type_t<array>(), std::move( params ) );
}

void math_exp::math_exp_impl::new_oper()
{
    op_ctxt op( ops.top() );
    ops.pop();
    std::visit( overloaded{
        [this, &op]( pbin_op v )
        {
            cata_assert( output.size() >= 2 );
            thingie rhs = _resolve_proto( output.top() );
            output.pop();
            thingie lhs = _resolve_proto( output.top() );
            output.pop();
            parse_position = op.pos;
            if( v->symbol == "?" ) {
                throw math::syntax_error( "Unterminated ternary" );
            }
            if( v->symbol == ":" ) {
                std::string_view top_sym =
                    !ops.empty() && std::holds_alternative<pbin_op>( ops.top().op )
                    ? std::get<pbin_op>( ops.top().op )->symbol
                    : std::string_view{};

                if( !output.empty() && top_sym == "?" ) {
                    new_ternary( lhs, rhs );

                } else if( !arity.empty() && arity.top().current > 0 ) {
                    new_kwarg( lhs, rhs );

                } else {
                    throw math::syntax_error( "Misplaced colon" );
                }
            } else {
                parse_position = op.pos;
                _validate_operand( lhs, v->symbol );
                _validate_operand( rhs, v->symbol );
                if( output.empty() && arity.empty() ) {
                    type = v->type;
                }
                output.emplace( std::in_place_type_t<oper>(), lhs, rhs, v->f );
            }
        },
        [this, &op]( punary_op v )
        {
            cata_assert( !output.empty() );
            thingie rhs = _resolve_proto( output.top() );
            output.pop();
            parse_position = op.pos;
            _validate_operand( rhs, v->symbol );
            output.emplace( std::in_place_type_t<oper>(), thingie { 0.0 }, rhs, v->f );
        },
        [this, &op]( pass_op v )
        {
            thingie rhs{ 1.0 };
            if( !v->unaryone ) {
                cata_assert( output.size() >= 2 );
                rhs = _resolve_proto( output.top() );
                output.pop();
            }

            thingie temp = std::move( output.top() );
            output.pop();
            std::vector<diag_value> const &args = _get_diag_vals( temp );
            thingie lhs = _resolve_proto( temp, true, &args );

            thingie mhs{ 0.0 };
            if( v->needs_mhs ) {
                mhs = _resolve_proto( temp, false, &args );
            }

            parse_position = op.pos;
            if( !is_assign_target( lhs ) ) {
                throw math::syntax_error( "lhs of assignment operator must be an assign target" );
            }
            if( !output.empty() || !arity.empty() ) {
                throw math::syntax_error( "misplaced assignment operator" );
            }
            type = math_type_t::assign;
            output.emplace( std::in_place_type_t<ass_oper>(), lhs, mhs, rhs, v->f );
        },
        []( auto /* v */ )
        {
            // we should never get here due to paren validation
            throw math::internal_error( "Internal oper error.  That's all we know." );
        }
    },
    op.op );
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
                throw math::syntax_error( "Unknown scope %c in variable %s", str[0], str );
        }
    } else if( str.size() > 1 && str[0] == '_' ) {
        type = var_type::context;
        scoped = scoped.substr( 1 );
    }
    validate_string( scoped, " \'" );
    output.emplace( std::in_place_type_t<var>(), type, std::string{ scoped } );
}

std::string math_exp::math_exp_impl::error( std::string_view str, std::string_view what )
{
    std::ptrdiff_t offset =
        std::max<std::ptrdiff_t>( 0, parse_position.data() - str.data() );
    // center the problematic token on screen if the expression is too long
    if( offset > 80 ) {
        str.remove_prefix( offset - 40 );
        offset = 40;
    }
    // NOLINTNEXTLINE(cata-translate-string-literal): debug message
    std::string mess = string_format( "Expression parsing failed: %s", what );
    if( parse_position == "(" && state.expected == parse_state::expect::oper &&
        std::holds_alternative<var>( output.top().data ) ) {
        // NOLINTNEXTLINE(cata-translate-string-literal): debug message
        mess = string_format( "%s (or unknown function %s)", mess,
                              std::get<var>( output.top().data ).varinfo.name );
    }

    offset = std::max<std::ptrdiff_t>( 0, offset - 1 );
    // NOLINTNEXTLINE(cata-translate-string-literal): debug message
    return string_format( "\n%s\n\n%.80s\n%*s▲▲▲\n", mess, str, offset, " " );
}

void math_exp::math_exp_impl::validate_string( std::string_view str, std::string_view badlist )
{
    std::string_view::size_type const pos = str.find_first_of( badlist );
    if( pos != std::string_view::npos ) {
        // NOLINTNEXTLINE(cata-translate-string-literal): debug message
        throw math::syntax_error( R"(Stray " %c " inside %s operand "%s")", str[pos], str );
    }
}

math_exp::math_exp( math_exp_impl impl_ )
    : impl( std::make_unique<math_exp_impl>( std::move( impl_ ) ) )
{
}

bool math_exp::parse( std::string_view str, bool handle_errors )
{
    impl = std::make_unique<math_exp_impl>();
    return impl->parse( str, handle_errors );
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

double math_exp::eval( const_dialogue const &d ) const
{
    return impl->eval( d );
}

double math_exp::eval( dialogue &d ) const
{
    return impl->eval( d );
}

math_type_t math_exp::get_type() const
{
    return impl->get_type();
}
