#pragma once

#include "empty_range.h"
#include "range_helpers.h"
#include "snmalloc/stl/atomic.h"

namespace snmalloc
{
  /**
   * Used to measure memory usage.
   */
  struct StatsRange
  {
    template<typename ParentRange = EmptyRange<>>
    class Type : public ContainsParent<ParentRange>
    {
      using ContainsParent<ParentRange>::parent;

      static inline stl::Atomic<size_t> current_usage{};
      static inline stl::Atomic<size_t> peak_usage{};

    public:
      static constexpr bool Aligned = ParentRange::Aligned;

      static constexpr bool ConcurrencySafe = ParentRange::ConcurrencySafe;

      using ChunkBounds = typename ParentRange::ChunkBounds;

      constexpr Type() = default;

      CapPtr<void, ChunkBounds> alloc_range(size_t size)
      {
        auto result = parent.alloc_range(size);
        if (result != nullptr)
        {
          auto prev = current_usage.fetch_add(size);
          auto curr = peak_usage.load();
          while (curr < prev + size)
          {
            if (peak_usage.compare_exchange_weak(curr, prev + size))
              break;
          }
        }
        return result;
      }

      void dealloc_range(CapPtr<void, ChunkBounds> base, size_t size)
      {
        current_usage -= size;
        parent.dealloc_range(base, size);
      }

      size_t get_current_usage()
      {
        return current_usage.load();
      }

      size_t get_peak_usage()
      {
        return peak_usage.load();
      }
    };
  };

  template<typename StatsR1, typename StatsR2>
  class StatsCombiner
  {
    StatsR1 r1{};
    StatsR2 r2{};

  public:
    size_t get_current_usage()
    {
      return r1.get_current_usage() + r2.get_current_usage();
    }

    size_t get_peak_usage()
    {
      return r1.get_peak_usage() + r2.get_peak_usage();
    }
  };
} // namespace snmalloc
