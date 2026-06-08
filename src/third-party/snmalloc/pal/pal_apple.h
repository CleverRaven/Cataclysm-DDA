#pragma once

#ifdef __APPLE__

#  include "pal_bsd.h"

#  include <CommonCrypto/CommonRandom.h>
#  include <errno.h>
#  include <mach/mach_init.h>
#  include <mach/mach_vm.h>
#  include <mach/vm_statistics.h>
#  include <mach/vm_types.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <sys/mman.h>
#  include <unistd.h>

#  if __has_include(<AvailabilityMacros.h>) && __has_include(<Availability.h>)
#    include <Availability.h>
#    include <AvailabilityMacros.h>
#    if defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && \
      defined(MAC_OS_X_VERSION_14_4)
#      if __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_14_4
#        define SNMALLOC_APPLE_HAS_OS_SYNC_WAIT_ON_ADDRESS
#      endif
#    endif
#  endif

namespace snmalloc
{
#  ifdef SNMALLOC_APPLE_HAS_OS_SYNC_WAIT_ON_ADDRESS
  // For macos 14.4+, we use os_sync_wait_on_address and friends. It is
  // available as a part of stable API, and the usage is more straightforward.
  extern "C" int os_sync_wait_on_address(
    void* addr, uint64_t value, size_t size, uint32_t flags);

  extern "C" int
  os_sync_wake_by_address_any(void* addr, size_t size, uint32_t flags);

  extern "C" int
  os_sync_wake_by_address_all(void* addr, size_t size, uint32_t flags);
#  else
  // For platforms before macos 14.4, we use __ulock_wait and friends. It is
  // available since macos 10.12.
  extern "C" int
  __ulock_wait(uint32_t lock_type, void* addr, uint64_t value, uint32_t);

  extern "C" int __ulock_wake(uint32_t lock_type, void* addr, uint64_t);
#  endif

  /**
   * PAL implementation for Apple systems (macOS, iOS, watchOS, tvOS...).
   */
  template<uint8_t PALAnonID = PALAnonDefaultID>
  class PALApple : public PALBSD<PALApple<>>
  {
  public:
    /**
     * The features exported by this PAL.
     */
    static constexpr uint64_t pal_features =
      AlignedAllocation | LazyCommit | Entropy | Time | WaitOnAddress;

    /*
     * `page_size`
     *
     * On 64-bit ARM platforms, the page size (for user-space) is 16KiB.
     * Otherwise (e.g. x86_64) it is 4KiB.
     *
     * macOS on Apple Silicon ARM does support 4KiB pages, but they are reserved
     * for "exotic" processes (i.e. Rosetta 2) and kernel-space. Using 4KiB
     * pages from user-space in "native" (non-translated) processes is incorrect
     * and will cause bugs.
     *
     * However, Apple's 64-bit embedded ARM-based platforms (phones, pads, tvs)
     * do not support 4KiB pages.
     *
     */
    static constexpr size_t page_size = Aal::aal_name == ARM ? 0x4000 : 0x1000;

    static constexpr size_t minimum_alloc_size = page_size;

    /*
     * Memory Tag
     *
     * A memory tag is an 8-bit value that denotes auxillary "type information"
     * of a vm region. This tag can be used for marking memory for profiling and
     * debugging, or instructing the kernel to perform tag-specific behavior.
     * (E.g. VM_MEMORY_MALLOC{_*} is reused by default, unless it is no longer
     * in its "original state". See `vm_map_entry_is_reusable` in
     * `osfmk/vm/vm_map.c` for more details of this behavior.)
     *
     * Memory tags are encoded using `VM_MAKE_TAG(tag_value)`, and can be passed
     * to the kernel by either `mmap` or `mach_vm_map`:
     * 1. `fd` argument of `mmap`.
     * 2. `flags` argument of `mach_vm_map`.
     *
     * There are currently 4 categories of memory tags:
     *
     * 1. Reserved: [0, 39]. Typically used for Apple libraries and services.
     * Use may trigger undocumented kernel-based behavior.
     *
     * 2. Defined "placeholders": [39, 98]. Typically used for Apple libraries
     * and services.
     *
     * 3. Undefined "placeholders": [99, 239]. Unallocated by Apple. Typically
     * used for libraries. (E.g. LLVM sanitizers use 99.)
     *
     * 4. Application specific: [240, 255]
     *
     * See <mach/vm_statistics.h> for more details about memory tags and their
     * uses.
     *
     * In the future, we may switch our default memory tag from "category 4" to
     * "category 3", thereby affording us a "well-known" memory tag that can be
     * easily identified in tools such as vmmap(1) or Instruments.
     *
     */

    // Encoded memory tag passed to `mmap`.
    static constexpr int anonymous_memory_fd =
      int(VM_MAKE_TAG(uint32_t(PALAnonID)));

    // Encoded memory tag passed to `mach_vm_map`.
    static constexpr int default_mach_vm_map_flags =
      int(VM_MAKE_TAG(uint32_t(PALAnonID)));

    /**
     * Notify platform that we will not be using these pages.
     *
     * We deviate from `PALBSD::notify_not_using` b/c `MADV_FREE` does not
     * behave as expected on Apple platforms. The pages are never marked as
     * "reusable" by the kernel and this can be observed through profiling. E.g.
     * at least ~75% to ~90% less dirty memory is used by `func-malloc-16`
     * (observed on x86_64).
     *
     * Apple's own malloc implementation as well as many ports for Apple
     * Operating Systems use MADV_FREE_REUS{E, ABLE} instead of MADV_FREE. See:
     *  https://opensource.apple.com/source/libmalloc/libmalloc-53.1.1/src/magazine_malloc.c.auto.html
     *  https://bugs.chromium.org/p/chromium/issues/detail?id=713892
     *
     */
    static void notify_not_using(void* p, size_t size) noexcept
    {
      SNMALLOC_ASSERT(is_aligned_block<page_size>(p, size));

      if constexpr (Debug)
        memset(p, 0x5a, size);

      // `MADV_FREE_REUSABLE` can only be applied to writable pages,
      // otherwise it's an error.
      //
      // `mach_vm_behavior_set` is observably slower in benchmarks.
      //
      // macOS 11 Big Sur may behave in an undocumented manner.
      while (madvise(p, size, MADV_FREE_REUSABLE) == -1 && errno == EAGAIN)
        ;

      if constexpr (mitigations(pal_enforce_access))
      {
        // This must occur after `MADV_FREE_REUSABLE`.
        //
        // `mach_vm_protect` is observably slower in benchmarks.
        mprotect(p, size, PROT_NONE);
      }
    }

    /**
     * Notify platform that we will be using these pages.
     *
     * We deviate from `PALPOSIX::notify_using` for three reasons:
     * 1. `MADV_FREE_REUSABLE` must be paired with `MADV_FREE_REUSE`.
     * 2. `MADV_FREE_REUSE` must only be applied to writable pages, otherwise it
     * is an error.
     * 3. `PALPOSIX::notify_using` will apply mprotect(PROT_READ|PROT_WRITE) to
     * the pages, and then call `PALPOSIX::zero<true>` (overwrite pages with
     * mmap, and if mmap fails call bzero on the pages). This is very wasteful;
     * if mmap succeeds we do not need to change the permissions of the pages
     * since it is done during mmap. Instead `PALApple::notify_using` will try
     * to overwrite with mmap, and if mmap fails, apply
     * mprotect(PROT_READ|PROT_WRITE), madvise(MADV_REUSE), and finally call
     * `bzero` to clear the pages.
     *
     * Currently, `PALPOSIX::zero` will call `bzero` on the pages that `mmap`
     * failed to overwrite. In the future, we should duplicate the behavior of
     * `PALWindows` and abort the process if a `mmap` call fails. But for now we
     * are going to be consistent with the behavior of the other POSIX PAL
     * implementations.
     *
     */
    template<ZeroMem zero_mem>
    static bool notify_using(void* p, size_t size) noexcept
    {
      KeepErrno e;
      SNMALLOC_ASSERT(
        is_aligned_block<page_size>(p, size) || (zero_mem == NoZero));

      if constexpr (zero_mem == YesZero)
      {
        void* r = mmap(
          p,
          size,
          PROT_READ | PROT_WRITE,
          MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
          anonymous_memory_fd,
          0);

        if (SNMALLOC_LIKELY(r != MAP_FAILED))
        {
          return true;
        }
      }

      if constexpr (mitigations(pal_enforce_access))
      {
        // Mark pages as writable for `madvise` below.
        //
        // `mach_vm_protect` is observably slower in benchmarks.
        mprotect(p, size, PROT_READ | PROT_WRITE);
      }

      // `MADV_FREE_REUSE` can only be applied to writable pages,
      // otherwise it's an error.
      //
      // `mach_vm_behavior_set` is observably slower in benchmarks.
      //
      // macOS 11 Big Sur may behave in an undocumented manner.
      while (madvise(p, size, MADV_FREE_REUSE) == -1 && errno == EAGAIN)
        ;

      if constexpr (zero_mem == YesZero)
      {
        ::bzero(p, size);
      }
      return true;
    }

    // Apple's `mmap` doesn't support user-specified alignment and only
    // guarantees mappings are aligned to the system page size, so we use
    // `mach_vm_map` instead.
    template<bool state_using>
    static void* reserve_aligned(size_t size) noexcept
    {
      SNMALLOC_ASSERT(bits::is_pow2(size));
      SNMALLOC_ASSERT(size >= minimum_alloc_size);

      // mask has least-significant bits set
      mach_vm_offset_t mask = size - 1;

      int flags = VM_FLAGS_ANYWHERE | default_mach_vm_map_flags;

      // must be initialized to 0 or addr is interepreted as a lower-bound.
      mach_vm_address_t addr = 0;

      vm_prot_t prot = (state_using || !mitigations(pal_enforce_access)) ?
        VM_PROT_READ | VM_PROT_WRITE :
        VM_PROT_NONE;

      kern_return_t kr = mach_vm_map(
        mach_task_self(),
        &addr,
        size,
        mask,
        flags,
        MEMORY_OBJECT_NULL,
        0,
        TRUE,
        prot,
        VM_PROT_READ | VM_PROT_WRITE,
        VM_INHERIT_COPY);

      if (SNMALLOC_UNLIKELY(kr != KERN_SUCCESS))
      {
        return nullptr;
      }

      return reinterpret_cast<void*>(addr);
    }

    /**
     * Source of Entropy
     *
     * Apple platforms have a working `getentropy(2)` implementation.
     * However, it is not allowed on the App Store and Apple actively
     * discourages its use. The substitutes `arc4random_buf(3)`,
     * `CCRandomGenerateBytes`, and `SecRandomCopyBytes` are recommended
     * instead.
     *
     * `CCRandomGenerateBytes` was selected because:
     *
     * 1. The implementation of `arc4random_buf(3)` differs from its
     * documentation. It is documented to never fail, yet its'
     * implementation can fail silently: it calls the function
     * `ccrng_generate`, but ignores the error case.
     * `CCRandomGenerateBytes` is built on the same function, but can return an
     * error code in case of failure. See:
     *      https://opensource.apple.com/source/Libc/Libc-1439.40.11/gen/FreeBSD/arc4random.c.auto.html
     *      https://opensource.apple.com/source/CommonCrypto/CommonCrypto-60061/include/CommonRandom.h.auto.html
     *
     * 2. `SecRandomCopyBytes` introduces a dependency on `Security.framework`.
     * `CCRandomGenerateBytes` introduces no new dependencies.
     *
     */
    static uint64_t get_entropy64()
    {
      uint64_t result;
      if (
        CCRandomGenerateBytes(
          reinterpret_cast<void*>(&result), sizeof(result)) != kCCSuccess)
      {
        error("Failed to get system randomness");
      }

      return result;
    }

    using WaitingWord = uint32_t;
#  ifndef SNMALLOC_APPLE_HAS_OS_SYNC_WAIT_ON_ADDRESS
    static constexpr uint32_t UL_COMPARE_AND_WAIT = 0x0000'0001;
    static constexpr uint32_t ULF_NO_ERRNO = 0x0100'0000;
    static constexpr uint32_t ULF_WAKE_ALL = 0x0000'0100;
#  endif

    template<class T>
    static void wait_on_address(stl::Atomic<T>& addr, T expected)
    {
      [[maybe_unused]] int errno_backup = errno;
      while (addr.load(stl::memory_order_relaxed) == expected)
      {
#  ifdef SNMALLOC_APPLE_HAS_OS_SYNC_WAIT_ON_ADDRESS
        os_sync_wait_on_address(
          &addr, static_cast<uint64_t>(expected), sizeof(T), 0);
#  else
        __ulock_wait(
          UL_COMPARE_AND_WAIT | ULF_NO_ERRNO,
          &addr,
          static_cast<uint64_t>(expected),
          0);
#  endif
        errno = errno_backup;
      }
    }

    template<class T>
    static void notify_one_on_address(stl::Atomic<T>& addr)
    {
#  ifdef SNMALLOC_APPLE_HAS_OS_SYNC_WAIT_ON_ADDRESS
      os_sync_wake_by_address_any(&addr, sizeof(T), 0);
#  else
      // __ulock_wake can get interrupted, so retry until either waking up a
      // waiter or failing because there are no waiters (ENOENT).
      for (;;)
      {
        int ret = __ulock_wake(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, &addr, 0);
        if (ret >= 0 || ret == -ENOENT)
          return;
      }
#  endif
    }

    template<class T>
    static void notify_all_on_address(stl::Atomic<T>& addr)
    {
#  ifdef SNMALLOC_APPLE_HAS_OS_SYNC_WAIT_ON_ADDRESS
      os_sync_wake_by_address_all(&addr, sizeof(T), 0);
#  else
      // __ulock_wake can get interrupted, so retry until either waking up a
      // waiter or failing because there are no waiters (ENOENT).
      for (;;)
      {
        int ret = __ulock_wake(
          UL_COMPARE_AND_WAIT | ULF_NO_ERRNO | ULF_WAKE_ALL, &addr, 0);
        if (ret >= 0 || ret == -ENOENT)
          return;
      }
#  endif
    }
  };
} // namespace snmalloc
#endif
