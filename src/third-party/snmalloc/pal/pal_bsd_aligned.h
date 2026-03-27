#pragma once

#include "pal_bsd.h"

namespace snmalloc
{
  /**
   * FreeBSD-specific platform abstraction layer.
   *
   * This adds aligned allocation using `MAP_ALIGNED` to the generic BSD
   * implementation.  This flag is supported by NetBSD and FreeBSD.
   */
  template<class OS, auto... Args>
  class PALBSD_Aligned : public PALBSD<OS, Args...>
  {
  public:
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     *
     * This class adds support for aligned allocation.
     */
    static constexpr uint64_t pal_features =
      AlignedAllocation | PALBSD<OS>::pal_features;

    static SNMALLOC_CONSTINIT_STATIC size_t minimum_alloc_size =
      aal_supports<StrictProvenance> ? 1 << 24 : 4096;

    /**
     * Reserve memory at a specific alignment.
     */
    template<bool state_using>
    static void* reserve_aligned(size_t size) noexcept
    {
      // Alignment must be a power of 2.
      SNMALLOC_ASSERT(bits::is_pow2(size));
      SNMALLOC_ASSERT(size >= minimum_alloc_size);

      int log2align = static_cast<int>(bits::next_pow2_bits(size));

      auto prot = state_using || !mitigations(pal_enforce_access) ?
        PROT_READ | PROT_WRITE :
        PROT_NONE;

      void* p = mmap(
        nullptr,
        size,
        prot,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_ALIGNED(log2align) |
          OS::extra_mmap_flags(state_using),
        -1,
        0);

      if (p == MAP_FAILED)
        return nullptr;

      return p;
    }
  };
} // namespace snmalloc
