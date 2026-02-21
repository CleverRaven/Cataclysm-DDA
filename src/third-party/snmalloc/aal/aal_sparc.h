#pragma once

#if defined(__arch64__) // More reliable than __sparc64__
#  define SNMALLOC_VA_BITS_64
#else
#  define SNMALLOC_VA_BITS_32
#endif

namespace snmalloc
{
  /**
   * Sparc architecture abstraction layer.
   */
  class AAL_Sparc
  {
  public:
    /**
     * Bitmap of AalFeature flags
     */
    static constexpr uint64_t aal_features = IntegerPointers;

    static constexpr enum AalName aal_name = Sparc;

#ifdef SNMALLOC_VA_BITS_64
    /**
     * Even Ultra-Sparc I supports 8192 and onwards
     */
    static constexpr size_t smallest_page_size = 0x2000;
#else
    static constexpr size_t smallest_page_size = 0x1000;
#endif

    /**
     * On Sparc ideally pause instructions ought to be
     * optimised per Sparc processor but here a version
     * as least common denominator to avoid numerous ifdef,
     * reading Conditions Code Register here
     */
    static inline void pause()
    {
      __asm__ volatile("rd %%ccr, %%g0" ::: "memory");
    }

    static inline void prefetch(void* ptr)
    {
#ifdef SNMALLOC_VA_BITS_64
      __asm__ volatile("prefetch [%0], 0" ::"r"(ptr));
#else
      UNUSED(ptr);
#endif
    }

    static inline uint64_t tick()
    {
      uint64_t tick;
      __asm__ volatile("rd %%asr4, %0" : "=r"(tick));
      return tick;
    }
  };

  using AAL_Arch = AAL_Sparc;
} // namespace snmalloc
