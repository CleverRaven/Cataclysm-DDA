#pragma once
#ifndef CATA_TESTS_COLONY_LIST_TEST_HELPERS_H
#define CATA_TESTS_COLONY_LIST_TEST_HELPERS_H

// Fast xorshift+128 random number generator function
// original: https://codingforspeed.com/using-faster-psudo-random-generator-xorshift/
static unsigned int xor_rand()
{
    static unsigned int x = 123456789;
    static unsigned int y = 362436069;
    static unsigned int z = 521288629;
    static unsigned int w = 88675123;

    const unsigned int t = x ^ ( x << 11 );

    // Rotate the static values (w rotation in return statement):
    x = y;
    y = z;
    z = w;

    return w = w ^ ( w >> 19 ) ^ ( t ^ ( t >> 8 ) );
}

struct small_struct {
    double *empty_field_1;
    double unused_number;
    unsigned int empty_field2;
    double *empty_field_3;
    int number;
    unsigned int empty_field4;

    small_struct( const int num ) noexcept: number( num ) {}
};

struct perfect_forwarding_test {
    const bool success;

    perfect_forwarding_test( int && /*perfect1*/, int &perfect2 ) : success( true ) {
        perfect2 = 1;
    }

    template <typename T, typename U>
    perfect_forwarding_test( T && /*imperfect1*/, U && /*imperfect2*/ ) : success( false )
    {}
};

#endif // CATA_TESTS_COLONY_LIST_TEST_HELPERS_H
