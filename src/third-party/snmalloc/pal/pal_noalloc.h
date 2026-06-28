#ifndef SNMALLOC_PAL_NO_ALLOC_H
#define SNMALLOC_PAL_NO_ALLOC_H

#pragma once

#include "../aal/aal.h"
#include "pal_concept.h"
#include "pal_consts.h"
#include "pal_timer_default.h"

#include <string.h>

namespace snmalloc
{
#ifdef __cpp_concepts
  /**
   * The minimal subset of a PAL that we need for delegation
   */
  template<typename PAL>
  concept PALNoAllocBase = IsPAL_static_sizes<PAL> && IsPAL_error<PAL>;
#endif

  /**
   * Platform abstraction layer that does not allow allocation.
   *
   * This is a minimal PAL for pre-reserved memory regions, where the
   * address-space manager is initialised with all of the memory that it will
   * ever use.
   */
  template<SNMALLOC_CONCEPT(PALNoAllocBase) BasePAL>
  struct PALNoAlloc : public BasePAL
  {
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     */
    static constexpr uint64_t pal_features = NoAllocation;

    static constexpr size_t page_size = BasePAL::page_size;

    static constexpr size_t address_bits = BasePAL::address_bits;

    /**
     * Print a stack trace.
     */
    static void print_stack_trace()
    {
      BasePAL::print_stack_trace();
    }

    /**
     * Report a message to the user.
     */
    static void message(const char* const str) noexcept
    {
      BasePAL::message(str);
    }

    /**
     * Report a fatal error an exit.
     */
    [[noreturn]] static void error(const char* const str) noexcept
    {
      BasePAL::error(str);
    }

    /**
     * Notify platform that we will not be using these pages.
     *
     * This is a no-op in this stub.
     */
    static void notify_not_using(void*, size_t) noexcept {}

    /**
     * Notify platform that we will be using these pages.
     *
     * This is a no-op in this stub, except for zeroing memory if required.
     */
    template<ZeroMem zero_mem>
    static bool notify_using(void* p, size_t size) noexcept
    {
      if constexpr (zero_mem == YesZero)
      {
        zero<true>(p, size);
      }
      else
      {
        UNUSED(p, size);
      }
      return true;
    }

    /**
     * OS specific function for zeroing memory.
     *
     * This just calls memset - we don't assume that we have access to any
     * virtual-memory functions.
     */
    template<bool page_aligned = false>
    static void zero(void* p, size_t size) noexcept
    {
      memset(p, 0, size);
    }
  };
} // namespace snmalloc

#endif
