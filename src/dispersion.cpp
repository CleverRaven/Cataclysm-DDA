#include "dispersion.h"

#include "rng.h"

double dispersion_sources::roll() const
{
    // The roll "latches" for a given instance of a dispersion source.
    double this_roll = 0.0;
    if( prev_roll < 0.0 ) {
        for( const double &source : linear_sources ) {
            this_roll += rng_float( 0.0, source );
        }
        for( const double &source : normal_sources ) {
            this_roll += rng_normal( source );
        }
        for( const double &source : multipliers ) {
            this_roll *= source;
        }
        prev_roll = this_roll;
    } else {
        this_roll = prev_roll;
    }
    for( const double &source : spread_sources ) {
        // Normal distribution centered on 0, but flip negatives to positive.
        double sample = rng_normal( -source, source );
        this_roll += sample >= 0 ? sample : -sample;
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
        sum += source;
    }
    for( const double &source : multipliers ) {
        sum *= source;
    }
    for( const double &spread : spread_sources ) {
        sum += spread;
    }
    return sum;
}

double dispersion_sources::avg() const
{
    return max() / 2.0;
}

