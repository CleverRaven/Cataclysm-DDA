#include "math_parser_diag_value.h"

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include "cata_utility.h"
#include "debug.h"
#include "dialogue.h"
#include "math_parser_type.h"
#include "point.h"

namespace
{
template<typename T>
constexpr std::string_view _str_type_of( T const &/* t */ )
{
    if constexpr( std::is_same_v<T, double> ) {
        return "double";
    } else if constexpr( std::is_same_v<T, std::string> ) {
        return "string";
    } else if constexpr( std::is_same_v<T, diag_array> ) {
        return "array";
    } else if constexpr( std::is_same_v<T, tripoint_abs_ms> ) {
        return "tripoint";
    }
    return "cookies";
}

template<typename T, bool at_runtime = false>
T _convert( diag_value::legacy_value const &val )
{
    if constexpr( std::is_same_v<T, double> ) {
        if( std::optional<double> ret = svtod( val.val ); ret ) {
            return *ret;
        }

    } else if constexpr( std::is_same_v<T, tripoint_abs_ms> ) {
        return tripoint_abs_ms{ tripoint::from_string( val.val ) };

    } else if constexpr( std::is_same_v<T, std::string> ) {
        return val.val;
    }

    if constexpr( at_runtime ) {
        throw math::runtime_error( R"(Could not convert legacy value "%s" to a %s)", _str_type_of( T{} ) );
    }

    debugmsg( R"(Could not convert legacy value "%s" to a %s)", _str_type_of( T{} ) );
    static T const null_val{};
    return null_val;
}

template<typename C, bool at_runtime = false, typename R = C const &>
constexpr R _diag_value_helper( diag_value::impl_t const &data, const_dialogue const &/* d */ = {} )
{
    return std::visit( overloaded{
        []( std::monostate const &/* std */ ) -> R
        {
            static C const null_R{};
            return null_R;
        },
        []( auto const & v ) -> R
        {
            if constexpr( std::is_same_v<std::decay_t<decltype( v )>, C> )
            {
                return v;
            } else if constexpr( std::is_same_v<std::decay_t<decltype( v )>, diag_value::legacy_value> )
            {
                if( !v.converted ) {
                    v.converted = std::make_shared<diag_value::impl_t>( _convert<C>( v ) );
                }
                return _diag_value_helper<C, at_runtime, R>( *v.converted );
            } else if constexpr( at_runtime )
            {
                throw math::runtime_error( "Type mismatch in diag_value: requested %s, got %s", _str_type_of( C{} ),
                                           _str_type_of( v ) );
            } else
            {

                debugmsg( "Type mismatch in diag_value: requested %s, got %s", _str_type_of( C{} ),
                          _str_type_of( v ) );
                static C const null_R{};
                return null_R;
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
    return _diag_value_helper<double, false, double>( data );
}

double diag_value::dbl( const_dialogue const &d ) const
{
    return _diag_value_helper<double, true, double>( data, d );
}

bool diag_value::is_tripoint() const
{
    return std::holds_alternative<tripoint_abs_ms>( data );
}

tripoint_abs_ms const &diag_value::tripoint() const
{
    return _diag_value_helper<tripoint_abs_ms>( data );
}

tripoint_abs_ms const &diag_value::tripoint( const_dialogue const &d ) const
{
    return _diag_value_helper<tripoint_abs_ms, true>( data, d );
}

bool diag_value::is_str() const
{
    return std::holds_alternative<std::string>( data );
}

std::string const &diag_value::str() const
{
    return _diag_value_helper<std::string>( data );
}

std::string const &diag_value::str( const_dialogue const &d ) const
{
    return _diag_value_helper<std::string, true>( data, d );
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
    return _diag_value_helper<diag_array>( data );
}

diag_array const &diag_value::array( const_dialogue const &d ) const
{
    return _diag_value_helper<diag_array, true>( data, d );
}
