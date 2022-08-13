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

// Factoring out the distance from Beer-Lambert results in:
// 1 / (exp(a1*a2*a3*...*an)*l)
// We merge all of the absorption values by taking their cumulative average.
// TODO rename
// TODO should this return a light?
// TODO sometimes float argument as distance
float sight_calc(const float &numerator, const float &transparency, const int &distance) {
    return numerator / std::exp( transparency * distance );
}
