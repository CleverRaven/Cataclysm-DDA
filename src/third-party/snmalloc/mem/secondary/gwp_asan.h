#pragma once

#include "gwp_asan/guarded_pool_allocator.h"
#include "snmalloc/ds_core/defines.h"
#include "snmalloc/mem/sizeclasstable.h"
#if defined(SNMALLOC_BACKTRACE_HEADER)
#  include SNMALLOC_BACKTRACE_HEADER
#endif

namespace snmalloc
{
  class GwpAsanSecondaryAllocator
  {
    static inline gwp_asan::GuardedPoolAllocator singleton;
    static inline size_t max_allocation_size;

  public:
    static constexpr inline bool pass_through = false;

    static void initialize() noexcept
    {
      // for now, we use default options
      gwp_asan::options::Options opt;
      opt.setDefaults();
#ifdef SNMALLOC_BACKTRACE_HEADER
      opt.Backtrace = [](uintptr_t* buf, size_t length) {
        return static_cast<size_t>(
          ::backtrace(reinterpret_cast<void**>(buf), static_cast<int>(length)));
      };
#endif
      singleton.init(opt);
      max_allocation_size =
        singleton.getAllocatorState()->maximumAllocationSize();
    }

    // Use thunk to avoid extra computation when allocation decision can be made
    // before size and alignment are computed.
    template<class SizeAlign>
    SNMALLOC_FAST_PATH static void* allocate(SizeAlign&& getter)
    {
      // TODO: this `shouldSample` is only triggered on snmalloc's slowpath,
      // which may reduce the chance of error detection. We may reconsider
      // the logic to improve the precision in future commits.
      if (SNMALLOC_UNLIKELY(singleton.shouldSample()))
      {
        auto [size, align] = getter();
        if (size > max_allocation_size)
          return nullptr;
        return singleton.allocate(size, align);
      }
      return nullptr;
    }

    SNMALLOC_FAST_PATH
    static void deallocate(void* pointer)
    {
      snmalloc_check_client(
        mitigations(sanity_checks),
        singleton.pointerIsMine(pointer),
        "Not allocated by snmalloc or secondary allocator");

      singleton.deallocate(pointer);
    }

    SNMALLOC_FAST_PATH
    static size_t alloc_size(const void* pointer)
    {
      return singleton.getSize(pointer);
    }
  };
} // namespace snmalloc
