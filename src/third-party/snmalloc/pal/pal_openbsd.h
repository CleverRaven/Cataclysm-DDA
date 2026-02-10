#pragma once

#if defined(__OpenBSD__) && !defined(_KERNEL)
#  include "pal_bsd.h"

#  include <sys/futex.h>

namespace snmalloc
{
  /**
   * OpenBSD platform abstraction layer.
   *
   * OpenBSD behaves exactly like a generic BSD platform but this class exists
   * as a place to add OpenBSD-specific behaviour later, if required.
   */
  class PALOpenBSD : public PALBSD<PALOpenBSD>
  {
  public:
    /**
     * The features exported by this PAL.
     *
     * Currently, these are identical to the generic BSD PAL.  This field is
     * declared explicitly to remind anyone who modifies this class that they
     * should add any required features.
     */
    static constexpr uint64_t pal_features =
      PALBSD::pal_features | WaitOnAddress;

    using WaitingWord = int;

    template<class T>
    static void wait_on_address(std::atomic<T>& addr, T expected)
    {
      int backup = errno;
      static_assert(
        sizeof(T) == sizeof(WaitingWord) && alignof(T) == alignof(WaitingWord),
        "T must be the same size and alignment as WaitingWord");
      while (addr.load(std::memory_order_relaxed) == expected)
      {
        futex(
          (uint32_t*)&addr,
          FUTEX_WAIT_PRIVATE,
          static_cast<int>(expected),
          nullptr,
          nullptr);
      }
      errno = backup;
    }

    template<class T>
    static void notify_one_on_address(std::atomic<T>& addr)
    {
      static_assert(
        sizeof(T) == sizeof(WaitingWord) && alignof(T) == alignof(WaitingWord),
        "T must be the same size and alignment as WaitingWord");
      futex((uint32_t*)&addr, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr);
    }

    template<class T>
    static void notify_all_on_address(std::atomic<T>& addr)
    {
      static_assert(
        sizeof(T) == sizeof(WaitingWord) && alignof(T) == alignof(WaitingWord),
        "T must be the same size and alignment as WaitingWord");
      futex((uint32_t*)&addr, FUTEX_WAKE_PRIVATE, INT_MAX, nullptr, nullptr);
    }
  };
} // namespace snmalloc
#endif
