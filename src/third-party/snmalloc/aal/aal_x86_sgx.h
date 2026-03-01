#pragma once

#ifdef _MSC_VER
#  include <immintrin.h>
#  include <intrin.h>
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
   * x86-specific architecture abstraction layer minimised for use
   * inside SGX enclaves.
   */
  class AAL_x86_sgx
  {
  public:
    /**
     * Bitmap of AalFeature flags
     */
    static constexpr uint64_t aal_features = IntegerPointers;

    static constexpr enum AalName aal_name = X86_SGX;

    static constexpr size_t smallest_page_size = 0x1000;

    /**
     * On pipelined processors, notify the core that we are in a spin loop and
     * that speculative execution past this point may not be a performance gain.
     */
    static inline void pause()
    {
#ifdef _MSC_VER
      _mm_pause();
#else
      asm volatile("pause");
#endif
    }

    /**
     * Issue a prefetch hint at the specified address.
     */
    static inline void prefetch(void* ptr)
    {
#ifdef _MSC_VER
      _mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_T0);
#else
      asm volatile("prefetcht0 %0" ::"m"(ptr));
#endif
    }

    /**
     * Return a cycle counter value.
     * Not guaranteed inside an enclave, so just always return 0.
     * This is only used for benchmarking inside snmalloc.
     */
    static inline uint64_t tick()
    {
      return 0;
    }
  };

  using AAL_Arch = AAL_x86_sgx;
} // namespace snmalloc
