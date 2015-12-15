// Copyright (c) 2013, Peter Pettersson
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation are those
// of the authors and should not be interpreted as representing official policies,
// either expressed or implied, of the FreeBSD Project.

#ifndef RANDOM_WELL512A_H
#define RANDOM_WELL512A_H

#include <stdlib.h>     // Needed for rand().
#include <memory.h>     // Needed for memcpy().
#include <limits>

class RandomWELL512a
{
public:
    RandomWELL512a(int seed);
    RandomWELL512a(unsigned *seed);

    unsigned GetUnsigned();

    double GetDouble();

private:
    unsigned state[16];
    unsigned index;
};


inline RandomWELL512a::RandomWELL512a(int seed)
    : index(0)
{
    srand(seed);
    for (int i = 0; i < 16; ++i)
        state[i] = rand();
}

inline RandomWELL512a::RandomWELL512a(unsigned *seed)
    : index(0)
{
    memcpy(state, seed, 16 * sizeof(unsigned));
}

inline unsigned RandomWELL512a::GetUnsigned()
{
    #define MUTATE_LEFT(value, shift)           value ^ (value << shift)
    #define MUTATE_RIGHT(value, shift)          value ^ (value >> shift)
    #define MUTATE_LEFT_MIX(value, shift, mix)  value ^ ((value << shift) & mix)

    unsigned index_9  = (index +  9) & 15;
    unsigned index_13 = (index + 13) & 15;
    unsigned index_15 = (index + 15) & 15;

    unsigned state_index    = state[index];
    unsigned state_index_9  = state[index_9];
    unsigned state_index_13 = state[index_13];
    unsigned state_index_15 = state[index_15];

    unsigned z1	= MUTATE_LEFT(state_index, 16);
    z1 ^= MUTATE_LEFT(state_index_13, 15);

    unsigned z2	= MUTATE_RIGHT(state_index_9, 11);

    unsigned result0 = z1 ^ z2;
    state[index] = result0;

    unsigned result1 = MUTATE_LEFT(state_index_15, 2);
    result1 ^= MUTATE_LEFT(z1, 18);;
    result1 ^= z2 << 28;

    result1 ^= MUTATE_LEFT_MIX(result0, 5, 0xda442d24U);

    index = index_15;
    state[index] = result1;
    return result1;

    #undef MUTATE_LEFT
    #undef MUTATE_RIGHT
    #undef MUTATE_LEFT_MIX
}

inline double RandomWELL512a::GetDouble()
{
    const double kToFloat = 2.32830643653869628906e-10;
    return GetUnsigned() * kToFloat;
}

// C++11 conforming uniform random engine
class well512a_engine
{
public:
    well512a_engine( int seed );

    typedef unsigned result_type;
    static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
    static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
    result_type operator()();
private:
    RandomWELL512a prng;
};

inline well512a_engine::well512a_engine( int seed ) : prng( seed ) {}

inline well512a_engine::result_type well512a_engine::operator()()
{
    return prng.GetUnsigned();
}

#endif // RANDOM_WELL512A_H
