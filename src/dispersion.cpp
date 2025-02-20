#include "dispersion.h"

#include <algorithm>

#include "rng.h"

double dispersion_sources::roll() const
{
    double this_roll = 0.0;
    for( const double &source : linear_sources ) {
        this_roll += rng_float( 0.0, source );
    }
    for( const double &source : normal_sources ) {
        this_roll += rng_normal( source );
    }
    for( const double &source : multipliers ) {
        this_roll *= source;
    }
    return std::min( this_roll, 3600.0 );
}

double dispersion_sources::max() const
{
    double sum = 0.0;
    for( const double &source : linear_sources ) {
        sum += source;
    }
    for( const double &source : normal_sources ) {
        sum += source;
    }
    for( const double &source : multipliers ) {
        sum *= source;
    }
    sum += spread_sources;
    return sum;
}

double dispersion_sources::avg() const
{
    return max() / 2.0;
}

