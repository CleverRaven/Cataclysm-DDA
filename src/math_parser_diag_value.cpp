#include "math_parser_diag_value.h"

#include <optional>
#include <string>
#include <variant>

#include "cata_utility.h"
#include "math_parser.h"
#include "string_formatter.h"

namespace
{
template<typename T>
constexpr std::string_view _str_type_of( T /* t */ )
{
    if constexpr( std::is_same_v<T, double> ) {
        return "double";
    } else if constexpr( std::is_same_v<T, std::string> ) {
        return "string";
    } else if constexpr( std::is_same_v<T, var_info> ) {
        return "variable";
    } else if constexpr( std::is_same_v<T, math_exp> ) {
        return "sub-expression";
    } else if constexpr( std::is_same_v<T, diag_array> ) {
        return "array";
    }
    return "cookies";
}

template<class C, class R = C, bool at_runtime = false>
constexpr R _diag_value_at_parse_time( diag_value::impl_t const &data )
{
    return std::visit( overloaded{
        []( std::monostate const &/* std */ ) -> R
        {
            static R null_R{};
            return null_R;
        },
        []( C const & v ) -> R
        {
            return v;
        },
        []( auto const & v ) -> R
        {
            if constexpr( at_runtime )
            {
                throw math::runtime_error( "Expected %s, got %s", _str_type_of( C{} ), _str_type_of( v ) );
            } else
            {
                throw math::syntax_error( "Expected %s, got %s", _str_type_of( C{} ), _str_type_of( v ) );
            }
        },
    },
    data );
}

} // namespace

bool diag_value::is_dbl() const
{
    return std::holds_alternative<double>( data );
}

double diag_value::dbl() const
{
    return _diag_value_at_parse_time<double>( data );
}

double diag_value::dbl( const_dialogue const &d ) const
{
    return std::visit( overloaded{
        []( std::monostate const &/* std */ )
        {
            return 0.0;
        },
        []( double v )
        {
            return v;
        },
        []( std::string const & v )
        {
            if( std::optional<double> ret = svtod( v ); ret ) {
                return *ret;
            }
            throw math::runtime_error( R"(Could not convert string "%s" to double)", v );
        },
        [&d]( var_info const & v )
        {
            std::string const val = read_var_value( v, d );
            if( std::optional<double> ret = svtod( val ); ret ) {
                return *ret;
            }
            throw math::runtime_error( R"(Could not convert variable "%s" with value "%s" to double)", v.name,
                                       val );
        },
        [&d]( math_exp const & v ) -> double
        {
            return v.eval( d );
        },
        []( diag_array const & ) -> double
        {
            throw math::runtime_error( R"(Cannot directly convert array to doubles)" );
        },
    },
    data );
}

bool diag_value::is_str() const
{
    return std::holds_alternative<std::string>( data );
}

std::string_view diag_value::str() const
{
    return _diag_value_at_parse_time<std::string, std::string_view>( data );
}

std::string diag_value::str( const_dialogue const &d ) const
{
    return std::visit( overloaded{
        []( std::monostate const &/* std */ )
        {
            return std::string{};
        },
        []( double v )
        {
            // NOLINTNEXTLINE(cata-translate-string-literal)
            return string_format( "%g", v );
        },
        []( std::string const & v )
        {
            return v;
        },
        [&d]( var_info const & v )
        {
            return read_var_value( v, d );
        },
        [&d]( math_exp const & v )
        {
            // NOLINTNEXTLINE(cata-translate-string-literal)
            return string_format( "%g", v.eval( d ) );
        },
        []( diag_array const & )
        {
            throw math::runtime_error( R"(Cannot directly convert array to strings)" );
            return std::string{};
        },
    },
    data );
}

bool diag_value::is_var() const
{
    return std::holds_alternative<var_info>( data );
}

var_info diag_value::var() const
{
    return _diag_value_at_parse_time<var_info>( data );
}

var_info diag_value::var( const_dialogue const &/* d */ ) const
{
    return _diag_value_at_parse_time<var_info, var_info const &, true>( data );
}

bool diag_value::is_array() const
{
    return std::holds_alternative<diag_array>( data );
}

bool diag_value::is_empty() const
{
    return std::holds_alternative<std::monostate>( data );
}

diag_array const &diag_value::array() const
{
    return _diag_value_at_parse_time<diag_array, diag_array const &>( data );
}

diag_array const &diag_value::array( const_dialogue const &/* d */ ) const
{
    return _diag_value_at_parse_time<diag_array, diag_array const &, true>( data );
}
