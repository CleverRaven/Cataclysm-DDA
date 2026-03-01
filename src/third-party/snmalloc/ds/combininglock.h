#pragma once

#include "../aal/aal.h"
#include "../pal/pal.h"
#include "snmalloc/stl/atomic.h"

namespace snmalloc
{
  class CombiningLockNode;

  struct CombiningLock
  {
    // Fast path lock incase there is no contention.
    stl::Atomic<bool> flag{false};

    // MCS queue of work items
    stl::Atomic<CombiningLockNode*> last{nullptr};

    void release()
    {
      flag.store(false, stl::memory_order_release);
    }
  };

  /**
   * @brief Combinations of MCS queue lock with Flat Combining
   *
   * Each element in the queue has a pointer to a work item.
   * This means when under contention the thread holding the lock
   * can perform the work.
   *
   * As the work items are arbitrary lambdas there are no simplifications
   * for combining related work items.  I.e. original Flat Combining paper
   * might sort a collection of inserts, and perform them in a single traversal.
   *
   * Note that, we should perhaps add a Futex/WakeOnAddress mode to improve
   * performance in the contended case, rather than spinning.
   */
  class CombiningLockNode
  {
    template<typename Pal>
    static constexpr bool use_wait_on_address =
      pal_supports<PalFeatures::WaitOnAddress, Pal> &&
      SNMALLOC_USE_WAIT_ON_ADDRESS;

    template<bool HasWaitOnAddress, typename Pal>
    struct WaitWordTypeSelect;

    template<typename Pal>
    struct WaitWordTypeSelect<true, Pal>
    {
      using type = typename Pal::WaitingWord;
    };

    template<typename Pal>
    struct WaitWordTypeSelect<false, Pal>
    {
      using type = int;
    };

    using WaitingWordType =
      typename WaitWordTypeSelect<use_wait_on_address<DefaultPal>, DefaultPal>::
        type;

    template<typename F>
    friend class CombiningLockNodeTempl;

    enum class LockStatus : WaitingWordType
    {
      // The work for this node has not been completed.
      WAITING,

      // The work for this thread has been completed, and it is not the
      // last element in the queue.
      DONE,

      // The work for this thread has not been completed, and it is the
      // head of the queue.
      HEAD,

      // The waiter is currently sleeping.
      SLEEPING
    };

    // Status of the queue, set by the thread at the head of the queue,
    // When it makes the thread for this node either the head of the queue
    // or completes its work.
    stl::Atomic<LockStatus> status{LockStatus::WAITING};

    // Used to store the queue
    stl::Atomic<CombiningLockNode*> next{nullptr};

    // Stores the C++ lambda associated with this node in the queue.
    void (*f_raw)(CombiningLockNode*);

    constexpr CombiningLockNode(void (*f)(CombiningLockNode*)) : f_raw(f) {}

    void set_status(LockStatus s)
    {
      status.store(s, stl::memory_order_release);
    }

    template<typename Pal = DefaultPal>
    static void wake(CombiningLockNode* node, LockStatus message)
    {
      if constexpr (!use_wait_on_address<Pal>)
      {
        node->set_status(message);
      }
      else
      {
        if (
          node->status.exchange(message, stl::memory_order_acq_rel) ==
          LockStatus::SLEEPING)
        {
          Pal::notify_one_on_address(node->status);
        }
      }
    }

    template<typename Pal = DefaultPal>
    void wait()
    {
      if constexpr (!use_wait_on_address<Pal>)
      {
        while (status.load(stl::memory_order_acquire) == LockStatus::WAITING)
          Aal::pause();
      }
      else
      {
        int remaining = 100;
        while (remaining > 0)
        {
          if (status.load(stl::memory_order_acquire) != LockStatus::WAITING)
            return;
          Aal::pause();
          remaining--;
        }
        LockStatus expected = LockStatus::WAITING;
        if (status.compare_exchange_strong(
              expected, LockStatus::SLEEPING, stl::memory_order_acq_rel))
        {
          Pal::wait_on_address(status, LockStatus::SLEEPING);
          if (status.load(stl::memory_order_acquire) == LockStatus::SLEEPING)
          {
            error("Corruption in core locking primitive. Aborting execution.");
          }
        }
      }
    }

    SNMALLOC_SLOW_PATH void attach_slow(CombiningLock& lock)
    {
      // There is contention for the lock, we need to add our work to the
      // queue of pending work
      auto prev = lock.last.exchange(this, stl::memory_order_acq_rel);

      if (prev != nullptr)
      {
        // If we aren't the head, link into predecessor
        prev->next.store(this, stl::memory_order_release);

        // Wait to for predecessor to complete
        wait();

        // Determine if another thread completed our work.
        if (status.load(stl::memory_order_acquire) == LockStatus::DONE)
          return;
      }
      else
      {
        // We are the head of the queue. Spin until we acquire the fast path
        // lock.  As we are in the queue future requests shouldn't try to
        // acquire the fast path lock, but stale views of the queue being empty
        // could still be concurrent with this thread.
        while (lock.flag.exchange(true, stl::memory_order_acquire))
        {
          while (lock.flag.load(stl::memory_order_relaxed))
          {
            Aal::pause();
          }
        }

        // We could set
        //    status = LockStatus::HEAD
        // However, the subsequent state assumes it is HEAD, and
        // nothing would read it.
      }

      // We are the head of the queue, and responsible for
      // waking/performing our and subsequent work.
      auto curr = this;
      while (true)
      {
        // Start pulling in the next element of the queue
        auto n = curr->next.load(stl::memory_order_acquire);
        Aal::prefetch(n);

        // Perform work for head of the queue
        curr->f_raw(curr);

        // Determine if there are more elements.
        n = curr->next.load(stl::memory_order_acquire);
        if (n == nullptr)
          break;
        // Signal this work was completed and move on to
        // next item.
        wake(curr, LockStatus::DONE);
        curr = n;
      }

      // This could be the end of the queue, attempt to close the
      // queue.
      auto curr_c = curr;
      if (lock.last.compare_exchange_strong(
            curr_c,
            nullptr,
            stl::memory_order_release,
            stl::memory_order_relaxed))
      {
        // Queue was successfully closed.
        // Notify last element the work was completed.
        wake(curr, LockStatus::DONE);
        lock.release();
        return;
      }

      // Failed to close the queue wait for next thread to be
      // added.
      while (curr->next.load(stl::memory_order_relaxed) == nullptr)
        Aal::pause();

      auto n = curr->next.load(stl::memory_order_acquire);

      // As we had to wait, give the job to the next thread
      // to carry on performing the work.
      wake(n, LockStatus::HEAD);

      // Notify the thread that we completed its work.
      // Note that this needs to be before setting curr->status,
      // as after the status is set the thread may deallocate the
      // queue node.
      wake(curr, LockStatus::DONE);
      return;
    }
  };

  template<typename F>
  class CombiningLockNodeTempl : CombiningLockNode
  {
    template<typename FF>
    friend void with(CombiningLock&, FF&&);

    // This holds the closure for the lambda
    F f;

    CombiningLockNodeTempl(CombiningLock& lock, F&& f_)
    : CombiningLockNode([](CombiningLockNode* self) {
        CombiningLockNodeTempl* self_templ =
          reinterpret_cast<CombiningLockNodeTempl*>(self);
        self_templ->f();
      }),
      f(stl::forward<F>(f_))
    {
      attach_slow(lock);
    }
  };

  /**
   * Lock primitive. This takes a reference to a Lock, and a thunk to
   * call when the lock is available.  The thunk should be independent of
   * the current thread as the thunk may be executed by a different thread.
   */
  template<typename F>
  inline void with(CombiningLock& lock, F&& f)
  {
    // A unix fork while holding a lock can lead to deadlock. Protect against
    // this by not allowing a fork while holding a lock.
    PreventFork pf;
    snmalloc::UNUSED(pf);
    {
      // Test if no one is waiting
      if (SNMALLOC_LIKELY(lock.last.load(stl::memory_order_relaxed) == nullptr))
      {
        // No one was waiting so low contention. Attempt to acquire the flag
        // lock.
        if (SNMALLOC_LIKELY(
              lock.flag.exchange(true, stl::memory_order_acquire) == false))
        {
          // We grabbed the lock.
          // Execute the thunk.
          f();

          // Release the lock
          lock.release();
          return;
        }
      }

      // There is contention for the lock, we need to take the slow path
      // with the queue.
      CombiningLockNodeTempl<F> node(lock, stl::forward<F>(f));
    }
  }
} // namespace snmalloc
