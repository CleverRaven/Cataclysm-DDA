#pragma once

#include "snmalloc/stl/atomic.h"

namespace snmalloc
{
  /**
   * @brief Get the an id for the current thread.
   *
   * @return the thread id, this should never be the default of
   * ThreadIdentity. Callers can assume it is a non-default value.
   *
   * Note, this is only for debug.  We should not assume that this is unique.
   */
  inline size_t debug_get_tid() noexcept
  {
    static thread_local size_t tid{0};
    static stl::Atomic<size_t> tid_source{0};

    if (tid == 0)
    {
      tid = ++tid_source;
    }
    return tid;
  }
} // namespace snmalloc