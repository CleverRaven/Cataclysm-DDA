#include "math_parser_jmath.h"

#include <string>
#include <string_view>

#include "condition.h"
#include "dialogue.h"
#include "generic_factory.h"
#include "math_parser.h"
#include "math_parser_diag.h"

namespace
{
generic_factory<jmath_func> &get_all_jmath_func()
{
    static generic_factory<jmath_func> jmath_func_factory( "jmath_function" );
    return jmath_func_factory;
}
} // namespace

/** @relates string_id */
template <>
jmath_func const &string_id<jmath_func>::obj() const
{
    return get_all_jmath_func().obj( *this );
}

/** @relates string_id */
template <>
bool string_id<jmath_func>::is_valid() const
{
    return get_all_jmath_func().is_valid( *this );
}

void jmath_func::reset()
{
    get_all_jmath_func().reset();
}

std::vector<jmath_func> const &jmath_func::get_all()
{
    return get_all_jmath_func().get_all();
}

void jmath_func::load_func( const JsonObject &jo, std::string const &src )
{
    get_all_jmath_func().load( jo, src );
}

void jmath_func::load( JsonObject const &jo, const std::string_view /*src*/ )
{
    optional( jo, was_loaded, "num_args", num_params );
    optional( jo, was_loaded, "return", _str );

    for( auto const &iter : get_all_diag_eval_funcs() ) {
        if( std::string const idstr = id.str(); iter.first == idstr ) {
            jo.throw_error( string_format(
                                R"(jmath function "%s" shadows a built-in function with the same name.  You must rename it.)",
                                idstr ) );
        }
    }
}

void jmath_func::finalize()
{
    for( jmath_func const &jmf : jmath_func::get_all() ) {
        jmf._exp.parse( jmf._str );
        jmf._str.clear();
    }
}

double jmath_func::eval( dialogue &d ) const
{
    return _exp.eval( d );
}

double jmath_func::eval( dialogue &d, std::vector<double> const &params ) const
{
    dialogue d_next( d );
    for( std::vector<double>::size_type i = 0; i < params.size(); i++ ) {
        write_var_value( var_type::context, "npctalk_var_" + std::to_string( i ), &d_next, params[i] );
    }

    return eval( d_next );
}
