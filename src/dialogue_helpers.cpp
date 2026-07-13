#include "dialogue_helpers.h"

#include <cstddef>
#include <string>
#include <type_traits>
#include <variant>

#include "cata_utility.h"
#include "debug.h"
#include "dialogue.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "global_vars.h"
#include "math_parser_diag_value.h"
#include "rng.h"
#include "string_formatter.h"
#include "talker.h"

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

namespace
{

template<typename valueT, typename funcT>
valueT _evaluate_func( const_dialogue const &d, funcT const &arg )
{
    return arg( d );
}

template<>
diag_value _evaluate_func( const_dialogue const &d, string_mutator<translation> const &arg )
{
    return diag_value{ arg( d ).translated() };
}

template<>
diag_value _evaluate_func( const_dialogue const &d, eoc_math const &arg )
{
    return diag_value{ arg.act( d ) };
}

template<>
double _evaluate_func( const_dialogue const &d, eoc_math const &arg )
{
    return arg.act( d );
}

template<>
time_duration _evaluate_func( const_dialogue const &d, eoc_math const &arg )
{
    return time_duration::from_turns( arg.act( d ) );
}

template<typename valueT>
valueT dv_to_T( diag_value const &dv )
{
    if constexpr( std::is_same_v<valueT, diag_value> ) {
        return dv;
    } else if constexpr( std::is_same_v<valueT, std::string> ) {
        return dv.str();
    } else if constexpr( std::is_same_v<valueT, double> ) {
        return dv.dbl();
    } else if constexpr( std::is_same_v<valueT, time_duration> ) {
        return time_duration::from_turns( dv.dbl() );
    } else if constexpr( std::is_same_v<valueT, translation> ) {
        // translated when set
        return translation::no_translation( dv.str() );
    }
}

template<typename V, typename T>
bool try_deserialize_type( V &v, JsonValue const &jsin )
{
    if constexpr( std::is_same_v<T, diag_value> ) {
        // ~copy of JsonValue::read() since we need to pass an extra arg to deserialize
        diag_value dv;
        try {
            dv._deserialize( jsin, false );
        } catch( JsonError const &/* je */ ) {
            return false;
        }
        v = dv;
        return true;
    } else if( T t; jsin.read( t, false ) ) {
        v = t;
        return true;
    }

    return false;
}

template<typename V, typename... T>
bool deserialize_variant( V &v, JsonValue const &jsin )
{
    // NOLINTNEXTLINE(cata-avoid-alternative-tokens) broken check
    return ( try_deserialize_type<V, T>( v, jsin ) || ... );
}

} // namespace

template<typename valueT, typename... funcT>
void value_or_var<valueT, funcT...>::deserialize( JsonValue const &jsin )
{
    if( deserialize_variant<decltype( val ), valueT, var_info, funcT...>( val, jsin ) ) {

        if( std::holds_alternative<var_info>( val ) ) {
            JsonObject const &jo_vi = jsin.get_object();
            optional( jo_vi, false, "default", default_val );
            jo_vi.allow_omitted_members();
        }
    } else {
        jsin.throw_error( "No valid value for value_or_var" );
    }
}

template<typename valueT, typename... funcT>
valueT value_or_var<valueT, funcT...>::constant() const
{
    return std::visit( overloaded{
        []( valueT const & tv ) -> valueT
        {
            return tv;
        },
        []( auto const & /* v */ ) -> valueT
        {
            debugmsg( "this value_or_var is not a constant" );

            static const valueT nulltv{};
            return nulltv;
        },
    },
    val );
}
template<typename valueT, typename... funcT>
valueT value_or_var<valueT, funcT...>::evaluate( const_dialogue const &d ) const
{
    return std::visit( overloaded{
        []( valueT const & tv ) -> valueT
        {
            return tv;
        },
        [&d, this]( var_info const & v ) -> valueT
        {
            if( diag_value const *dv = maybe_read_var_value( v, d ); dv != nullptr )
            {
                return dv_to_T<valueT>( *dv );
            }
            if( default_val )
            {
                return *default_val;
            }

            static const valueT nulltv{};
            return nulltv;
        },
        [&d]( auto const & v ) -> valueT
        {
            return _evaluate_func<valueT>( d, v );
        }
    },
    val );
}

template<typename valueT, typename... funcT>
void value_or_var_pair<valueT, funcT...>::deserialize( JsonValue const &jsin )
{
    if( jsin.test_array() ) {
        JsonArray ja = jsin.get_array();
        if( size_t size = ja.size(); size > 2 ) {
            ja.throw_error( string_format( "Too many values in array - expected 2, got %i", size ) );
        }

        ja.next_value().read( min, true );
        ja.next_value().read( max, true );
    } else {
        jsin.read( min, true );
    }
}

template<typename valueT, typename... funcT>
valueT value_or_var_pair<valueT, funcT...>::evaluate( const_dialogue const &d ) const
{
    if( max ) {
        return rng( min.evaluate( d ), max->evaluate( d ) );
    }
    return min.evaluate( d );
}

template struct value_or_var<double, eoc_math>;
template struct value_or_var_pair<double, eoc_math>;
template struct value_or_var<time_duration, eoc_math>;
template struct value_or_var_pair<time_duration, eoc_math>;
template struct value_or_var<std::string, string_mutator<std::string>>;
template struct value_or_var<translation, string_mutator<translation>>;
template struct value_or_var<diag_value, eoc_math, string_mutator<translation>>;
