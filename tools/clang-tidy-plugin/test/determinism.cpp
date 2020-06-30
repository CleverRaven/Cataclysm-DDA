// RUN: %check_clang_tidy %s cata-determinism %t -- -plugins=%cata_plugin --

using size_t = unsigned long;

namespace std
{

using uint_fast32_t = unsigned;

template <

    class UIntType,
    UIntType a,
    UIntType c,
    UIntType m
    > struct linear_congruential_engine
{
    using result_type = UIntType;
    explicit linear_congruential_engine( result_type value );
    void seed( result_type value );
};
using minstd_rand0 = std::linear_congruential_engine<std::uint_fast32_t, 16807, 0, 2147483647>;

template <

    class UIntType,
    size_t w, size_t n, size_t m, size_t r,
    UIntType a, size_t u, UIntType d, size_t s,
    UIntType b, size_t t,
    UIntType c, size_t l, UIntType f
    > struct mersenne_twister_engine
{
    using result_type = UIntType;
    void seed( result_type value );
};
using mt19937 = std::mersenne_twister_engine<std::uint_fast32_t, 32, 624, 397, 31,
      0x9908b0df, 11,
      0xffffffff, 7,
      0x9d2c5680, 15,
      0xefc60000, 18, 1812433253>;
}

struct SomeOtherStruct {
    int i;
    void foo();
};

struct StructWithSeedButNotResultType {
    void seed();
};

using SomeOtherAlias = SomeOtherStruct;

int rand();

using cata_default_random_engine = std::minstd_rand0;

namespace std
{
using ::rand;
}

namespace other
{
int rand();
}

void f()
{
    std::mt19937 gen0;
    // CHECK-MESSAGES: [[@LINE-1]]:18: warning: Construction of library random engine 'std::mt19937' (aka 'mersenne_twister_engine<unsigned int, 32, 624, 397, 31, 2567483615U, 11, 4294967295U, 7, 2636928640U, 15, 4022730752U, 18, 1812433253>').  To ensure determinism for a fixed seed, use the common tools from rng.h rather than your own random number engines. [cata-determinism]
    cata_default_random_engine gen1( 0 );
    // CHECK-MESSAGES: [[@LINE-1]]:32: warning: Construction of library random engine 'cata_default_random_engine' (aka 'linear_congruential_engine<unsigned int, 16807, 0, 2147483647>').  To ensure determinism for a fixed seed, use the common tools from rng.h rather than your own random number engines. [cata-determinism]
    SomeOtherStruct s0;
    StructWithSeedButNotResultType s1;
    SomeOtherAlias a;
    rand();
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Call to library random function 'rand'.  To ensure determinism for a fixed seed, use the common tools from rng.h rather than calling library random functions. [cata-determinism]
    std::rand();
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Call to library random function 'rand'.  To ensure determinism for a fixed seed, use the common tools from rng.h rather than calling library random functions. [cata-determinism]
    other::rand();
}
