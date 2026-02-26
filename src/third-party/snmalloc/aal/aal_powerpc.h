#pragma once

#if defined(__powerpc64__)
#  define SNMALLOC_VA_BITS_64
#else
#  define SNMALLOC_VA_BITS_32
#endif

namespace snmalloc
{
  /**
   * ARM-specific architecture abstraction layer.
   */
  class AAL_PowerPC
  {
  public:
    /**
     * Bitmap of AalFeature flags
     */
    static constexpr uint64_t aal_features = IntegerPointers;

    static constexpr enum AalName aal_name = PowerPC;

    static constexpr size_t smallest_page_size = 0x1000;

    /**
     * On pipelined processors, notify the core that we are in a spin loop and
     * that speculative execution past this point may not be a performance gain.
     */
    static inline void pause()
    {
      __asm__ volatile("or 27,27,27"); // "yield"
    }
  };

  using AAL_Arch = AAL_PowerPC;
} // namespace snmalloc
