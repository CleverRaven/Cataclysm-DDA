
#pragma once

#include "pal_consts.h"
#include "pal_ds.h"

namespace snmalloc
{
  template<typename PalTime>
  class PalTimerDefaultImpl
  {
    inline static PalTimer timers{};

  public:
    static uint64_t time_in_ms()
    {
      auto time = PalTime::internal_time_in_ms();

      // Process timers
      timers.check(time);

      return time;
    }

    static void register_timer(PalTimerObject* timer)
    {
      timers.register_timer(timer);
    }
  };
} // namespace snmalloc
