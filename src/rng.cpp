#include "rng.h"

#include <chrono>
#include <climits>
#include <cstdlib>
#include <algorithm>
#include <utility>

#include "cata_utility.h"

unsigned int rng_bits()
{
    // Whole uint range.
    static std::uniform_int_distribution<unsigned int> rng_uint_dist;
    return rng_uint_dist( rng_get_engine() );
}

int rng( int lo, int hi )
{
    static std::uniform_int_distribution<int> rng_int_dist;
    if( lo > hi ) {
        std::swap( lo, hi );
    }
    return rng_int_dist( rng_get_engine(), std::uniform_int_distribution<>::param_type( lo, hi ) );
}

double rng_float( double lo, double hi )
{
    static std::uniform_real_distribution<double> rng_real_dist;
    if( lo > hi ) {
        std::swap( lo, hi );
    }
    return rng_real_dist( rng_get_engine(), std::uniform_real_distribution<>::param_type( lo, hi ) );
}

double normal_roll( double mean, double stddev )
{
    static std::normal_distribution<double> rng_normal_dist;
    return rng_normal_dist( rng_get_engine(), std::normal_distribution<>::param_type( mean, stddev ) );
}

bool one_in( int chance )
{
    return ( chance <= 1 || rng( 0, chance - 1 ) == 0 );
}

bool x_in_y( double x, double y )
{
    return rng_float( 0.0, 1.0 ) <= x / y;
}

int dice( int number, int sides )
{
    int ret = 0;
    for( int i = 0; i < number; i++ ) {
        ret += rng( 1, sides );
    }
    return ret;
}

// probabilistically round a double to an int
// 1.3 has a 70% chance of rounding to 1, 30% chance to 2.
int roll_remainder( double value )
{
    double integ;
    double frac = modf( value, &integ );
    if( value > 0.0 && value > integ && x_in_y( frac, 1.0 ) ) {
        integ++;
    } else if( value < 0.0 && value < integ && x_in_y( -frac, 1.0 ) ) {
        integ--;
    }

    return integ;
}

// http://www.cse.yorku.ca/~oz/hash.html
// for world seeding.
int djb2_hash( const unsigned char *input )
{
    unsigned int hash = 5381;
    unsigned char c = *input++;
    while( c != '\0' ) {
        hash = ( ( hash << 5 ) + hash ) + c; /* hash * 33 + c */
        c = *input++;
    }
    return hash;
}

double rng_normal( double lo, double hi )
{
    if( lo > hi ) {
        std::swap( lo, hi );
    }

    const double range = ( hi - lo ) / 4;
    if( range == 0.0 ) {
        return hi;
    }
    double val = normal_roll( ( hi + lo ) / 2, range );
    return clamp( val, lo, hi );
}

std::default_random_engine &rng_get_engine()
{
    static std::default_random_engine eng(
        std::chrono::high_resolution_clock::now().time_since_epoch().count() );
    return eng;
}

void rng_set_engine_seed( unsigned int seed )
{
    if( seed != 0 ) {
        rng_get_engine().seed( seed );
    }
}
