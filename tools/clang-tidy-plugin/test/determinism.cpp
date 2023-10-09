// RUN: %check_clang_tidy -allow-stdinc %s cata-determinism %t -- --load=%cata_plugin -- -isystem %cata_include

#include <random>

#include "rng.h"

struct SomeOtherStruct {
    int i;
    void foo();
};

struct StructWithSeedButNotResultType {
    void seed();
};

using SomeOtherAlias = SomeOtherStruct;

namespace other
{
int rand();
}

void f()
{
    std::mt19937 gen0;
    // CHECK-MESSAGES: [[@LINE-1]]:18: warning: Construction of library random engine 'std::mt19937' (aka 'mersenne_twister_engine<unsigned {{(int|long)}}, 32, 624, 397, 31, 2567483615UL, 11, 4294967295UL, 7, 2636928640UL, 15, 4022730752UL, 18, 1812433253UL>').  To ensure determinism for a fixed seed, use the common tools from rng.h rather than your own random number engines. [cata-determinism]
    cata_default_random_engine gen1( 0 );
    // CHECK-MESSAGES: [[@LINE-1]]:32: warning: Construction of library random engine 'cata_default_random_engine' (aka 'linear_congruential_engine<unsigned {{(int|long)}}, 16807UL, 0UL, 2147483647UL>').  To ensure determinism for a fixed seed, use the common tools from rng.h rather than your own random number engines. [cata-determinism]
    SomeOtherStruct s0;
    StructWithSeedButNotResultType s1;
    SomeOtherAlias a;
    rand();
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Call to library random function 'rand'.  To ensure determinism for a fixed seed, use the common tools from rng.h rather than calling library random functions. [cata-determinism]
    std::rand();
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Call to library random function 'rand'.  To ensure determinism for a fixed seed, use the common tools from rng.h rather than calling library random functions. [cata-determinism]
    other::rand();
}
