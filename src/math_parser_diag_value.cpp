#include "math_parser_diag_value.h"

#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include "cata_utility.h"
#include "cata_variant.h"
#include "debug.h"
#include "dialogue.h"
#include "flexbuffer_json.h"
#include "json.h"
#include "math_parser_type.h"
#include "point.h"
#include "translation.h"

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

std::string diag_value::to_string( bool i18n ) const
{
    return std::visit( overloaded{
        []( std::monostate const &/* std */ )
        {
            return std::string{};
        },
        [i18n]( double v )
        {
            std::ostringstream conv;
            if( !i18n ) {
                conv.imbue( std::locale::classic() );
            }
            conv << v;
            return conv.str();
        },
        [i18n]( std::string const & v )
        {
            if( i18n ) {
                // FIXME: is this the correct place to translate?
                return to_translation( v ).translated();
            }
            return v;
        },
        []( diag_array const & v )
        {
            std::string ret = "[";
            for( diag_value const &e : v ) {
                ret += e.to_string();
                ret += ",";
            }
            ret += "]";
            return ret;
        },
        []( tripoint_abs_ms const & v )
        {
            return v.to_string();
        },
        []( legacy_value const & v )
        {
            return v.val;
        },
    },
    data );

}

diag_value::diag_value( cata_variant const &cv )
{
    diag_value blorg;
    if( cv.type() == cata_variant_type::int_ ) {
        blorg = diag_value( cv.get<cata_variant_type::int_>() );
    } else if( cv.type() == cata_variant_type::bool_ ) {
        blorg = diag_value( cv.get<cata_variant_type::bool_>() );
    } else if( cv.type() == cata_variant_type::tripoint ) {
        blorg = diag_value( tripoint_abs_ms{ cv.get<cata_variant_type::tripoint>() } );
    } else {
        blorg = diag_value( cv.get_string() );
    }
    data = std::move( blorg.data );
}

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

bool operator==( diag_value::legacy_value const &lhs, diag_value::legacy_value const &rhs )
{
    return lhs.val == rhs.val;
}
bool operator!=( diag_value::legacy_value const &lhs, diag_value::legacy_value const &rhs )
{
    return lhs.val != rhs.val;
}

bool operator==( diag_value const &lhs, diag_value const &rhs )
{
    if( lhs.is_dbl() && rhs.is_dbl() ) {
        return lhs == rhs.dbl();
    }
    return lhs.data == rhs.data;
}

bool operator!=( diag_value const &lhs, diag_value const &rhs )
{
    if( lhs.is_dbl() && rhs.is_dbl() ) {
        return lhs != rhs.dbl();
    }
    return lhs.data != rhs.data;
}

bool operator==( diag_value const &lhs, double rhs )
{
    return lhs.is_dbl() && float_equals( lhs.dbl(), rhs );
}

bool operator!=( diag_value const &lhs, double rhs )
{
    return !lhs.is_dbl() || !float_equals( lhs.dbl(), rhs );
}

bool operator==( diag_value const &lhs, std::string_view rhs )
{
    return lhs.is_str() && lhs.str() == rhs;
}

bool operator!=( diag_value const &lhs, std::string_view rhs )
{
    return !lhs.is_str() || lhs.str() != rhs;
}

bool operator==( diag_value const &lhs, tripoint_abs_ms const &rhs )
{
    return lhs.is_tripoint() && lhs.tripoint() == rhs;
}

bool operator!=( diag_value const &lhs, tripoint_abs_ms const &rhs )
{
    return !lhs.is_tripoint() || lhs.tripoint() != rhs;
}

namespace
{

void _serialize( diag_value::impl_t const &data, JsonOut &jsout )
{
    std::visit( overloaded{
        [&jsout]( std::monostate const &/* std */ )
        {
            jsout.write_null();
        },
        [&jsout]( double v )
        {
            if( std::isfinite( v ) ) {
                jsout.write( v );
            } else {
                // inf and nan aren't allowed in JSON
                jsout.start_object();
                jsout.member( "dbl", std::to_string( v ) );
                jsout.end_object();
            }
        },
        [&jsout]( std::string const & v )
        {
            jsout.start_object();
            jsout.member( "str", v );
            jsout.end_object();
        },
        [&jsout]( diag_array const & v )
        {
            jsout.write_as_array( v );
        },
        [&jsout]( tripoint_abs_ms const & v )
        {
            jsout.start_object();
            jsout.member( "tripoint", v );
            jsout.end_object();
        },
        [&jsout]( diag_value::legacy_value const & v )
        {
            if( v.converted ) {
                _serialize( *v.converted, jsout );
            } else {
                jsout.write( v.val );
            }
        },
    },
    data );
}

} //namespace

void diag_value::serialize( JsonOut &jsout ) const
{
    _serialize( data, jsout );
}

void diag_value::deserialize( const JsonValue &jsin )
{
    if( jsin.test_null() ) {
        data = std::monostate{};
    } else if( jsin.test_float() ) {
        data = jsin.get_float();
    } else if( jsin.test_string() ) {
        data = legacy_value{ jsin.get_string() };
    } else if( jsin.test_array() ) {
        diag_array a;
        jsin.read( a );
        data = a;
    } else if( jsin.test_object() ) {
        JsonObject const &jo = jsin.get_object();
        if( jo.has_member( "tripoint" ) ) {
            tripoint_abs_ms t;
            jo.read( "tripoint", t );
            data = t;
        } else if( jo.has_member( "str" ) ) {
            std::string str;
            jo.read( "str", str );
            data = str;
        } else if( jo.has_member( "dbl" ) ) {
            // inf and nan
            std::string str;
            jo.read( "dbl", str );
            data = std::stof( str );
        }
    }
}
