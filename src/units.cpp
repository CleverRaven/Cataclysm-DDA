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

} // namespace units
