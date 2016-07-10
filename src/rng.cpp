#include "output.h"
#include "rng.h"
#include <stdlib.h>

int xorshift_pointer;
unsigned long long xorshift_state[16];

// xorshift1024* random number generator
// (http://vigna.di.unimi.it/ftp/papers/xorshift.pdf)
unsigned long long rand_u64()
{
    const unsigned long long s0 = xorshift_state[xorshift_pointer];
    unsigned long long s1 = xorshift_state[xorshift_pointer = ( xorshift_pointer + 1 ) & 15];
    s1 ^= s1 << 31;
    xorshift_state[xorshift_pointer] = s1 ^ s0 ^ ( s1 >> 11 ) ^ ( s0 >> 30 );
    return xorshift_state[xorshift_pointer] * 1181783497276652981ULL;
}

long long rand_s64()
{
    return rand_u64() & 0x7FFFFFFFFFFFFFFFLL;
}

int rand_s32()
{
    return rand_u64() & 0x7FFFFFFF;
}

void init_xorshift_rng( unsigned long long x )
{
    // use splitmix64 algorithm to feed the xorshift's 1024bit state
    for( int i = 0; i < 16; i++ ) {
        unsigned long long z = ( x += 0x9E3779B97F4A7C15ULL );
        z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
        z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBULL;
        xorshift_state[i] = z ^ ( z >> 31 );
    }
    xorshift_pointer = 0;
}

long rng( long val1, long val2 )
{
    if( val1 > val2 ) {
        std::swap( val1, val2 );
    }
    return val1 + long( ( val2 - val1 + 1 ) * double( rand_u64() / double(
                            UINT64_MAX + 1.0 ) ) );
}

double rng_float( double val1, double val2 )
{
    if( val1 > val2 ) {
        std::swap( val1, val2 );
    }
    return val1 + ( val2 - val1 ) * double( rand_u64() ) / double( UINT64_MAX + 1.0 );
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
    return ( ( double )rand_u64() / UINT64_MAX ) <= ( ( double )x / y );
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
    while( c != '\0' ) {
        hash = ( hash ^ c ) * fnv_prime;
        c = *str++;
    }
    return hash;
}
