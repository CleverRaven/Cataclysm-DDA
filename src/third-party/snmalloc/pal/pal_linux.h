#pragma once

#if defined(__linux__)
#  include "../ds_core/ds_core.h"
#  include "pal_posix.h"
#  include "snmalloc/stl/array.h"

#  include <string.h>
#  include <sys/mman.h>
#  include <sys/prctl.h>
#  include <syscall.h>
// __has_include does not reliably determine if we actually have linux/random.h
// available
#  if defined(SNMALLOC_HAS_LINUX_RANDOM_H)
#    include <linux/random.h>
#  endif

#  if defined(SNMALLOC_HAS_LINUX_FUTEX_H)
#    include <linux/futex.h>
#  endif

#  include <stdio.h>

namespace snmalloc
{
  class PALLinux : public PALPOSIX<PALLinux>
  {
  public:
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     *
     * We always make sure that linux has entropy support.
     */
    static constexpr uint64_t pal_features = PALPOSIX::pal_features | Entropy |
      CoreDump
#  ifdef SNMALLOC_HAS_LINUX_FUTEX_H
      | WaitOnAddress
#  endif
      ;

    static constexpr size_t page_size =
      Aal::aal_name == PowerPC ? 0x10000 : PALPOSIX::page_size;

    /**
     * Linux requires an explicit no-reserve flag in `mmap` to guarantee lazy
     * commit if /proc/sys/vm/overcommit_memory is set to `heuristic` (0).
     *
     *   https://www.kernel.org/doc/html/latest/vm/overcommit-accounting.html
     */
    static constexpr int default_mmap_flags = MAP_NORESERVE;

    /**
     * MADV_FREE is only available since Linux 4.5.
     *
     * Fallback to MADV_DONTNEED on older kernels
     */
    static constexpr int madvise_free_flags =
#  if defined(MADV_FREE)
      MADV_FREE
#  else
      MADV_DONTNEED
#  endif
      ;

    static void* reserve(size_t size) noexcept
    {
      void* p = PALPOSIX<PALLinux>::reserve(size);
      if (p)
      {
#  ifdef SNMALLOC_PAGEID
#    ifndef PR_SET_VMA
#      define PR_SET_VMA 0x53564d41
#      define PR_SET_VMA_ANON_NAME 0
#    endif

        /**
         *
         * If the kernel is set with CONFIG_ANON_VMA_NAME
         * the reserved pages would appear as follow
         *
         * 7fa5f0ceb000-7fa5f0e00000 rw-p 00000000 00:00 0 [anon:snmalloc]
         * 7fa5f0e00000-7fa5f1800000 rw-p 00000000 00:00 0 [anon:snmalloc]
         *
         */

        prctl(
          PR_SET_VMA,
          PR_SET_VMA_ANON_NAME,
          (unsigned long)p,
          size,
          (unsigned long)"snmalloc");
#  endif
      }
      return p;
    }

    /**
     * OS specific function for zeroing memory.
     *
     * Linux implements an unusual interpretation of `MADV_DONTNEED`, which
     * immediately resets the pages to the zero state (rather than marking them
     * as sensible ones to swap out in high memory pressure).  We use this to
     * clear the underlying memory range.
     */
    template<bool page_aligned = false>
    static void zero(void* p, size_t size) noexcept
    {
      // QEMU does not seem to be giving the desired behaviour for
      // MADV_DONTNEED. switch back to memset only for QEMU.
#  ifndef SNMALLOC_QEMU_WORKAROUND
      if (
        (page_aligned || is_aligned_block<page_size>(p, size)) &&
        (size > 16 * page_size))
      {
        // Only use this on large allocations as memset faster, and doesn't
        // introduce IPI so faster for small allocations.
        SNMALLOC_ASSERT(is_aligned_block<page_size>(p, size));
        madvise(p, size, MADV_DONTNEED);
      }
      else
#  endif
      {
        ::memset(p, 0, size);
      }
    }

    static void notify_not_using(void* p, size_t size) noexcept
    {
      KeepErrno k;
      SNMALLOC_ASSERT(is_aligned_block<page_size>(p, size));

      // Fill memory so that when we switch the pages back on we don't make
      // assumptions on the content.
      if constexpr (Debug)
        memset(p, 0x5a, size);

      madvise(p, size, madvise_free_flags);

      if constexpr (mitigations(pal_enforce_access))
      {
        mprotect(p, size, PROT_NONE);
      }
    }

    /**
     * Notify platform that these pages should be included in a core dump.
     */
    static void notify_do_dump(void* p, size_t size) noexcept
    {
      KeepErrno k;
      madvise(p, size, MADV_DODUMP);
    }

    /**
     * Notify platform that these pages should not be included in a core dump.
     */
    static void notify_do_not_dump(void* p, size_t size) noexcept
    {
      KeepErrno k;
      madvise(p, size, MADV_DONTDUMP);
    }

    static uint64_t get_entropy64()
    {
      KeepErrno k;

      // TODO: If the system call fails then the POSIX PAL calls libc
      // functions that can require malloc, which may result in deadlock.

      // SYS_getrandom API stablized since 3.17.
      // This fallback implementation is to aid some environments
      // where SYS_getrandom is provided in kernel but the libc
      // is not providing getentropy interface.

      union
      {
        uint64_t result;
        char buffer[sizeof(uint64_t)];
      };

      ssize_t ret;

      // give a try to SYS_getrandom
#  ifdef SYS_getrandom
      static stl::AtomicBool syscall_not_working = false;
      // Relaxed ordering should be fine here. This function will be called
      // during early initialisation, which will examine the availability in a
      // protected routine.
      if (false == syscall_not_working.load(stl::memory_order_relaxed))
      {
        auto current = stl::begin(buffer);
        auto target = stl::end(buffer);
        while (auto length = target - current)
        {
          // Reading data via syscall from system entropy pool.
          // According to both MUSL and GLIBC implementation, getentropy uses
          // /dev/urandom (blocking API).
          //
          // The third argument here indicates:
          // 1. `GRND_RANDOM` bit is not set, so the source of entropy will be
          // `urandom`.
          // 2. `GRND_NONBLOCK` bit is set. Since we are reading from
          // `urandom`, this means if the entropy pool is
          // not initialised, we will get a EAGAIN.
          ret = syscall(SYS_getrandom, current, length, GRND_NONBLOCK);
          // check whether are interrupt by a signal
          if (SNMALLOC_UNLIKELY(ret < 0))
          {
            if (SNMALLOC_UNLIKELY(errno == EAGAIN))
            {
              // the system is going through early initialisation: at this stage
              // it is very likely that snmalloc is being used in some system
              // programs and we do not want to block it.
              return reinterpret_cast<uint64_t>(&result) ^
                reinterpret_cast<uint64_t>(&error);
            }
            if (errno != EINTR)
            {
              break;
            }
          }
          else
          {
            current += ret;
          }
        }
        if (SNMALLOC_UNLIKELY(target != current))
        {
          // in this routine, the only possible situations should be ENOSYS
          // or EPERM (forbidden by seccomp, for example).
          SNMALLOC_ASSERT(errno == ENOSYS || errno == EPERM);
          syscall_not_working.store(true, stl::memory_order_relaxed);
        }
        else
        {
          return result;
        }
      }
#  endif

      // Syscall is not working.
      // In this case, it is not a good idea to fallback to std::random_device:
      // 1. it may want to use malloc to create a buffer, which causes
      // reentrancy problem during initialisation routine.
      // 2. some implementations also require libstdc++ to be linked since
      // its APIs are not exception-free.
      return dev_urandom();
    }

#  ifdef SNMALLOC_HAS_LINUX_FUTEX_H
    using WaitingWord = int;

    template<class T>
    static void wait_on_address(stl::Atomic<T>& addr, T expected)
    {
      KeepErrno k;
      static_assert(
        sizeof(T) == sizeof(WaitingWord) && alignof(T) == alignof(WaitingWord),
        "T must be the same size and alignment as WaitingWord");
      while (addr.load(stl::memory_order_relaxed) == expected)
      {
        // Man page
        // (https://www.man7.org/linux/man-pages/man2/futex.2.html#RETURN_VALUE)
        // says:
        //  FUTEX_WAIT
        //    Returns 0 if the caller was woken up.  Note that a wake-up
        //    can also be caused by common futex usage patterns in
        //    unrelated code that happened to have previously used the
        //    futex word's memory location (e.g., typical futex-based
        //    implementations of Pthreads mutexes can cause this under
        //    some conditions).  Therefore, callers should always
        //    conservatively assume that a return value of 0 can mean a
        //    spurious wake-up, and use the futex word's value (i.e., the
        //    user-space synchronization scheme) to decide whether to
        //    continue to block or not.
        // We ignore the return and recheck.
        syscall(
          SYS_futex, &addr, FUTEX_WAIT_PRIVATE, expected, nullptr, nullptr, 0);
      }
    }

    template<class T>
    static void notify_one_on_address(stl::Atomic<T>& addr)
    {
      KeepErrno k;
      static_assert(
        sizeof(T) == sizeof(WaitingWord) && alignof(T) == alignof(WaitingWord),
        "T must be the same size and alignment as WaitingWord");
      syscall(SYS_futex, &addr, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }

    template<class T>
    static void notify_all_on_address(stl::Atomic<T>& addr)
    {
      KeepErrno k;
      static_assert(
        sizeof(T) == sizeof(WaitingWord) && alignof(T) == alignof(WaitingWord),
        "T must be the same size and alignment as WaitingWord");
      syscall(
        SYS_futex, &addr, FUTEX_WAKE_PRIVATE, INT_MAX, nullptr, nullptr, 0);
    }
#  endif
  };
} // namespace snmalloc
#endif
