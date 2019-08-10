#pragma once
#ifndef CATA_TUPLE_HASH_H
#define CATA_TUPLE_HASH_H

// Support for tuple and pair hashing.
// This is taken almost directly from the boost library code.
// Function has to live in the std namespace
// so that it is picked up by argument-dependent name lookup (ADL).
namespace cata
{
namespace
{

// Code from boost
// Reciprocal of the golden ratio helps spread entropy
//     and handles duplicates.
// See Mike Seymour in magic-numbers-in-boosthash-combine:
//     http://stackoverflow.com/questions/4948780

template <class T>
inline void hash_combine( std::size_t &seed, const T &v )
{
    seed ^= std::hash<T>()( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

// Recursive template code derived from Matthieu M.
template < class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1 >
struct HashValueImpl
{
    static void apply( size_t &seed, const Tuple &tuple ) {
        HashValueImpl < Tuple, Index - 1 >::apply( seed, tuple );
        hash_combine( seed, std::get<Index>( tuple ) );
    }
};

template <class Tuple>
struct HashValueImpl<Tuple, 0> {
    static void apply( size_t &seed, const Tuple &tuple ) {
        hash_combine( seed, std::get<0>( tuple ) );
    }
};
} // namespace

struct tuple_hash {
    template <typename ... TT>
    std::size_t operator()( const std::tuple<TT...> &tt ) const {
        size_t seed = 0;
        HashValueImpl<std::tuple<TT...> >::apply( seed, tt );
        return seed;
    }

    template <class A, class B>
    std::size_t operator()( const std::pair<A, B> &v ) const {
        std::size_t seed = 0;
        hash_combine( seed, v.first );
        hash_combine( seed, v.second );
        return seed;
    }
};

} // namespace cata

#endif // CATA_TUPLE_HASH_H
