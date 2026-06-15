#pragma once

#include "../ds_core/ds_core.h"

#if defined(FreeBSD_KERNEL)
extern "C"
{
#  include <sys/vmem.h>
#  include <vm/vm.h>
#  include <vm/vm_extern.h>
#  include <vm/vm_kern.h>
#  include <vm/vm_object.h>
#  include <vm/vm_param.h>
}

namespace snmalloc
{
  class PALFreeBSDKernel
  {
    vm_offset_t get_vm_offset(uint_ptr_t p)
    {
      return static_cast<vm_offset_t>(reinterpret_cast<uintptr_t>(p));
    }

  public:
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     */
    static constexpr uint64_t pal_features = AlignedAllocation;

    /**
     * Report a message to console, followed by a newline.
     */
    static void message(const char* const str) noexcept
    {
      printf("%s\n", str);
    }

    [[noreturn]] void error(const char* const str)
    {
      panic("snmalloc error: %s", str);
    }

    /// Notify platform that we will not be using these pages
    static void notify_not_using(void* p, size_t size)
    {
      vm_offset_t addr = get_vm_offset(p);
      kmem_unback(kernel_object, addr, size);
    }

    /// Notify platform that we will be using these pages
    template<ZeroMem zero_mem>
    static bool notify_using(void* p, size_t size)
    {
      vm_offset_t addr = get_vm_offset(p);
      int flags = M_WAITOK | ((zero_mem == YesZero) ? M_ZERO : 0);
      if (kmem_back(kernel_object, addr, size, flags) != KERN_SUCCESS)
      {
        return false;
      }
      return true;
    }

    /// OS specific function for zeroing memory
    template<bool page_aligned = false>
    static void zero(void* p, size_t size)
    {
      ::bzero(p, size);
    }

    template<bool state_using>
    static void* reserve_aligned(size_t size) noexcept
    {
      SNMALLOC_ASSERT(bits::is_pow2(size));
      SNMALLOC_ASSERT(size >= minimum_alloc_size);
      size_t align = size;

      vm_offset_t addr;
      if (vmem_xalloc(
            kernel_arena,
            size,
            align,
            0,
            0,
            VMEM_ADDR_MIN,
            VMEM_ADDR_MAX,
            M_BESTFIT,
            &addr))
      {
        return nullptr;
      }
      if (state_using)
      {
        if (
          kmem_back(kernel_object, addr, size, M_ZERO | M_WAITOK) !=
          KERN_SUCCESS)
        {
          vmem_xfree(kernel_arena, addr, size);
          return nullptr;
        }
      }
      return get_vm_offset(addr);
    }
  };
}
#endif
