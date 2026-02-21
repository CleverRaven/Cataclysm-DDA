#pragma once

#include "empty_range.h"
#include "range_helpers.h"

namespace snmalloc
{
  /**
   * RangeName is an integer to specify which range is being logged. Strings can
   * be used as template parameters.
   *
   * ParentRange is what the range is logging calls to.
   */
  template<size_t RangeName>
  struct LogRange
  {
    template<typename ParentRange = EmptyRange<>>
    class Type : public ContainsParent<ParentRange>
    {
      using ContainsParent<ParentRange>::parent;

    public:
      static constexpr bool Aligned = ParentRange::Aligned;

      static constexpr bool ConcurrencySafe = ParentRange::ConcurrencySafe;

      using ChunkBounds = typename ParentRange::ChunkBounds;

      constexpr Type() = default;

      CapPtr<void, ChunkBounds> alloc_range(size_t size)
      {
#ifdef SNMALLOC_TRACING
        message<1024>("Call alloc_range({}) on {}", size, RangeName);
#endif
        auto range = parent.alloc_range(size);
#ifdef SNMALLOC_TRACING
        message<1024>(
          "{} = alloc_range({}) in {}", range.unsafe_ptr(), size, RangeName);
#endif
        return range;
      }

      void dealloc_range(CapPtr<void, ChunkBounds> base, size_t size)
      {
#ifdef SNMALLOC_TRACING
        message<1024>(
          "dealloc_range({}, {}}) on {}", base.unsafe_ptr(), size, RangeName);
#endif
        parent.dealloc_range(base, size);
#ifdef SNMALLOC_TRACING
        message<1024>(
          "Done dealloc_range({}, {}})! on {}",
          base.unsafe_ptr(),
          size,
          RangeName);
#endif
      }
    };
  };
} // namespace snmalloc
