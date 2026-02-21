#pragma once

#include "pal_noalloc.h"

#ifdef OPEN_ENCLAVE
extern "C" void* oe_memset_s(void* p, size_t p_size, int c, size_t size);
extern "C" int oe_random(void* data, size_t size);
extern "C" [[noreturn]] void oe_abort();

namespace snmalloc
{
  class OpenEnclaveErrorHandler
  {
  public:
    static void print_stack_trace() {}

    [[noreturn]] static void error(const char* const str)
    {
      UNUSED(str);
      oe_abort();
    }

    static constexpr size_t address_bits = Aal::address_bits;
    static constexpr size_t page_size = Aal::smallest_page_size;
  };

  using OpenEnclaveBasePAL = PALNoAlloc<OpenEnclaveErrorHandler>;

  class PALOpenEnclave : public OpenEnclaveBasePAL
  {
  public:
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     */
    static constexpr uint64_t pal_features =
      OpenEnclaveBasePAL::pal_features | Entropy;

    template<bool page_aligned = false>
    static void zero(void* p, size_t size) noexcept
    {
      oe_memset_s(p, size, 0, size);
    }

    /**
     * Source of Entropy
     */
    static uint64_t get_entropy64()
    {
      uint64_t result = 0;
      if (oe_random(&result, sizeof(result)) != OE_OK)
        error("Failed to get system randomness");
      return result;
    }
  };
}
#endif
