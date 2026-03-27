#pragma once

#ifdef _MSC_VER
#  include <immintrin.h>
#  include <intrin.h>
#else
#  include <cpuid.h>
#  include <emmintrin.h>
#endif

#if defined(__linux__)
#  include <x86intrin.h>
#endif

#if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || \
  defined(_M_AMD64)
#  define SNMALLOC_VA_BITS_64
#else
#  define SNMALLOC_VA_BITS_32
#endif

namespace snmalloc
{
  /**
   * x86-specific architecture abstraction layer.
   */
  class AAL_x86
  {
    /**
     * Read the timestamp counter, guaranteeing that all previous instructions
     * have been retired.
     */
    static inline uint64_t tickp()
    {
      unsigned int aux;
#if defined(_MSC_VER)
      return __rdtscp(&aux);
#else
      return __builtin_ia32_rdtscp(&aux);
#endif
    }

    /**
     * Issue a fully serialising instruction.
     */
    static inline void halt_out_of_order()
    {
#if defined(_MSC_VER)
      int cpu_info[4];
      __cpuid(cpu_info, 0);
#else
      unsigned int eax, ebx, ecx, edx;
      __get_cpuid(0, &eax, &ebx, &ecx, &edx);
#endif
    }

  public:
    /**
     * Bitmap of AalFeature flags
     */
    static constexpr uint64_t aal_features = IntegerPointers;

    static constexpr enum AalName aal_name = X86;

    static constexpr size_t smallest_page_size = 0x1000;

    /**
     * On pipelined processors, notify the core that we are in a spin loop and
     * that speculative execution past this point may not be a performance gain.
     */
    static inline void pause()
    {
      _mm_pause();
    }

    /**
     * Issue a prefetch hint at the specified address.
     */
    static inline void prefetch(void* ptr)
    {
#if defined(_MSC_VER)
      _m_prefetchw(ptr);
#else
      _mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_ET0);
#endif
    }

    /**
     * Return a cycle counter value.
     */
    static inline uint64_t tick()
    {
#if defined(_MSC_VER)
      return __rdtsc();
#else
      return __builtin_ia32_rdtsc();
#endif
    }

    /**
     * Return the cycle counter value that can be used to indicate the start of
     * a benchmark run.  This is responsible for ensuring any serialisation
     * that the pipeline may require.
     */
    static inline uint64_t benchmark_time_start()
    {
      halt_out_of_order();
      return AAL_Generic<AAL_x86>::tick();
    }

    /**
     * Return the cycle counter value that can be used to indicate the end of a
     * benchmark run.  This is responsible for ensuring any serialisation that
     * the pipeline may require.
     */
    static inline uint64_t benchmark_time_end()
    {
      uint64_t t = tickp();
      halt_out_of_order();
      return t;
    }
  };

  using AAL_Arch = AAL_x86;
} // namespace snmalloc
