#pragma once

#include "../mem/metadata.h"
#include "../pal/pal.h"
#include "empty_range.h"
#include "range_helpers.h"

namespace snmalloc
{
  template<SNMALLOC_CONCEPT(IsPagemapWithRegister) Pagemap>
  struct PagemapRegisterRange
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

      CapPtr<void, ChunkBounds> alloc_range(size_t size)
      {
        auto base = parent.alloc_range(size);

        if (base != nullptr)
        {
          auto result = Pagemap::register_range(base, size);
          if (!result)
          {
            // If register_range fails, typically there is no recovery from this
            // so just return nullptr.
            return CapPtr<void, ChunkBounds>(nullptr);
          }
        }

        return base;
      }
    };
  };
} // namespace snmalloc
