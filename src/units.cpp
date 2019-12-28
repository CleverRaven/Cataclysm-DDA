#include "json.h"
#include "units.h"

namespace units
{
template<>
void volume::serialize( JsonOut &jsout ) const
{
    if( value_ % 1000 == 0 ) {
        jsout.write( string_format( "%d L", value_ ) );
    } else {
        jsout.write( string_format( "%d ml", value_ ) );
    }
}

template<>
void mass::serialize( JsonOut &jsout ) const
{
    if( value_ % 1000000 == 0 ) {
        jsout.write( string_format( "%d kg", value_ ) );
    } else if( value_ % 1000 == 0 ) {
        jsout.write( string_format( "%d g", value_ ) );
    } else {
        jsout.write( string_format( "%d mg", value_ ) );
    }
}
} // namespace units
