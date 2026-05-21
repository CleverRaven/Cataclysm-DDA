#pragma once

#include "../ds_core/ds_core.h"
#include "pal_timer_default.h"

namespace snmalloc
{
  // Can be extended
  // Will require a reserve method in subclasses.
  template<class State>
  class PALPlainMixin : public State, public PalTimerDefaultImpl<State>
  {
  public:
    // Notify platform that we will not be using these pages
    static void notify_not_using(void*, size_t) noexcept {}

    // Notify platform that we will not be using these pages
    template<ZeroMem zero_mem>
    static bool notify_using(void* p, size_t size) noexcept
    {
      if constexpr (zero_mem == YesZero)
      {
        State::zero(p, size);
      }
      else
      {
        UNUSED(p, size);
      }
      return true;
    }
  };
} // namespace snmalloc
