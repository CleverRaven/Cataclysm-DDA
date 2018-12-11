#include "rng.h"

#include <chrono>
#include <climits>
#include <cstdlib>
#include <algorithm>
#include <utility>

#include "cata_utility.h"

static std::uniform_int_distribution<long> rng_int_dist;
static std::uniform_real_distribution<double> rng_real_dist;
static std::normal_distribution<double> rng_normal_dist;

uintmax_t rng_bits()
{
    size_t size = sizeof( uintmax_t ) * CHAR_BIT;
    size_t word = rng_get_engine().word_size;
    div_t res = div( static_cast<int>( size ), static_cast<int>( word ) );
    if( res.rem ) {
        res.quot++;
    }
    uintmax_t ret = 0;
    while( true ) {
        res.quot--;
        ret |= rng_get_engine()();
        if( !res.quot ) {
            break;
        }
        ret <<= word;
    }
    return ret;
}

long rng( long lo, long hi )
{
    if( lo > hi ) {
        std::swap( lo, hi );
    }

    decltype( rng_int_dist.param() ) range( lo, hi );
    rng_int_dist.param( range );
    return rng_int_dist( rng_get_engine() );
}

double rng_float( double lo, double hi )
{
    if( lo > hi ) {
        std::swap( lo, hi );
    }

    decltype( rng_real_dist.param() ) range( lo, hi );
    rng_real_dist.param( range );
    return rng_real_dist( rng_get_engine() );
}

bool one_in( long chance )
{
    return ( chance <= 1 || rng( 0, chance - 1 ) == 0 );
}

bool x_in_y( double x, double y )
{
    return rng_float( 0, 1 ) * y <= x;
}

long dice( long number, long sides )
{
    long ret = 0;
    for( long i = 0; i < number; i++ ) {
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
    if( value > integ && x_in_y( frac, 1.0 ) ) {
        integ++;
    }

    return integ;
}

// http://www.cse.yorku.ca/~oz/hash.html
// for world seeding.
int djb2_hash( const unsigned char *input )
{
    unsigned long hash = 5381;
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

std::mt19937 &rng_get_engine()
{
    static std::mt19937 eng( std::chrono::high_resolution_clock::now().time_since_epoch().count() );
    return eng;
}

void rng_set_engine_seed( uintmax_t seed )
{
    if( seed != 0 ) {
        rng_get_engine().seed( seed );
    }
}

double normal_roll( double mean, double stddev )
{
    decltype( rng_normal_dist.param() ) params( mean, stddev );
    rng_normal_dist.param( params );
    return rng_normal_dist( rng_get_engine() );
}
