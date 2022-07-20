#include "light.h"

int light::range_in_air() const
{
    if( this->value <= LIGHT_AMBIENT_LOW.value ) {
        return 0;
    }
    return static_cast<int>( -std::log( LIGHT_AMBIENT_LOW.value / this->value ) * ( 1.0 /
                             LIGHT_TRANSPARENCY_OPEN_AIR ) );
}
