#pragma once

#include "snmalloc/ds_core/defines.h"
#include "snmalloc/ds_core/mitigations.h"

#include <stddef.h>

namespace snmalloc
{
  class DefaultSecondaryAllocator
  {
  public:
    // This flag is used to turn off checks on fast paths if the secondary
    // allocator does not own the memory at all.
    static constexpr inline bool pass_through = true;

    SNMALLOC_FAST_PATH
    static void initialize() {}

    template<class SizeAlign>
    SNMALLOC_FAST_PATH static void* allocate(SizeAlign&&)
    {
      return nullptr;
    }

    SNMALLOC_FAST_PATH
    static void deallocate(void* pointer)
    {
      // If pointer is not null, then dealloc has been call on something
      // it shouldn't be called on.
      // TODO: Should this be tested even in the !CHECK_CLIENT case?
      snmalloc_check_client(
        mitigations(sanity_checks),
        pointer == nullptr,
        "Not allocated by snmalloc.");
    }

    SNMALLOC_FAST_PATH
    static size_t alloc_size(const void*)
    {
      SNMALLOC_ASSERT(
        false &&
        "secondary alloc_size should never be invoked with default setup");
      return 0;
    }
  };
} // namespace snmalloc
