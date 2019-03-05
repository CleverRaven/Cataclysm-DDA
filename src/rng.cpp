#include "rng.h"

#include <chrono>
#include <cmath>
#include <cstdlib>

#include "output.h"

long rng( long val1, long val2 )
{
    long minVal = ( val1 < val2 ) ? val1 : val2;
    long maxVal = ( val1 < val2 ) ? val2 : val1;
    return minVal + long( ( maxVal - minVal + 1 ) * double( rand() / double( RAND_MAX + 1.0 ) ) );
}

double rng_float( double val1, double val2 )
{
    double minVal = ( val1 < val2 ) ? val1 : val2;
    double maxVal = ( val1 < val2 ) ? val2 : val1;
    return minVal + ( maxVal - minVal ) * double( rand() ) / double( RAND_MAX + 1.0 );
}

bool one_in( int chance )
{
    return ( chance <= 1 || rng( 0, chance - 1 ) == 0 );
}

//this works just like one_in, but it accepts doubles as input to calculate chances like "1 in 350,52"
bool one_in_improved( double chance )
{
    return ( chance <= 1 || rng_float( 0, chance ) < 1 );
}

bool x_in_y( double x, double y )
{
    return ( static_cast<double>( rand() ) / RAND_MAX ) <= ( static_cast<double>( x ) / y );
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
    const int trunc = int( value );
    if( value > trunc && x_in_y( value - trunc, 1.0 ) ) {
        return trunc + 1;
    }

    return trunc;
}

int divide_roll_remainder( double dividend, double divisor )
{
    const double div = dividend / divisor;
    return roll_remainder( div );
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
    return std::max( std::min( val, hi ), lo );
}

std::default_random_engine &rng_get_engine()
{
    static std::default_random_engine eng(
        std::chrono::system_clock::now().time_since_epoch().count() );
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
    return std::normal_distribution<double>( mean, stddev )( rng_get_engine() );
}
