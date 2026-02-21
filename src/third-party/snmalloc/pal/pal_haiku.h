#pragma once

#if defined(__HAIKU__)

#  include "pal_posix.h"

#  include <sys/mman.h>

namespace snmalloc
{
  /**
   * Platform abstraction layer for Haiku.  This provides features for this
   * system.
   */
  class PALHaiku : public PALPOSIX<PALHaiku>
  {
  public:
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     *
     */
    static constexpr uint64_t pal_features = PALPOSIX::pal_features | Entropy;

    /**
     * Haiku requires an explicit no-reserve flag in `mmap` to guarantee lazy
     * commit.
     */
    static constexpr int default_mmap_flags = MAP_NORESERVE;

    /**
     * Notify platform that we will not be needing these pages.
     * Haiku does not provide madvise call per say only the posix equivalent.
     */
    static void notify_not_using(void* p, size_t size) noexcept
    {
      SNMALLOC_ASSERT(is_aligned_block<page_size>(p, size));
      posix_madvise(p, size, POSIX_MADV_DONTNEED);
    }
  };
} // namespace snmalloc
#endif
