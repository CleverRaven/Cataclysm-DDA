#pragma once

#include "prevent_fork.h"
#include "snmalloc/stl/atomic.h"

namespace snmalloc
{
  /*
   * In some use cases we need to run before any of the C++ runtime has been
   * initialised.  This singleton class is designed to not depend on the
   * runtime.
   */
  template<class Object, void init(Object*) noexcept>
  class Singleton
  {
    enum class State
    {
      Uninitialised,
      Initialising,
      Initialised
    };

    inline static stl::Atomic<State> initialised{State::Uninitialised};
    inline static Object obj;

  public:
    /**
     * If argument is non-null, then it is assigned the value
     * true, if this is the first call to get.
     * At most one call will be first.
     */
    inline SNMALLOC_SLOW_PATH static Object& get(bool* first = nullptr)
    {
      // If defined should be initially false;
      SNMALLOC_ASSERT(first == nullptr || *first == false);

      auto state = initialised.load(stl::memory_order_acquire);
      if (SNMALLOC_UNLIKELY(state == State::Uninitialised))
      {
        // A unix fork while initialising a singleton can lead to deadlock.
        // Protect against this by not allowing a fork while attempting
        // initialisation.
        PreventFork pf;
        snmalloc::UNUSED(pf);
        if (initialised.compare_exchange_strong(
              state, State::Initialising, stl::memory_order_relaxed))
        {
          init(&obj);
          initialised.store(State::Initialised, stl::memory_order_release);
          if (first != nullptr)
            *first = true;
        }
      }

      while (SNMALLOC_UNLIKELY(state != State::Initialised))
      {
        Aal::pause();
        state = initialised.load(stl::memory_order_acquire);
      }

      return obj;
    }
  };
} // namespace snmalloc
