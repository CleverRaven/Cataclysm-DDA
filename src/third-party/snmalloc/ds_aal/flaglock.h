#pragma once

#include "../aal/aal.h"
#include "prevent_fork.h"
#include "snmalloc/ds_core/ds_core.h"
#include "snmalloc/stl/atomic.h"

namespace snmalloc
{
  /**
   * @brief The DebugFlagWord struct
   * Wrapper for stl::AtomicBool so that we can examine
   * the re-entrancy problem at debug mode.
   */
  struct DebugFlagWord
  {
    using ThreadIdentity = size_t;

    /**
     * @brief flag
     * The underlying atomic field.
     */
    stl::AtomicBool flag{false};

    constexpr DebugFlagWord() = default;

    template<typename... Args>
    constexpr DebugFlagWord(Args&&... args) : flag(stl::forward<Args>(args)...)
    {}

    /**
     * @brief set_owner
     * Record the identity of the locker.
     */
    void set_owner()
    {
      SNMALLOC_ASSERT(ThreadIdentity() == owner);
      owner = get_thread_identity();
    }

    /**
     * @brief clear_owner
     * Set the identity to null.
     */
    void clear_owner()
    {
      SNMALLOC_ASSERT(get_thread_identity() == owner);
      owner = ThreadIdentity();
    }

    /**
     * @brief assert_not_owned_by_current_thread
     * Assert the lock should not be held already by current thread.
     */
    void assert_not_owned_by_current_thread()
    {
      SNMALLOC_ASSERT(get_thread_identity() != owner);
    }

  private:
    /**
     * @brief owner
     * We use the Pal to provide the ThreadIdentity.
     */
    stl::Atomic<ThreadIdentity> owner = ThreadIdentity();

    /**
     * @brief get_thread_identity
     * @return The identity of current thread.
     */
    static ThreadIdentity get_thread_identity()
    {
      return debug_get_tid();
    }
  };

  /**
   * @brief The ReleaseFlagWord struct
   * The shares the same structure with DebugFlagWord but
   * all member functions associated with ownership checkings
   * are empty so that they can be optimised out at Release mode.
   */
  struct ReleaseFlagWord
  {
    stl::AtomicBool flag{false};

    constexpr ReleaseFlagWord() = default;

    template<typename... Args>
    constexpr ReleaseFlagWord(Args&&... args)
    : flag(stl::forward<Args>(args)...)
    {}

    void set_owner() {}

    void clear_owner() {}

    void assert_not_owned_by_current_thread() {}
  };

#ifdef NDEBUG
  using FlagWord = ReleaseFlagWord;
#else
  using FlagWord = DebugFlagWord;
#endif

  class FlagLock
  {
  private:
    FlagWord& lock;

    // A unix fork while holding a lock can lead to deadlock. Protect against
    // this by not allowing a fork while holding a lock.
    PreventFork pf{};

  public:
    FlagLock(FlagWord& lock) : lock(lock)
    {
      while (
        SNMALLOC_UNLIKELY(lock.flag.exchange(true, stl::memory_order_acquire)))
      {
        // assert_not_owned_by_current_thread is only called when the first
        // acquiring is failed; which means the lock is already held somewhere
        // else.
        lock.assert_not_owned_by_current_thread();
        // This loop is better for spin-waiting because it won't issue
        // expensive write operation (xchg for example).
        while (SNMALLOC_UNLIKELY(lock.flag.load(stl::memory_order_relaxed)))
        {
          Aal::pause();
        }
      }
      lock.set_owner();
    }

    ~FlagLock()
    {
      lock.clear_owner();
      lock.flag.store(false, stl::memory_order_release);
    }
  };

  template<typename F>
  inline void with(FlagWord& lock, F&& f)
  {
    FlagLock l(lock);
    f();
  }
} // namespace snmalloc
