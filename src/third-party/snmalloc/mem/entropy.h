#pragma once

#include "../ds/ds.h"
#include "../pal/pal.h"
#include "snmalloc/stl/type_traits.h"

#include <stdint.h>

namespace snmalloc
{
  struct FreeListKey
  {
    address_t key1;
    address_t key2;
    address_t key_next;

    constexpr FreeListKey(uint64_t key1, uint64_t key2, uint64_t key_next)
    : key1(static_cast<address_t>(key1)),
      key2(static_cast<address_t>(key2)),
      key_next(static_cast<address_t>(key_next))
    {}
  };

  class LocalEntropy
  {
    uint64_t bit_source{0};
    uint64_t local_key{0};
    uint64_t local_counter{0};
    uint64_t fresh_bits{0};
    uint64_t count{0};

  public:
    constexpr LocalEntropy() = default;

    template<typename PAL>
    void init()
    {
      local_key = get_entropy64<PAL>();
      local_counter = get_entropy64<PAL>();
      bit_source = get_next();
    }

    /**
     * Returns a bit.
     *
     * The bit returned is cycled every 64 calls.
     * This is a very cheap source of some randomness.
     * Returns the bottom bit.
     */
    uint32_t next_bit()
    {
      uint64_t bottom_bit = bit_source & 1;
      bit_source = (bottom_bit << 63) | (bit_source >> 1);
      return bit_source & 1;
    }

    /**
     * A key for the free lists for this thread.
     */
    void make_free_list_key(FreeListKey& key)
    {
      if constexpr (bits::BITS == 64)
      {
        key.key1 = static_cast<address_t>(get_next());
        key.key2 = static_cast<address_t>(get_next());
        key.key_next = static_cast<address_t>(get_next());
      }
      else
      {
        key.key1 = static_cast<address_t>(get_next() & 0xffff'ffff);
        key.key2 = static_cast<address_t>(get_next() & 0xffff'ffff);
        key.key_next = static_cast<address_t>(get_next() & 0xffff'ffff);
      }
    }

    /**
     * Source of random 64bit values
     *
     * Has a 2^64 period.
     *
     * Applies a Feistel cipher to a counter
     */
    uint64_t get_next()
    {
      uint64_t c = ++local_counter;
      uint64_t bottom;
      for (int i = 0; i < 2; i++)
      {
        bottom = c & 0xffff'fffff;
        c = (c << 32) | (((bottom * local_key) ^ c) >> 32);
      }
      return c;
    }

    /**
     * Refresh `next_bit` source of bits.
     *
     * This loads new entropy into the `next_bit` values.
     */
    void refresh_bits()
    {
      bit_source = get_next();
    }

    /**
     * Pseudo random bit source.
     *
     * Does not cycle as frequently as `next_bit`.
     */
    uint16_t next_fresh_bits(size_t n)
    {
      if (count <= n)
      {
        fresh_bits = get_next();
        count = 64;
      }
      uint16_t result = static_cast<uint16_t>(fresh_bits & bits::mask_bits(n));
      fresh_bits >>= n;
      count -= n;
      return result;
    }

    /***
     * Approximation of a uniform distribution
     *
     * Biases high numbers. A proper uniform distribution
     * was too expensive.  This maps a uniform distribution
     * over the next power of two (2^m), and for numbers that
     * are drawn larger then n-1, they are mapped onto uniform
     * top range of n.
     */
    uint16_t sample(uint16_t n)
    {
      size_t i = bits::next_pow2_bits(n);
      uint16_t bits = next_fresh_bits(i);
      uint16_t result = bits;
      // Put over flowing bits at the top.
      if (bits >= n)
        result = n - (1 + bits - n);
      return result;
    }
  };
} // namespace snmalloc
