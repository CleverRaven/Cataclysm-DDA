#include "output.h"
#include "rng.h"
#include <stdlib.h>

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
    return ( ( double )rand() / RAND_MAX ) <= ( ( double )x / y );
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

// http://www.isthe.com/chongo/tech/comp/fnv/
// for better world seeding.
unsigned long long fnv1a_hash( const unsigned char *str )
{
	const unsigned long long offset_basis = 14695981039346656037ULL, fnv_prime = 1099511628211ULL;
	unsigned long long hash = offset_basis;
	unsigned char c = *str++;
	while ( c != '\0' ) {
		hash = ( hash ^ c ) * fnv_prime;
		c = *str++;
	}
	return hash;
}
