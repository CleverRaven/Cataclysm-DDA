#include <stdlib.h> // abs()

#include "dispersion.h"
#include "rng.h"

double dispersion_sources::roll() const
{
    double sum = 0.0;
    double this_roll = 0.0;
    for( const double &source : normal_sources ) {
        sum += source;
    }
    // normal_roll( mean, stdev )
    this_roll = abs( normal_roll( 0.0, sum ) );
    // clip the tail of the normal distribution at 2*stdev
    this_roll = std::min( this_roll, sum * 2 );

    for( const double &source : linear_sources ) {
        this_roll += rng_float( 0.0, source );
    }
    for( const double &source : multipliers ) {
        this_roll *= source;
    }
    return this_roll;
}

double dispersion_sources::max() const
{
    double sum = 0.0;
    for( const double &source : linear_sources ) {
        sum += source;
    }
    for( const double &source : normal_sources ) {
        // the max of the normal distribution is set to stdev * 2
        sum += source * 2;
    }
    for( const double &source : multipliers ) {
        sum *= source;
    }
    return sum;
}

double dispersion_sources::avg() const
{
    double sum = 0.0;
    for( const double &source : normal_sources ) {
        sum += source;
    }
    // from https://en.wikipedia.org/wiki/Half-normal_distribution
    // the mean of a half normal distribution is stdev * sqrt(2/pi)
    sum = sum * 0.798

    for( const double &source : linear_sources ) {
        sum += source / 2;
    }
    for( const double &source : multipliers ) {
        sum *= source;
    }
    return sum;
}

double dispersion_sources::stdev() const
{
    // returns the sum of all normal_sources
    double sum = 0.0;
    for( const double &source : normal_sources ) {
        sum += source;
    }
    return sum;
}
