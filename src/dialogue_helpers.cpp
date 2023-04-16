#include "dialogue_helpers.h"

#include <string>

#include "dialogue.h"

std::string read_var_value( const var_info &info, const dialogue &d )
{
    std::string ret_val;
    global_variables &globvars = get_globals();
    switch( info.type ) {
        case var_type::global:
            ret_val = globvars.get_global_value( info.name );
            break;
        case var_type::u:
            ret_val = d.actor( false )->get_value( info.name );
            break;
        case var_type::npc:
            ret_val = d.actor( true )->get_value( info.name );
            break;
        case var_type::faction:
            debugmsg( "Not implemented yet." );
            break;
        case var_type::party:
            debugmsg( "Not implemented yet." );
            break;
        default:
            debugmsg( "Invalid type." );
            break;
    }
    if( ret_val.empty() ) {
        ret_val = info.default_val;
    }
    return ret_val;
}

std::string str_or_var::evaluate( dialogue const &d ) const
{
    if( str_val.has_value() ) {
        return str_val.value();
    }
    if( var_val.has_value() ) {
        std::string val = read_var_value( var_val.value(), d );
        if( !val.empty() ) {
            return val;
        }
        if( default_val.has_value() ) {
            return default_val.value();
        }
        std::string var_name = var_val.value().name;
        if( var_name.find( "npctalk_var" ) != std::string::npos ) {
            var_name = var_name.substr( 12 );
        }
        debugmsg( "No default value provided for str_or_var_part while encountering unused "
                  "variable %s.  Add a \"default_str\" member to prevent this.",
                  var_name );
        return "";
    }
    debugmsg( "No valid value for str_or_var_part." );
    return "";
}

double dbl_or_var_part::evaluate( dialogue const &d ) const
{
    if( dbl_val.has_value() ) {
        return dbl_val.value();
    }
    if( var_val.has_value() ) {
        std::string val = read_var_value( var_val.value(), d );
        if( !val.empty() ) {
            return std::stof( val );
        }
        if( default_val.has_value() ) {
            return default_val.value();
        }
        std::string var_name = var_val.value().name;
        if( var_name.find( "npctalk_var" ) != std::string::npos ) {
            var_name = var_name.substr( 12 );
        }
        debugmsg( "No default value provided for dbl_or_var_part while encountering unused "
                  "variable %s.  Add a \"default\" member to prevent this.",
                  var_name );
        return 0;
    }
    if( arithmetic_val.has_value() ) {
        arithmetic_val.value()( d );
        var_info info = var_info( var_type::global, "temp_var" );
        std::string val = read_var_value( info, d );
        if( !val.empty() ) {
            return std::stof( val );
        }
        debugmsg( "No valid arithmetic value for dbl_or_var_part." );
        return 0;
    }
    if( math_val ) {
        return math_val->act( d );
    }
    debugmsg( "No valid value for dbl_or_var_part." );
    return 0;
}

double dbl_or_var::evaluate( dialogue const &d ) const
{
    if( pair ) {
        return rng( min.evaluate( d ), max.evaluate( d ) );
    }
    return min.evaluate( d );
}

time_duration duration_or_var_part::evaluate( dialogue const &d ) const
{
    if( dur_val.has_value() ) {
        return dur_val.value();
    }
    if( var_val.has_value() ) {
        std::string val = read_var_value( var_val.value(), d );
        if( !val.empty() ) {
            time_duration ret_val;
            ret_val = time_duration::from_turns( std::stoi( val ) );
            return ret_val;
        }
        if( default_val.has_value() ) {
            return default_val.value();
        }
        std::string var_name = var_val.value().name;
        if( var_name.find( "npctalk_var" ) != std::string::npos ) {
            var_name = var_name.substr( 12 );
        }
        debugmsg( "No default value provided for duration_or_var_part while encountering unused "
                  "variable %s.  Add a \"default\" member to prevent this.",
                  var_name );
        return 0_seconds;
    }
    if( arithmetic_val.has_value() ) {
        arithmetic_val.value()( d );
        var_info info = var_info( var_type::global, "temp_var" );
        std::string val = read_var_value( info, d );
        if( !val.empty() ) {
            time_duration ret_val;
            ret_val = time_duration::from_turns( std::stoi( val ) );
            return ret_val;
        }
        debugmsg( "No valid arithmetic value for duration_or_var_part." );
        return 0_seconds;
    }
    if( math_val ) {
        return time_duration::from_turns( math_val->act( d ) );
    }
    debugmsg( "No valid value for duration_or_var_part." );
    return 0_seconds;
}

time_duration duration_or_var::evaluate( dialogue const &d ) const
{
    if( pair ) {
        return rng( min.evaluate( d ), max.evaluate( d ) );
    }
    return min.evaluate( d );
}
