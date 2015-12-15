#include "output.h"
#include "rng.h"
#include "random_well512a.h"
#include <ctime>
#include <random>

well512a_engine *prng_engine;

void init_rng() {
    if( prng_engine == nullptr ) {
        seed_rng( time( nullptr ) );
    }
}

void seed_rng( int seed )
{
    if( prng_engine == nullptr ) {
        prng_engine = new well512a_engine( seed );
    } else {
        delete prng_engine;
        prng_engine = new well512a_engine( seed );
    }
}

long rng(long val1, long val2)
{
    init_rng();
    return std::uniform_int_distribution<long>{val1, val2}( *prng_engine );
}

double rng_float(double val1, double val2)
{
    init_rng();
    return std::uniform_real_distribution<double>{val1, val2}( *prng_engine );
}

unsigned rng_unsigned()
{
    init_rng();
    return ( *prng_engine )();
}

bool one_in(int chance)
{
    return (chance <= 1 || rng(0, chance - 1) == 0);
}

//this works just like one_in, but it accepts doubles as input to calculate chances like "1 in 350,52"
bool one_in_improved(double chance)
{
    return (chance <= 1 || rng_float(0, chance) < 1);
}

bool x_in_y(double x, double y)
{
    return ( y <= x || rng_float( 0, y ) <= x );
}

int dice(int number, int sides)
{
    int ret = 0;
    for (int i = 0; i < number; i++) {
        ret += rng(1, sides);
    }
    return ret;
}

int divide_roll_remainder( double dividend, double divisor )
{
    const double div = dividend / divisor;
    const int trunc = int(div);
    if( div > trunc && x_in_y( div - trunc, 1.0 ) ) {
        return trunc + 1;
    }

    return trunc;
}


// http://www.cse.yorku.ca/~oz/hash.html
// for world seeding.
int djb2_hash(const unsigned char *str)
{
    unsigned long hash = 5381;
    unsigned char c = *str++;
    while (c != '\0') {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        c = *str++;
    }
    return hash;
}

