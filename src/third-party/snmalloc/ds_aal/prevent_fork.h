#pragma once

#include <snmalloc/aal/aal.h>
#include <snmalloc/stl/atomic.h>
#include <stddef.h>

#ifdef SNMALLOC_PTHREAD_ATFORK_WORKS
#  include <pthread.h>
#endif

namespace snmalloc
{

#ifdef SNMALLOC_PTHREAD_FORK_PROTECTION
  // This is a simple implementation of a class that can be
  // used to prevent a process from forking. Holding a lock
  // in the allocator while forking can lead to deadlocks.
  // This causes the fork to wait out any other threads inside
  // the allocators locks.
  //
  // The use is
  // ```
  //  {
  //     PreventFork pf;
  //     // Code that should not be running during a fork.
  //  }
  // ```
  class PreventFork
  {
    // Global atomic counter of the number of threads currently preventing the
    // system from forking. The bottom bit is used to signal that a thread is
    // wanting to fork.
    static inline stl::Atomic<size_t> threads_preventing_fork{0};

    // The depth of the current thread's prevention of forking.
    // This is used to enable reentrant prevention of forking.
    static inline thread_local size_t depth_of_prevention{0};

    // There could be multiple copies of the atfork handler installed.
    // Only perform work for the first prefork and final postfork.
    static inline thread_local size_t depth_of_handlers{0};

    // This function ensures that the fork handler has been installed at least
    // once. It might be installed more than once, this is safe. As subsequent
    // calls would be ignored.
    static void ensure_init()
    {
#  ifdef SNMALLOC_PTHREAD_ATFORK_WORKS
      static stl::Atomic<bool> initialised{false};

      if (initialised.load(stl::memory_order_acquire))
        return;

      pthread_atfork(prefork, postfork_parent, postfork_child);
      initialised.store(true, stl::memory_order_release);
#  endif
    };

  public:
    PreventFork()
    {
      if (depth_of_prevention++ == 0)
      {
        // Ensure that the system is initialised before we start.
        // Don't do this on nested Prevent calls.
        ensure_init();
        while (true)
        {
          auto prev = threads_preventing_fork.fetch_add(2);
          if (prev % 2 == 0)
            break;

          threads_preventing_fork.fetch_sub(2);

          while ((threads_preventing_fork.load() % 2) == 1)
          {
            Aal::pause();
          }
        };
      }
    }

    ~PreventFork()
    {
      if (--depth_of_prevention == 0)
      {
        threads_preventing_fork -= 2;
      }
    }

    // The function that notifies new threads not to enter PreventFork regions
    // It waits until all threads are no longer in a PreventFork region before
    // returning.
    static void prefork()
    {
      if (depth_of_handlers++ != 0)
        return;

      if (depth_of_prevention != 0)
        error("Fork attempted while in PreventFork region.");

      while (true)
      {
        auto current = threads_preventing_fork.load();
        if (
          (current % 2 == 0) &&
          (threads_preventing_fork.compare_exchange_weak(current, current + 1)))
        {
          break;
        }
        Aal::pause();
      };

      while (threads_preventing_fork.load() != 1)
      {
        Aal::pause();
      }

      // Finally set the flag that allows this thread to enter PreventFork
      // regions This is safe as the only other calls here are to other prefork
      // handlers.
      depth_of_prevention++;
    }

    // Unsets the flag that allows threads to enter PreventFork regions
    // and for another thread to request a fork.
    static void postfork_child()
    {
      // Count out the number of handlers that have been called, and
      // only perform on the last.
      if (--depth_of_handlers != 0)
        return;

      // This thread is no longer preventing a fork, so decrement the counter.
      depth_of_prevention--;

      // Allow other threads to allocate
      // There could have been threads spinning in the prefork handler having
      // optimistically increasing thread_preventing_fork by 2, but now the
      // threads do not exist due to the fork. So restart the counter in the
      // child.
      threads_preventing_fork = 0;
    }

    // Unsets the flag that allows threads to enter PreventFork regions
    // and for another thread to request a fork.
    static void postfork_parent()
    {
      // Count out the number of handlers that have been called, and
      // only perform on the last.
      if (--depth_of_handlers != 0)
        return;

      // This thread is no longer preventing a fork, so decrement the counter.
      depth_of_prevention--;

      // Allow other threads to allocate
      // Just remove the bit, and let the potential other threads in prefork
      // remove their counts.
      threads_preventing_fork--;
    }
  };
#else
  // The fork protection can cost a lot and it is generally not required.
  // This is a dummy implementation of the PreventFork class that does nothing.
  class PreventFork
  {
  public:
    PreventFork() {}

    ~PreventFork() {}
  };
#endif
} // namespace snmalloc