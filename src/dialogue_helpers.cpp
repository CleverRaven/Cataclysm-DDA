#include "dialogue_helpers.h"

#include <string>

#include "dialogue.h"
#include "global_vars.h"
#include "rng.h"
#include "talker.h"
#include "math_parser_diag_value.h"

diag_value const &read_var_value( const var_info &info, const_dialogue const &d )
{
    static diag_value const null_val;
    diag_value const *ret = maybe_read_var_value( info, d );
    return ret ? *ret : null_val;
}

diag_value const *maybe_read_var_value( const var_info &info, const_dialogue const &d )
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
            diag_value const *const var_val = d.maybe_get_value( info.name );
            return var_val ? maybe_read_var_value( process_variable( var_val->str() ), d ) : nullptr;
        }
        case var_type::last:
            return nullptr;
    }
    return nullptr;
}

var_info process_variable( const std::string &type )
{
    var_type vt = var_type::global;
    std::string ret_str = type;

    if( type.compare( 0, 2, "u_" ) == 0 ) {
        vt = var_type::u;
        ret_str = type.substr( 2, type.size() - 2 );
    } else if( type.compare( 0, 2, "n_" ) == 0 ) {
        vt = var_type::npc;
        ret_str = type.substr( 2, type.size() - 2 );
    } else if( type.compare( 0, 1, "_" ) == 0 ) {
        vt = var_type::context;
        ret_str = type.substr( 1, type.size() - 1 );
    }

    return { vt, ret_str };
}

template<>
std::string str_or_var::evaluate( const_dialogue const &d, bool convert ) const
{
    if( function.has_value() ) {
        return function.value()( d );
    }
    if( str_val.has_value() ) {
        return str_val.value();
    }
    if( var_val.has_value() ) {
        diag_value const *val = maybe_read_var_value( var_val.value(), d );
        if( val ) {
            if( convert ) {
                return val->to_string();
            }
            return val->str();
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
std::string translation_or_var::evaluate( const_dialogue const &d, bool convert ) const
{
    if( function.has_value() ) {
        return function.value()( d ).translated();
    }
    if( str_val.has_value() ) {
        return str_val.value().translated();
    }
    if( var_val.has_value() ) {
        diag_value const *val = maybe_read_var_value( var_val.value(), d );
        if( val ) {
            if( convert ) {
                return val->to_string( true );
            }
            return val->str();
        }
        if( default_val.has_value() ) {
            return default_val.value().translated();
        }
        return {};
    }
    debugmsg( "No valid value for str_or_var_part.  %s", d.get_callstack() );
    return {};
}

std::string str_translation_or_var::evaluate( const_dialogue const &d, bool convert ) const
{
    return std::visit( [&d, convert]( auto &&val ) {
        return val.evaluate( d, convert );
    }, val );
}

double dbl_or_var_part::evaluate( const_dialogue const &d ) const
{
    if( dbl_val.has_value() ) {
        return dbl_val.value();
    }
    if( var_val.has_value() ) {
        diag_value const *val = maybe_read_var_value( var_val.value(), d );
        if( val ) {
            return val->dbl();
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
        diag_value const *val = maybe_read_var_value( var_val.value(), d );
        if( val ) {
            return time_duration::from_turns( val->dbl() );
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
