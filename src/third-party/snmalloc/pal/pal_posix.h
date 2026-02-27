#pragma once

#include "../aal/aal.h"
#include "pal_timer_default.h"
#if defined(SNMALLOC_BACKTRACE_HEADER)
#  include SNMALLOC_BACKTRACE_HEADER
#endif
#include "snmalloc/stl/array.h"
#include "snmalloc/stl/utility.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#if __has_include(<sys/random.h>)
#  include <sys/random.h>
#endif

namespace snmalloc
{
  /**
   * Platform abstraction layer for generic POSIX systems.
   *
   * This provides the lowest common denominator for POSIX systems.  It should
   * work on pretty much any POSIX system, but won't necessarily be the most
   * efficient implementation.  Subclasses should provide more efficient
   * implementations using platform-specific functionality.
   *
   * The first template parameter for this is the subclass and is used for
   * explicit up casts to allow this class to call non-virtual methods on the
   * templated version.  The next two allow subclasses to provide `writev` and
   * `fsync` implementations that bypass any libc machinery that might not be
   * working when an early-malloc error appears.
   */
  template<class OS, auto writev = ::writev, auto fsync = ::fsync>
  class PALPOSIX : public PalTimerDefaultImpl<PALPOSIX<OS>>
  {
    /**
     * Helper class to access the `default_mmap_flags` field of `OS` if one
     * exists or a default value if not.  This provides the default version,
     * which is used if `OS::default_mmap_flags` does not exist.
     */
    template<typename T, typename = int>
    struct DefaultMMAPFlags
    {
      /**
       * If `OS::default_mmap_flags` does not exist, use 0.  This value is
       * or'd with the other mmap flags and so a value of 0 is a no-op.
       */
      static const int flags = 0;
    };

    /**
     * Helper class to access the `default_mmap_flags` field of `OS` if one
     * exists or a default value if not.  This provides the version that
     * accesses the field, allowing other PALs to provide extra arguments to
     * the `mmap` calls used here.
     */
    template<typename T>
    struct DefaultMMAPFlags<T, decltype((void)T::default_mmap_flags, 0)>
    {
      static const int flags = T::default_mmap_flags;
    };

    /**
     * Helper class to allow `OS` to provide the file descriptor used for
     * anonymous memory. This is the default version, which provides the POSIX
     * default of -1.
     */
    template<typename T, typename = int>
    struct AnonFD
    {
      /**
       * If `OS::anonymous_memory_fd` does not exist, use -1.  This value is
       * defined by POSIX.
       */
      static const int fd = -1;
    };

    /**
     * Helper class to allow `OS` to provide the file descriptor used for
     * anonymous memory. This exposes the `anonymous_memory_fd` field in `OS`.
     */
    template<typename T>
    struct AnonFD<T, decltype((void)T::anonymous_memory_fd, 0)>
    {
      /**
       * The PAL's provided file descriptor for anonymous memory.  This is
       * used, for example, on Apple platforms, which use the file descriptor
       * in a `MAP_ANONYMOUS` mapping to encode metadata about the owner of the
       * mapping.
       */
      static const int fd = T::anonymous_memory_fd;
    };

  protected:
    /**
     * A RAII class to capture and restore errno
     */
    class KeepErrno
    {
      int cached_errno;

    public:
      KeepErrno() : cached_errno(errno) {}

      ~KeepErrno()
      {
        errno = cached_errno;
      }
    };

  public:
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     *
     * POSIX systems are assumed to support lazy commit. The build system checks
     * getentropy is available, only then this PAL supports Entropy.
     */
    static constexpr uint64_t pal_features = LazyCommit | Time
#if defined(SNMALLOC_PLATFORM_HAS_GETENTROPY)
      | Entropy
#endif
      ;
#ifdef SNMALLOC_PAGESIZE
    static_assert(
      bits::is_pow2(SNMALLOC_PAGESIZE), "Page size must be a power of 2");
    static constexpr size_t page_size = SNMALLOC_PAGESIZE;
#elif defined(PAGESIZE)
    static constexpr size_t page_size =
      bits::max(Aal::smallest_page_size, static_cast<size_t>(PAGESIZE));
#else
    static constexpr size_t page_size = Aal::smallest_page_size;
#endif

    /**
     * Address bits are potentially mediated by some POSIX OSes, but generally
     * default to the architecture's.
     *
     * Unlike the AALs, which are composited by explicitly delegating to their
     * template parameters and so play a SFINAE-based game to achieve similar
     * ends, for the PALPOSIX<> classes we instead use more traditional
     * inheritance (e.g., PALLinux is subtype of PALPOSIX<PALLinux>) and so we
     * can just use that mechanism here, too.
     */
    static constexpr size_t address_bits = Aal::address_bits;

    static void print_stack_trace()
    {
      // TODO: the backtrace mechanism does not yet work on CHERI, and causes
      // tests which expect to be able to hook abort() to fail.  Skip it until
      // https://github.com/CTSRD-CHERI/cheribsd/issues/962 is fixed.
#if defined(SNMALLOC_BACKTRACE_HEADER) && !defined(__CHERI_PURE_CAPABILITY__)
      constexpr int SIZE = 1024;
      void* buffer[SIZE];
      auto nptrs = backtrace(buffer, SIZE);
      backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
      UNUSED(write(STDERR_FILENO, "\n", 1));
      UNUSED(fsync(STDERR_FILENO));
#endif
    }

    /**
     * Report a message to standard error, followed by a newline.
     */
    static void message(const char* const str) noexcept
    {
      // We don't want logging to affect the errno behaviour of the program.
      auto hold = KeepErrno();

      void* nl = const_cast<char*>("\n");
      struct iovec iov[] = {{const_cast<char*>(str), strlen(str)}, {nl, 1}};
      UNUSED(writev(STDERR_FILENO, iov, sizeof(iov) / sizeof(struct iovec)));
      UNUSED(fsync(STDERR_FILENO));
    }

    /**
     * Report a fatal error an exit.
     */
    [[noreturn]] static void error(const char* const str) noexcept
    {
      /// by this part, the allocator is failed; so we cannot assume
      /// subsequent allocation will work.
      /// @attention: since the program is failing, we do not guarantee that
      /// previous bytes in stdout will be flushed
      void* nl = const_cast<char*>("\n");
      struct iovec iov[] = {
        {nl, 1}, {const_cast<char*>(str), strlen(str)}, {nl, 1}};
      UNUSED(writev(STDERR_FILENO, iov, sizeof(iov) / sizeof(struct iovec)));
      UNUSED(fsync(STDERR_FILENO));
      print_stack_trace();
      abort();
    }

    /**
     * Notify platform that we will not be using these pages.
     *
     * This does nothing in a generic POSIX implementation.  Most POSIX systems
     * provide an `madvise` call that can be used to return pages to the OS in
     * high memory pressure conditions, though on Linux this seems to impose
     * too much of a performance penalty.
     */
    static void notify_not_using(void* p, size_t size) noexcept
    {
      SNMALLOC_ASSERT(is_aligned_block<OS::page_size>(p, size));

      if constexpr (mitigations(pal_enforce_access))
      {
        // Fill memory so that when we switch the pages back on we don't make
        // assumptions on the content.
        if constexpr (Debug)
          memset(p, 0x5a, size);

        mprotect(p, size, PROT_NONE);
      }
      else
      {
        UNUSED(p, size);
      }
    }

    /**
     * Notify platform that we will be using these pages.
     *
     * On POSIX platforms, lazy commit means that this is a no-op, unless we
     * are also zeroing the pages in which case we call the platform's `zero`
     * function, or we have initially mapped the pages as PROT_NONE.
     */
    template<ZeroMem zero_mem>
    static bool notify_using(void* p, size_t size) noexcept
    {
      SNMALLOC_ASSERT(
        is_aligned_block<OS::page_size>(p, size) || (zero_mem == NoZero));

      if constexpr (mitigations(pal_enforce_access))
        mprotect(p, size, PROT_READ | PROT_WRITE);
      else
      {
        UNUSED(p, size);
      }

      if constexpr (zero_mem == YesZero)
        zero<true>(p, size);

      return true;
    }

    /**
     * Notify platform that we will be using these pages for reading.
     *
     * On POSIX platforms, lazy commit means that this is a no-op, unless
     * we have initially mapped the pages as PROT_NONE.
     */
    static bool notify_using_readonly(void* p, size_t size) noexcept
    {
      SNMALLOC_ASSERT(is_aligned_block<OS::page_size>(p, size));

      if constexpr (mitigations(pal_enforce_access))
        mprotect(p, size, PROT_READ);
      else
      {
        UNUSED(p, size);
      }

      return true;
    }

    /**
     * OS specific function for zeroing memory.
     *
     * The generic POSIX implementation uses mmap to map anonymous memory over
     * the range for ranges larger than a page.  The underlying OS is assumed
     * to provide new CoW copies of the zero page.
     *
     * Note: On most systems it is faster for a single page to zero the memory
     * explicitly than do this, we should probably tweak the threshold for
     * calling bzero at some point.
     */
    template<bool page_aligned = false>
    static void zero(void* p, size_t size) noexcept
    {
      if (page_aligned || is_aligned_block<OS::page_size>(p, size))
      {
        SNMALLOC_ASSERT(is_aligned_block<OS::page_size>(p, size));

        /*
         * If mmap fails, we're going to fall back to zeroing the memory
         * ourselves, which is not stellar, but correct.  However, mmap() will
         * have has left errno nonzero in an effort to explain its MAP_FAILED
         * result.  Capture its current value and restore it at the end of this
         * block.
         */
        auto hold = KeepErrno();

        void* r = mmap(
          p,
          size,
          PROT_READ | PROT_WRITE,
          MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | DefaultMMAPFlags<OS>::flags,
          AnonFD<OS>::fd,
          0);

        if (r != MAP_FAILED)
          return;
      }

      bzero(p, size);
    }

    /**
     * Extension point to allow subclasses to provide extra mmap flags.  The
     * argument indicates whether the memory should be in use or not.
     */
    static int extra_mmap_flags(bool)
    {
      return 0;
    }

    /**
     * Reserve memory.
     *
     * POSIX platforms support lazy commit, and so this also puts the memory in
     * the lazy commit state (i.e. pages will be allocated on first use).
     *
     * POSIX does not define a portable interface for specifying alignment
     * greater than a page.
     */
    static void* reserve(size_t size) noexcept
    {
      // If enforcing access, map pages initially as None, and then
      // add permissions as required.  Otherwise, immediately give all
      // access as this is the most efficient to implement.
      auto prot =
        mitigations(pal_enforce_access) ? PROT_NONE : PROT_READ | PROT_WRITE;

      void* p = mmap(
        nullptr,
        size,
        prot,
        MAP_PRIVATE | MAP_ANONYMOUS | DefaultMMAPFlags<OS>::flags |
          OS::extra_mmap_flags(false),
        AnonFD<OS>::fd,
        0);

      if (p != MAP_FAILED)
      {
#ifdef SNMALLOC_TRACING
        snmalloc::message<1024>("Pal_posix reserved: {} ({})", p, size);
#endif
        return p;
      }

      return nullptr;
    }

    /**
     * Source of Entropy
     *
     * This is a default that works on many POSIX platforms.
     */
    static uint64_t get_entropy64()
    {
      if constexpr (!pal_supports<Entropy, OS>)
      {
        // Derived Pal does not provide entropy.
        return 0;
      }
      else if constexpr (OS::get_entropy64 != get_entropy64)
      {
        // Derived Pal has provided a custom definition.
        return OS::get_entropy64();
      }
      else
      {
#ifdef SNMALLOC_PLATFORM_HAS_GETENTROPY
        uint64_t result;
        if (getentropy(&result, sizeof(result)) != 0)
          error("Failed to get system randomness");
        return result;
#endif
      }
      error("Entropy requested on platform that does not provide entropy");
    }

    static uint64_t internal_time_in_ms()
    {
      auto hold = KeepErrno();

      struct timespec ts;
      if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
      {
        error("Failed to get time");
      }

      return (static_cast<uint64_t>(ts.tv_sec) * 1000) +
        (static_cast<uint64_t>(ts.tv_nsec) / 1000000);
    }

    static uint64_t tick()
    {
      if constexpr (
        (Aal::aal_features & NoCpuCycleCounters) != NoCpuCycleCounters)
      {
        return Aal::tick();
      }
      else
      {
        auto hold = KeepErrno();

        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
        {
          error("Failed to get monotonic time");
        }

        return (static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000) +
          static_cast<uint64_t>(ts.tv_nsec);
      }
    }

    static uint64_t dev_urandom()
    {
      union
      {
        uint64_t result;
        char buffer[sizeof(uint64_t)];
      };

      ssize_t ret;
      int flags = O_RDONLY;
#if defined(O_CLOEXEC)
      flags |= O_CLOEXEC;
#endif
      auto fd = open("/dev/urandom", flags, 0);
      if (fd > 0)
      {
        auto current = stl::begin(buffer);
        auto target = stl::end(buffer);
        while (auto length = static_cast<size_t>(target - current))
        {
          ret = read(fd, current, length);
          if (ret <= 0)
          {
            if (errno != EAGAIN && errno != EINTR)
            {
              break;
            }
          }
          else
          {
            current += ret;
          }
        }
        ret = close(fd);
        SNMALLOC_ASSERT(0 == ret);
        if (SNMALLOC_LIKELY(target == current))
        {
          return result;
        }
      }

      error("Failed to get system randomness");
    }
  };
} // namespace snmalloc
