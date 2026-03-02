#pragma once
#include "../pal/pal.h"
#include "empty_range.h"
#include "range_helpers.h"

namespace snmalloc
{
  template<typename OptionalRange>
  struct StaticConditionalRange
  {
    // This is a range that can bypass the OptionalRange if it is disabled.
    // Disabling is global, and not local.
    // This is used to allow disabling thread local buddy allocators when the
    // initial fixed size heap is small.
    //
    // The range builds a more complex parent
    //    Pipe<ParentRange, OptionalRange>
    // and uses the ancestor functions to bypass the OptionalRange if the flag
    // has been set.
    template<typename ParentRange>
    class Type : public ContainsParent<Pipe<ParentRange, OptionalRange>>
    {
      // This contains connects the optional range to the parent range.
      using ActualParentRange = Pipe<ParentRange, OptionalRange>;

      using ContainsParent<ActualParentRange>::parent;

      // Global flag specifying if the optional range should be disabled.
      static inline bool disable_range_{false};

    public:
      // Both parent and grandparent must be aligned for this range to be
      // aligned.
      static constexpr bool Aligned =
        ActualParentRange::Aligned && ParentRange::Aligned;

      // Both parent and grandparent must be aligned for this range to be
      // concurrency safe.
      static constexpr bool ConcurrencySafe =
        ActualParentRange::ConcurrencySafe && ParentRange::ConcurrencySafe;

      using ChunkBounds = typename ActualParentRange::ChunkBounds;

      static_assert(
        stl::is_same_v<ChunkBounds, typename ParentRange::ChunkBounds>,
        "Grandparent and optional parent range chunk bounds must be equal");

      constexpr Type() = default;

      CapPtr<void, ChunkBounds> alloc_range(size_t size)
      {
        if (disable_range_)
        {
          // Use ancestor to bypass the optional range.
          return this->template ancestor<ParentRange>()->alloc_range(size);
        }

        return parent.alloc_range(size);
      }

      void dealloc_range(CapPtr<void, ChunkBounds> base, size_t size)
      {
        if (disable_range_)
        {
          // Use ancestor to bypass the optional range.
          this->template ancestor<ParentRange>()->dealloc_range(base, size);
          return;
        }
        parent.dealloc_range(base, size);
      }

      static void disable_range()
      {
        disable_range_ = true;
      }
    };
  };
} // namespace snmalloc
