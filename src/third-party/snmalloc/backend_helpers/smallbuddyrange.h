#pragma once

#include "../pal/pal.h"
#include "empty_range.h"
#include "range_helpers.h"

namespace snmalloc
{
  /**
   * struct for representing the redblack nodes
   * directly inside the meta data.
   */
  template<SNMALLOC_CONCEPT(capptr::IsBound) bounds>
  struct FreeChunk
  {
    CapPtr<FreeChunk, bounds> left;
    CapPtr<FreeChunk, bounds> right;
  };

  /**
   * Class for using the allocations own space to store in the RBTree.
   */
  template<SNMALLOC_CONCEPT(capptr::IsBound) bounds>
  class BuddyInplaceRep
  {
  public:
    using Handle = CapPtr<FreeChunk<bounds>, bounds>*;
    using Contents = CapPtr<FreeChunk<bounds>, bounds>;

    static constexpr Contents null = nullptr;
    static constexpr Contents root = nullptr;

    static constexpr address_t MASK = 1;

    static void set(Handle ptr, Contents r)
    {
      SNMALLOC_ASSERT((address_cast(r) & MASK) == 0);
      if (r == nullptr)
        *ptr = CapPtr<FreeChunk<bounds>, bounds>::unsafe_from(
          reinterpret_cast<FreeChunk<bounds>*>((*ptr).unsafe_uintptr() & MASK));
      else
        // Preserve lower bit.
        *ptr = pointer_offset(r, (address_cast(*ptr) & MASK))
                 .template as_static<FreeChunk<bounds>>();
    }

    static Contents get(Handle ptr)
    {
      return pointer_align_down<2, FreeChunk<bounds>>((*ptr).as_void());
    }

    static Handle ref(bool direction, Contents r)
    {
      if (direction)
        return &r->left;

      return &r->right;
    }

    static bool is_red(Contents k)
    {
      if (k == nullptr)
        return false;
      return (address_cast(*ref(false, k)) & MASK) == MASK;
    }

    static void set_red(Contents k, bool new_is_red)
    {
      if (new_is_red != is_red(k))
      {
        auto r = ref(false, k);
        auto old_addr = pointer_align_down<2, FreeChunk<bounds>>(r->as_void());

        if (new_is_red)
        {
          if (old_addr == nullptr)
            *r = CapPtr<FreeChunk<bounds>, bounds>::unsafe_from(
              reinterpret_cast<FreeChunk<bounds>*>(MASK));
          else
            *r = pointer_offset(old_addr, MASK)
                   .template as_static<FreeChunk<bounds>>();
        }
        else
        {
          *r = old_addr;
        }
        SNMALLOC_ASSERT(is_red(k) == new_is_red);
      }
    }

    static Contents offset(Contents k, size_t size)
    {
      return pointer_offset(k, size).template as_static<FreeChunk<bounds>>();
    }

    static Contents buddy(Contents k, size_t size)
    {
      // This is just doing xor size, but with what API
      // exists on capptr.
      auto base = pointer_align_down<FreeChunk<bounds>>(k.as_void(), size * 2);
      auto offset = (address_cast(k) & size) ^ size;
      return pointer_offset(base, offset)
        .template as_static<FreeChunk<bounds>>();
    }

    static Contents align_down(Contents k, size_t size)
    {
      return pointer_align_down<FreeChunk<bounds>>(k.as_void(), size);
    }

    static bool compare(Contents k1, Contents k2)
    {
      return address_cast(k1) > address_cast(k2);
    }

    static bool equal(Contents k1, Contents k2)
    {
      return address_cast(k1) == address_cast(k2);
    }

    static address_t printable(Contents k)
    {
      return address_cast(k);
    }

    /**
     * Return the holder in some format suitable for printing by snmalloc's
     * debug log mechanism.  Used only when used in tracing mode, not normal
     * debug or release builds. Raw pointers are printable already, so this is
     * the identity function.
     */
    static Handle printable(Handle k)
    {
      return k;
    }

    /**
     * Return a name for use in tracing mode.  Unused in any other context.
     */
    static const char* name()
    {
      return "BuddyInplaceRep";
    }

    static bool can_consolidate(Contents k, size_t size)
    {
      UNUSED(k, size);
      return true;
    }
  };

  struct SmallBuddyRange
  {
    template<typename ParentRange = EmptyRange<>>
    class Type : public ContainsParent<ParentRange>
    {
    public:
      using ChunkBounds = typename ParentRange::ChunkBounds;

    private:
      using ContainsParent<ParentRange>::parent;

      static constexpr size_t MIN_BITS =
        bits::next_pow2_bits_const(sizeof(FreeChunk<ChunkBounds>));

      Buddy<BuddyInplaceRep<ChunkBounds>, MIN_BITS, MIN_CHUNK_BITS> buddy_small;

      /**
       * Add a range of memory to the address space.
       * Divides blocks into power of two sizes with natural alignment
       */
      void add_range(CapPtr<void, ChunkBounds> base, size_t length)
      {
        range_to_pow_2_blocks<MIN_BITS>(
          base,
          length,
          [this](CapPtr<void, ChunkBounds> base, size_t align, bool) {
            if (align < MIN_CHUNK_SIZE)
            {
              CapPtr<void, ChunkBounds> overflow =
                buddy_small
                  .add_block(
                    base.template as_reinterpret<FreeChunk<ChunkBounds>>(),
                    align)
                  .template as_reinterpret<void>();
              if (overflow != nullptr)
                parent.dealloc_range(
                  overflow, bits::one_at_bit(MIN_CHUNK_BITS));
            }
            else
            {
              parent.dealloc_range(base, align);
            }
          });
      }

      CapPtr<void, ChunkBounds> refill(size_t size)
      {
        auto refill = parent.alloc_range(MIN_CHUNK_SIZE);

        if (refill != nullptr)
          add_range(pointer_offset(refill, size), MIN_CHUNK_SIZE - size);

        return refill;
      }

    public:
      static constexpr bool Aligned = true;
      static_assert(ParentRange::Aligned, "ParentRange must be aligned");

      static constexpr bool ConcurrencySafe = false;

      constexpr Type() = default;

      CapPtr<void, ChunkBounds> alloc_range(size_t size)
      {
        if (size >= MIN_CHUNK_SIZE)
          return parent.alloc_range(size);

        auto result = buddy_small.remove_block(size);
        if (result != nullptr)
        {
          result->left = nullptr;
          result->right = nullptr;
          return result.template as_reinterpret<void>();
        }
        return refill(size);
      }

      CapPtr<void, ChunkBounds> alloc_range_with_leftover(size_t size)
      {
        auto rsize = bits::next_pow2(size);

        auto result = alloc_range(rsize);

        if (result == nullptr)
          return nullptr;

        auto remnant = pointer_offset(result, size);

        add_range(remnant, rsize - size);

        return result.template as_reinterpret<void>();
      }

      void dealloc_range(CapPtr<void, ChunkBounds> base, size_t size)
      {
        add_range(base, size);
      }
    };
  };
} // namespace snmalloc
