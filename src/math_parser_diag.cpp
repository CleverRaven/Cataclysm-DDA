#include "math_parser_diag.h"

#include <functional>
#include <string>
#include <type_traits>
#include <vector>

#include "cata_utility.h"
#include "condition.h"
#include "dialogue.h"
#include "game.h"
#include "math_parser_shim.h"
#include "options.h"
#include "string_input_popup.h"
#include "units.h"
#include "weather.h"

namespace
{
bool is_beta( char scope )
{
    switch( scope ) {
        case 'n':
            return true;
        case 'u':
        default:
            return false;
    }
}

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
    }
    return "cookies";
}

} // namespace

double diag_value::dbl() const
{
    return std::visit( overloaded{
        []( double v )
        {
            return v;
        },
        []( auto const & v ) -> double
        {
            throw std::invalid_argument( string_format( "Expected number, got %s", _str_type_of( v ) ) );
        },
    },
    data );
}

double diag_value::dbl( dialogue const &d ) const
{
    return std::visit( overloaded{
        []( double v )
        {
            return v;
        },
        []( std::string const & v )
        {
            if( std::optional<double> ret = svtod( v ); ret ) {
                return *ret;
            }
            debugmsg( R"(Could not convert string "%s" to double)", v );
            return 0.0;
        },
        [&d]( var_info const & v )
        {
            std::string const val = read_var_value( v, d );
            if( std::optional<double> ret = svtod( val ); ret ) {
                return *ret;
            }
            debugmsg( R"(Could not convert variable "%s" with value "%s" to double)", v.name, val );
            return 0.0;
        },
        [&d]( math_exp const & v )
        {
            // FIXME: maybe re-constify eval paths?
            return v.eval( const_cast<dialogue &>( d ) );
        }
    },
    data );
}

std::string_view diag_value::str() const
{
    return std::visit( overloaded{
        []( std::string const & v ) -> std::string const &
        {
            return v;
        },
        []( auto const & v ) -> std::string const &
        {
            throw std::invalid_argument( string_format( "Expected string, got %s", _str_type_of( v ) ) );
        },
    },
    data );
}

std::string diag_value::str( dialogue const &d ) const
{
    return std::visit( overloaded{
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
            return string_format( "%g", v.eval( const_cast<dialogue &>( d ) ) );
        }
    },
    data );
}

var_info diag_value::var() const
{
    return std::visit( overloaded{
        []( var_info const & v )
        {
            return v;
        },
        []( auto const & v ) -> var_info
        {
            throw std::invalid_argument( string_format( "Expected variable, got %s", _str_type_of( v ) ) );
        },
    },
    data );
}

std::function<double( dialogue & )> u_val( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    kwargs_shim const shim( params, scope );
    try {
        return conditional_t::get_get_dbl( shim );
    } catch( std::exception const &e ) {
        debugmsg( "shim failed: %s", e.what() );
        return []( dialogue const & ) {
            return 0;
        };
    }
}

std::function<void( dialogue &, double )> u_val_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    kwargs_shim const shim( params, scope );
    try {
        return conditional_t::get_set_dbl( shim, {}, {}, false );
    } catch( std::exception const &e ) {
        debugmsg( "shim failed: %s", e.what() );
        return []( dialogue const &, double ) {};
    }
}

std::function<double( dialogue & )> option_eval( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[option = params[0]]( dialogue const & d ) {
        return get_option<float>( option.str( d ) );
    };
}

std::function<double( dialogue & )> armor_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[type = params[0], bpid = params[1], beta = is_beta( scope )]( dialogue const & d ) {
        damage_type_id dt( type.str( d ) );
        bodypart_id bp( bpid.str( d ) );
        return d.actor( beta )->armor_at( dt, bp );
    };
}

std::function<double( dialogue & )> num_input_eval( char /*scope*/,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return[prompt = params[0], default_val = params[1]]( dialogue const & d ) {
        string_input_popup popup;
        double dv = default_val.dbl( d );
        int popup_val = dv;
        popup
        .title( _( "Input a value:" ) )
        .width( 20 )
        .description( prompt.str( d ) )
        .edit( popup_val );
        if( popup.canceled() ) {
            return dv;
        }
        return static_cast<double>( popup_val );
    };
}

std::function<double( dialogue & )> attack_speed_eval( char scope,
        std::vector<diag_value> const &/* params */, diag_kwargs const &/* kwargs */ )
{
    return[beta = is_beta( scope )]( dialogue const & d ) {
        return d.actor( beta )->attack_speed();
    };
}

std::function<double( dialogue & )> pain_eval( char scope,
        std::vector<diag_value> const &/* params */, diag_kwargs const &/* kwargs */ )
{
    return [beta = is_beta( scope )]( dialogue const & d ) {
        return d.actor( beta )->pain_cur();
    };
}

std::function<void( dialogue &, double )> pain_ass( char scope,
        std::vector<diag_value> const &/* params */, diag_kwargs const &/* kwargs */ )
{
    return [beta = is_beta( scope )]( dialogue const & d, double val ) {
        d.actor( beta )->set_pain( val );
    };
}

std::function<double( dialogue & )> skill_eval( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return [beta = is_beta( scope ), sid = params[0] ]( dialogue const & d ) {
        return d.actor( beta )->get_skill_level( skill_id( sid.str( d ) ) );
    };
}

std::function<void( dialogue &, double )> skill_ass( char scope,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    return [beta = is_beta( scope ), sid = params[0] ]( dialogue const & d, double val ) {
        return d.actor( beta )->set_skill_level( skill_id( sid.str( d ) ), val );
    };
}

std::function<double( dialogue & )> test_diag( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &kwargs )
{
    std::vector<diag_value> all_params( params );
    for( diag_kwargs::value_type const &v : kwargs ) {
        if( v.first != "test_unused_kwarg" ) {
            all_params.emplace_back( *v.second );
        }
    }
    return [all_params]( dialogue const & d ) {
        double ret = 0;
        for( diag_value const &v : all_params ) {
            ret += v.dbl( d );
        }
        return ret;
    };
}

std::function<double( dialogue & )> weather_eval( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    if( params[0] == "temperature" ) {
        return []( dialogue const & ) {
            return units::to_kelvin( get_weather().weather_precise->temperature );
        };
    }
    if( params[0] == "windpower" ) {
        return []( dialogue const & ) {
            return get_weather().weather_precise->windpower;
        };
    }
    if( params[0] == "humidity" ) {
        return []( dialogue const & ) {
            return get_weather().weather_precise->humidity;
        };
    }
    if( params[0] == "pressure" ) {
        return []( dialogue const & ) {
            return get_weather().weather_precise->pressure;
        };
    }
    throw std::invalid_argument( string_format( "Unknown weather aspect %s", params[0].str() ) );
}

std::function<void( dialogue &, double )> weather_ass( char /* scope */,
        std::vector<diag_value> const &params, diag_kwargs const &/* kwargs */ )
{
    if( params[0] == "temperature" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->temperature = units::from_kelvin( val );
            get_weather().temperature = units::from_kelvin( val );
            get_weather().clear_temp_cache();
        };
    }
    if( params[0] == "windpower" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->windpower = val;
            get_weather().clear_temp_cache();
        };
    }
    if( params[0] == "humidity" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->humidity = val;
            get_weather().clear_temp_cache();
        };
    }
    if( params[0] == "pressure" ) {
        return []( dialogue const &, double val ) {
            get_weather().weather_precise->pressure = val;
            get_weather().clear_temp_cache();
        };
    }
    throw std::invalid_argument( string_format( "Unknown weather aspect %s", params[0].str() ) );
}
