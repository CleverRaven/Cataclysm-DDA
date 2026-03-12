#pragma once
#include "../ds_core/ds_core.h"

namespace snmalloc
{
  template<SNMALLOC_CONCEPT(capptr::IsBound) B = capptr::bounds::Arena>
  class EmptyRange
  {
  public:
    static constexpr bool Aligned = true;

    static constexpr bool ConcurrencySafe = true;

    using ChunkBounds = B;

    constexpr EmptyRange() = default;

    CapPtr<void, ChunkBounds> alloc_range(size_t)
    {
      return nullptr;
    }
  };
} // namespace snmalloc
