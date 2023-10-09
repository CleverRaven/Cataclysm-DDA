#pragma once
#ifndef CATA_TESTS_COLONY_LIST_TEST_HELPERS_H
#define CATA_TESTS_COLONY_LIST_TEST_HELPERS_H

unsigned int xor_rand();

struct small_struct {
    double *empty_field_1;
    double unused_number;
    unsigned int empty_field2;
    double *empty_field_3;
    int number;
    unsigned int empty_field4;

    explicit small_struct( const int num ) noexcept: number( num ) {}
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
