#pragma once

#include "../ds_core/ds_core.h"
#include "snmalloc/stl/atomic.h"

namespace snmalloc
{
  template<typename T>
  class PalList
  {
    /**
     * List of callbacks to notify
     */
    stl::Atomic<T*> elements{nullptr};

    static_assert(
      stl::is_same_v<decltype(T::pal_next), stl::Atomic<T*>>,
      "Required pal_next type.");

  public:
    /**
     * Add an element to the list
     */
    void add(T* element)
    {
      auto prev = &elements;
      auto curr = prev->load();
      do
      {
        while (curr != nullptr)
        {
          prev = &(curr->pal_next);
          curr = prev->load();
        }
      } while (!prev->compare_exchange_weak(curr, element));
    }

    /**
     * Applies function to all the elements of the list
     */
    template<typename F>
    void apply_all(F func)
    {
      T* curr = elements;
      while (curr != nullptr)
      {
        func(curr);
        curr = curr->pal_next;
      }
    }
  };

  /**
   * This struct is used to represent callbacks for notification from the
   * platform. It contains a next pointer as client is responsible for
   * allocation as we cannot assume an allocator at this point.
   */
  struct PalNotificationObject
  {
    stl::Atomic<PalNotificationObject*> pal_next = nullptr;

    void (*pal_notify)(PalNotificationObject* self);

    PalNotificationObject(void (*pal_notify)(PalNotificationObject* self))
    : pal_notify(pal_notify)
    {}
  };

  /***
   * Wrapper for managing notifications for PAL events
   */
  class PalNotifier
  {
    /**
     * List of callbacks to notify
     */
    PalList<PalNotificationObject> callbacks;

  public:
    /**
     * Register a callback object to be notified
     *
     * The object should never be deallocated by the client after calling
     * this.
     */
    void register_notification(PalNotificationObject* callback)
    {
      callbacks.add(callback);
    }

    /**
     * Calls the pal_notify of all the registered objects.
     */
    void notify_all()
    {
      callbacks.apply_all([](auto curr) { curr->pal_notify(curr); });
    }
  };

  class PalTimerObject
  {
    friend class PalTimer;
    template<typename T>
    friend class PalList;

    stl::Atomic<PalTimerObject*> pal_next;

    void (*pal_notify)(PalTimerObject* self);

    uint64_t last_run = 0;
    uint64_t repeat;

  public:
    PalTimerObject(void (*pal_notify)(PalTimerObject* self), uint64_t repeat)
    : pal_notify(pal_notify), repeat(repeat)
    {}
  };

  /**
   * Simple mechanism for handling timers.
   *
   * Note: This is really designed for a very small number of timers,
   * and this design should be changed if that is no longer the case.
   */
  class PalTimer
  {
    /**
     * List of callbacks to notify
     */
    PalList<PalTimerObject> timers;

  public:
    /**
     * Register a callback to be called every repeat milliseconds.
     */
    void register_timer(PalTimerObject* timer)
    {
      timers.add(timer);
    }

    void check(uint64_t time_ms)
    {
      static stl::AtomicBool lock{false};

      // Deduplicate calls into here, and make single threaded.
      if (lock.exchange(true, stl::memory_order_acquire))
        return;

      timers.apply_all([time_ms](PalTimerObject* curr) {
        if (
          (curr->last_run == 0) || ((time_ms - curr->last_run) > curr->repeat))
        {
          curr->last_run = time_ms;
          curr->pal_notify(curr);
        }
      });

      lock.store(false, stl::memory_order_release);
    }
  };
} // namespace snmalloc
