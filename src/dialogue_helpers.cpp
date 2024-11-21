#include "dialogue_helpers.h"

#include <string>

#include "dialogue.h"
#include "rng.h"
#include "talker.h"

std::optional<std::string> maybe_read_var_value( const var_info &info, const_dialogue const &d,
        int call_depth )
{
    global_variables &globvars = get_globals();
    switch( info.type ) {
        case var_type::global:
            return globvars.maybe_get_global_value( info.name );
        case var_type::context:
            return d.maybe_get_value( info.name );
        case var_type::u:
            return d.const_actor( false )->maybe_get_value( info.name );
        case var_type::npc:
            return d.const_actor( true )->maybe_get_value( info.name );
        case var_type::var: {
            std::optional<std::string> const var_val = d.maybe_get_value( info.name );
            if( call_depth > 1000 && var_val ) {
                debugmsg( "Possible infinite loop detected: var_val points to itself or forms a cycle.  %s->%s %s",
                          info.name, var_val.value(), d.get_callstack() );
                return std::nullopt;
            } else {
                return var_val ? maybe_read_var_value( process_variable( *var_val ), d,
                                                       call_depth + 1 ) : std::nullopt;
            }
        }
        case var_type::last:
            return std::nullopt;
    }
    return std::nullopt;
}

std::string read_var_value( const var_info &info, const_dialogue const &d )
{
    return maybe_read_var_value( info, d ).value_or( std::string{} );
}

var_info process_variable( const std::string &type )
{
    var_type vt = var_type::global;
    std::string ret_str = type;

    if( type.compare( 0, 2, "u_" ) == 0 ) {
        vt = var_type::u;
        ret_str = type.substr( 2, type.size() - 2 );
    } else if( type.compare( 0, 4, "npc_" ) == 0 ) {
        vt = var_type::npc;
        ret_str = type.substr( 4, type.size() - 4 );
    } else if( type.compare( 0, 2, "n_" ) == 0 ) {
        vt = var_type::npc;
        ret_str = type.substr( 2, type.size() - 2 );
    } else if( type.compare( 0, 7, "global_" ) == 0 ) {
        vt = var_type::global;
        ret_str = type.substr( 7, type.size() - 7 );
    } else if( type.compare( 0, 2, "g_" ) == 0 ) {
        vt = var_type::global;
        ret_str = type.substr( 2, type.size() - 2 );
    } else if( type.compare( 0, 2, "v_" ) == 0 ) {
        vt = var_type::var;
        ret_str = type.substr( 2, type.size() - 2 );
    } else if( type.compare( 0, 8, "context_" ) == 0 ) {
        vt = var_type::context;
        ret_str = type.substr( 8, type.size() - 8 );
    } else if( type.compare( 0, 1, "_" ) == 0 ) {
        vt = var_type::context;
        ret_str = type.substr( 1, type.size() - 1 );
    }

    return var_info( vt, ret_str );
}

template<>
std::string str_or_var::evaluate( const_dialogue const &d ) const
{
    if( function.has_value() ) {
        return function.value()( d );
    }
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
        return {};
    }
    debugmsg( "No valid value for str_or_var_part.  %s", d.get_callstack() );
    return {};
}

template<>
std::string translation_or_var::evaluate( const_dialogue const &d ) const
{
    if( function.has_value() ) {
        return function.value()( d ).translated();
    }
    if( str_val.has_value() ) {
        return str_val.value().translated();
    }
    if( var_val.has_value() ) {
        std::string val = read_var_value( var_val.value(), d );
        if( !val.empty() ) {
            return val;
        }
        if( default_val.has_value() ) {
            return default_val.value().translated();
        }
        return {};
    }
    debugmsg( "No valid value for str_or_var_part.  %s", d.get_callstack() );
    return {};
}

std::string str_translation_or_var::evaluate( const_dialogue const &d ) const
{
    return std::visit( [&d]( auto &&val ) {
        return val.evaluate( d );
    }, val );
}

double dbl_or_var_part::evaluate( const_dialogue const &d ) const
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
        return 0;
    }
    if( math_val ) {
        return math_val->act( d );
    }
    debugmsg( "No valid value for dbl_or_var_part.  %s", d.get_callstack() );
    return 0;
}

double dbl_or_var::evaluate( const_dialogue const &d ) const
{
    if( pair ) {
        return rng( min.evaluate( d ), max.evaluate( d ) );
    }
    return min.evaluate( d );
}

time_duration duration_or_var_part::evaluate( const_dialogue const &d ) const
{
    if( dur_val.has_value() ) {
        return dur_val.value();
    }
    if( var_val.has_value() ) {
        std::string val = read_var_value( var_val.value(), d );
        if( !val.empty() ) {
            time_duration ret_val;
            ret_val = time_duration::from_turns( std::stof( val ) );
            return ret_val;
        }
        if( default_val.has_value() ) {
            return default_val.value();
        }
        return 0_seconds;
    }
    if( math_val ) {
        return time_duration::from_turns( math_val->act( d ) );
    }
    debugmsg( "No valid value for duration_or_var_part.  %s", d.get_callstack() );
    return 0_seconds;
}

time_duration duration_or_var::evaluate( const_dialogue const &d ) const
{
    if( pair ) {
        return rng( min.evaluate( d ), max.evaluate( d ) );
    }
    return min.evaluate( d );
}
