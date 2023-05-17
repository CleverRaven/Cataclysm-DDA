#include "math_parser_diag.h"

#include <functional>
#include <string>
#include <vector>

#include "condition.h"
#include "dialogue.h"
#include "math_parser_shim.h"
#include "mission.h"
#include "options.h"
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
} // namespace

std::function<double( dialogue & )> u_val( char scope,
        std::vector<std::string> const &params )
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
        std::vector<std::string> const &params )
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
        std::vector<std::string> const &params )
{
    return[option = params[0]]( dialogue const & ) {
        return get_option<float>( option );
    };
}

std::function<double( dialogue & )> pain_eval( char scope,
        std::vector<std::string> const &/* params */ )
{
    return [beta = is_beta( scope )]( dialogue const & d ) {
        return d.actor( beta )->pain_cur();
    };
}

std::function<void( dialogue &, double )> pain_ass( char scope,
        std::vector<std::string> const &/* params */ )
{
    return [beta = is_beta( scope )]( dialogue const & d, double val ) {
        d.actor( beta )->set_pain( val );
    };
}

std::function<double( dialogue & )> skill_eval( char scope,
        std::vector<std::string> const &params )
{
    return [beta = is_beta( scope ), sid = skill_id( params[0] )]( dialogue const & d ) {
        return d.actor( beta )->get_skill_level( sid );
    };
}

std::function<void( dialogue &, double )> skill_ass( char scope,
        std::vector<std::string> const &params )
{
    return [beta = is_beta( scope ), sid = skill_id( params[0] )]( dialogue const & d, double val ) {
        return d.actor( beta )->set_skill_level( sid, val );
    };
}

std::function<double( dialogue & )> weather_eval( char /* scope */,
        std::vector<std::string> const &params )
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
    throw std::invalid_argument( "Unknown weather aspect " + params[0] );
}

std::function<void( dialogue &, double )> weather_ass( char /* scope */,
        std::vector<std::string> const &params )
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
    throw std::invalid_argument( "Unknown weather aspect " + params[0] );
}
