#include "json.h"
#include "string_formatter.h"
#include "units.h"

namespace units
{
template<>
void volume::serialize( JsonOut &jsout ) const
{
    if( value_ % 1000 == 0 ) {
        jsout.write( string_format( "%d L", value_ / 1000 ) );
    } else {
        jsout.write( string_format( "%d ml", value_ ) );
    }
}

template<>
void mass::serialize( JsonOut &jsout ) const
{
    if( value_ % 1000000 == 0 ) {
        jsout.write( string_format( "%d kg", value_ / 1000000 ) );
    } else if( value_ % 1000 == 0 ) {
        jsout.write( string_format( "%d g", value_ / 1000 ) );
    } else {
        jsout.write( string_format( "%d mg", value_ ) );
    }
}

template<>
void length::deserialize( const JsonValue &jv )
{
    *this = read_from_json_string( jv, units::length_units );
}

template<>
void specific_energy::serialize( JsonOut &jsout ) const
{
    jsout.write( string_format( "%f", value_ ) );
}

template<>
void specific_energy::deserialize( const JsonValue &jv )
{
    if( jv.test_int() ) {
        // Compatibility with old 0.F saves
        *this = units::from_joule_per_gram( jv.get_int() );
        return;
    }
    *this = units::from_joule_per_gram( std::stof( jv.get_string() ) );
}

template<>
void temperature::serialize( JsonOut &jsout ) const
{
    jsout.write( string_format( "%f", value_ ) );
}

template<>
void temperature::deserialize( const JsonValue &jv )
{
    if( jv.test_int() ) {
        // Compatibility with old 0.F saves
        *this = from_kelvin( jv.get_int() );
        return;
    }
    *this = from_kelvin( std::stof( jv.get_string() ) );
}

template<>
void energy::serialize( JsonOut &jsout ) const
{
    if( value_ % 1000000 == 0 ) {
        jsout.write( string_format( "%d kJ", value_ / 1000000 ) );
    } else if( value_ % 1000 == 0 ) {
        jsout.write( string_format( "%d J", value_ / 1000 ) ) ;
    } else {
        jsout.write( string_format( "%d mJ", value_ ) );
    }
}

template<>
void energy::deserialize( const JsonValue &jv )
{
    *this = read_from_json_string( jv, units::energy_units );
}

template<>
void angle::serialize( JsonOut &jsout ) const
{
    jsout.write( string_format( "%f rad", value_ ) );
}

template<>
void angle::deserialize( const JsonValue &jv )
{
    *this = read_from_json_string( jv, units::angle_units );
}

std::string display( const units::energy v )
{
    const int kj = units::to_kilojoule( v );
    const int j = units::to_joule( v );
    // at least 1 kJ and there is no fraction
    if( kj >= 1 && static_cast<float>( j ) / kj == 1000 ) {
        return std::to_string( kj ) + ' ' + pgettext( "energy unit: kilojoule", "kJ" );
    }
    const int mj = units::to_millijoule( v );
    // at least 1 J and there is no fraction
    if( j >= 1 && static_cast<float>( mj ) / j  == 1000 ) {
        return std::to_string( j ) + ' ' + pgettext( "energy unit: joule", "J" );
    }
    return std::to_string( mj ) + ' ' + pgettext( "energy unit: millijoule", "mJ" );
}

} // namespace units
