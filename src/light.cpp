#include "light.h"

/* Via Beer-Lambert we have:
* light_level * (1 / exp( transparency * distance) ) <= threshold
* Solving for distance:
* 1 / exp( transparency * distance ) <= threshold / light_level
* 1 <= exp( transparency * distance ) * threshold / light_level
* light_level <= exp( transparency * distance ) * threshold
* log(light_level) <= transparency * distance + log(threshold)
* log(light_level) - log(threshold) <= transparency * distance
* - log(threshold / light_level) <= transparency * distance
* - log(threshold / light_level) / transparency <= distance
*/
int light::sight_range(light threshold, float transparency) const
{
    // We always see at least one tile so we can see ourselves
    if( *this <= threshold ) {
        return 1;
    }
    return static_cast<int>( -std::log( threshold / *this ) / transparency);
}
