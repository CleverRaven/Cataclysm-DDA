#pragma once

#if defined(__sun)
#  include "pal_posix.h"

namespace snmalloc
{
  /**
   * Platform abstraction layer for Solaris.  This provides features for this
   * system.
   */
  class PALSolaris : public PALPOSIX<PALSolaris>
  {
  public:
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     *
     */
    static constexpr uint64_t pal_features =
      AlignedAllocation | PALPOSIX::pal_features;

    static constexpr size_t page_size =
      Aal::aal_name == Sparc ? 0x2000 : 0x1000;
    static constexpr size_t minimum_alloc_size = page_size;

    /**
     * Solaris requires an explicit no-reserve flag in `mmap` to guarantee lazy
     * commit.
     */
    static constexpr int default_mmap_flags = MAP_NORESERVE;

    /**
     * Reserve memory at a specific alignment.
     */
    template<bool state_using>
    static void* reserve_aligned(size_t size) noexcept
    {
      // Alignment must be a power of 2.
      SNMALLOC_ASSERT(bits::is_pow2(size));
      SNMALLOC_ASSERT(size >= minimum_alloc_size);
      UNUSED(state_using);

      uintptr_t alignment =
        static_cast<uintptr_t>(bits::align_up(size, page_size));

      auto prot =
        !mitigations(pal_enforce_access) ? PROT_READ | PROT_WRITE : PROT_NONE;

      void* p = mmap(
        (void*)alignment,
        size,
        prot,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_ALIGN | default_mmap_flags,
        -1,
        0);

      if (p == MAP_FAILED)
        return nullptr;

      return p;
    }
  };
} // namespace snmalloc
#endif
