#pragma once
#include "../mem/mem.h"
#include "empty_range.h"

namespace snmalloc
{
  /**
   * Creates an area inside a large allocation that is larger by
   * 2^RATIO_BITS.  Will not return a the block at the start or
   * the end of the large allocation.
   */
  template<typename PAL, size_t RATIO_BITS>
  struct SubRange
  {
    template<typename ParentRange = EmptyRange<>>
    class Type : public ContainsParent<ParentRange>
    {
      using ContainsParent<ParentRange>::parent;

    public:
      constexpr Type() = default;

      static constexpr bool Aligned = ParentRange::Aligned;

      static constexpr bool ConcurrencySafe = ParentRange::ConcurrencySafe;

      using ChunkBounds = typename ParentRange::ChunkBounds;

      CapPtr<void, ChunkBounds> alloc_range(size_t sub_size)
      {
        SNMALLOC_ASSERT(bits::is_pow2(sub_size));

        auto full_size = sub_size << RATIO_BITS;
        auto overblock = parent.alloc_range(full_size);
        if (overblock == nullptr)
          return nullptr;

        size_t offset_mask = full_size - sub_size;
        // Don't use first or last block in the larger reservation
        // Loop required to get uniform distribution.
        size_t offset;
        do
        {
          offset = get_entropy64<PAL>() & offset_mask;
        } while ((offset == 0) || (offset == offset_mask));

        return pointer_offset(overblock, offset);
      }
    };
  };
} // namespace snmalloc
