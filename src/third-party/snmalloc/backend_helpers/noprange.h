#pragma once
#include "range_helpers.h"

namespace snmalloc
{
  struct NopRange
  {
    template<typename ParentRange>
    class Type : public ContainsParent<ParentRange>
    {
      using ContainsParent<ParentRange>::parent;

    public:
      static constexpr bool Aligned = ParentRange::Aligned;

      static constexpr bool ConcurrencySafe = ParentRange::ConcurrencySafe;

      using ChunkBounds = typename ParentRange::ChunkBounds;
      static_assert(
        ChunkBounds::address_space_control ==
        capptr::dimension::AddressSpaceControl::Full);

      constexpr Type() = default;

      CapPtr<void, ChunkBounds> alloc_range(size_t size)
      {
        return parent.alloc_range(size);
      }

      void dealloc_range(CapPtr<void, ChunkBounds> base, size_t size)
      {
        parent.dealloc_range(base, size);
      }
    };
  };
} // namespace snmalloc
