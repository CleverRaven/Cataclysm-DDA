#include "math_parser_diag_value.h"

#include <string>
#include <variant>

#include "cata_utility.h"
#include "dialogue.h"
#include "math_parser_type.h"

namespace
{
template<typename T>
constexpr std::string_view _str_type_of( T /* t */ )
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
            if constexpr( std::is_same_v<decltype( v ), C> )
            {
                return v;
            } else if constexpr( at_runtime )
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
    return _diag_value_helper<double, false, double>( data );
}

double diag_value::dbl( const_dialogue const &/* d */ ) const
{
    return _diag_value_helper<double, true, double>( data );
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

diag_array const &diag_value::array( const_dialogue const &/* d */ ) const
{
    return _diag_value_helper<diag_array, true>( data );
}
