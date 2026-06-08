#pragma once

#include "../ds_core/ds_core.h"

#include <stdint.h>

namespace snmalloc
{
  /**
   * This class will attempt to call the PAL every 50ms to check the time.
   * If the caller of check_tick, does so more frequently, it will attempt
   * to back-off to only query the time, every n calls to check_tick, where
   * `n` adapts to the current frequency of calling.
   *
   * The aim is to reduce the time spent querying the time as this might be
   * an expensive operation if time has been virtualised.
   */
  template<typename PAL>
  class Ticker
  {
    /**
     * Calls to check_tick required before the time is next queried
     */
    uint64_t count_down = 1;

    /**
     * Number of ticks next time we check the time.
     * That is,
     *    counted - count_down
     * Is how many ticks, since last_epoch_ms was updated.
     */
    uint64_t counted = 1;

    /**
     * Last time we queried the clock.
     */
    uint64_t last_query_ms = 0;

    /**
     * Slow path that actually queries clock and sets up
     * how many calls for the next time we hit the slow path.
     */
    template<typename T = void*>
    SNMALLOC_SLOW_PATH T check_tick_slow(T p = nullptr) noexcept
    {
      uint64_t now_ms = PAL::time_in_ms();

      // Set up clock.
      if (last_query_ms == 0)
      {
        last_query_ms = now_ms;
        count_down = 1;
        counted = 1;
        return p;
      }

      uint64_t duration_ms = now_ms - last_query_ms;
      last_query_ms = now_ms;

      // Check is below clock resolution
      if (duration_ms == 0)
      {
        // Exponential back off
        count_down = counted;
        counted *= 2;
        return p;
      }

      constexpr size_t deadline_in_ms = 50;

      // Estimate number of ticks to get to the new deadline, based on the
      // current interval
      auto new_deadline_in_ticks =
        ((1 + counted) * deadline_in_ms) / duration_ms;

      counted = new_deadline_in_ticks;
      count_down = new_deadline_in_ticks;

      return p;
    }

  public:
    template<typename T = void*>
    SNMALLOC_FAST_PATH T check_tick(T p = nullptr)
    {
      if constexpr (pal_supports<Time, PAL>)
      {
        // Check before decrement, so that later calcations can use
        // count_down == 0 for check on the next call.
        // This is used if the ticks are way below the frequency of
        // heart beat.
        if (--count_down == 0)
        {
          return check_tick_slow(p);
        }
      }
      return p;
    }
  };
} // namespace snmalloc
