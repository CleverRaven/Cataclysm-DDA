#pragma once

#if defined(__FreeBSD__) && !defined(_KERNEL)
#  include "pal_bsd_aligned.h"

// On CHERI platforms, we need to know the value of CHERI_PERM_SW_VMEM.
// This pollutes the global namespace a little, sadly, but I think only with
// symbols that begin with CHERI_, which is as close to namespaces as C offers.
#  if defined(__CHERI_PURE_CAPABILITY__)
#    include <cheri/cherireg.h>
#    if !defined(CHERI_PERM_SW_VMEM)
#      define CHERI_PERM_SW_VMEM CHERI_PERM_CHERIABI_VMMAP
#    endif
#  endif

#  include <sys/umtx.h>

/**
 * Direct system-call wrappers so that we can skip libthr interception, which
 * won't work if malloc is broken.
 * @{
 */
extern "C" ssize_t __sys_writev(int fd, const struct iovec* iov, int iovcnt);
extern "C" int __sys_fsync(int fd);

/// @}

namespace snmalloc
{
  /**
   * FreeBSD-specific platform abstraction layer.
   *
   * This adds FreeBSD-specific aligned allocation to the generic BSD
   * implementation.
   */
  class PALFreeBSD
  : public PALBSD_Aligned<PALFreeBSD, __sys_writev, __sys_fsync>
  {
  public:
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     *
     * The FreeBSD PAL does not currently add any features beyond those of a
     * generic BSD with support for arbitrary alignment from `mmap`.  This
     * field is declared explicitly to remind anyone modifying this class to
     * add new features that they should add any required feature flags.
     */
    static constexpr uint64_t pal_features =
      PALBSD_Aligned::pal_features | CoreDump | WaitOnAddress;

    /**
     * FreeBSD uses atypically small address spaces on its 64 bit RISC machines.
     * Problematically, these are so small that if we used the default
     * address_bits (48), we'd try to allocate the whole AS (or larger!) for the
     * Pagemap itself!
     */
    static constexpr size_t address_bits = (Aal::bits == 32) ?
      Aal::address_bits :
      (Aal::aal_name == RISCV ? 38 : Aal::address_bits);

    // TODO, if we ever backport to MIPS, this should yield 39 there.

    /**
     * Extra mmap flags.  Exclude mappings from core files if they are
     * read-only or pure reservations.
     */
    static int extra_mmap_flags(bool state_using)
    {
      return state_using ? 0 : MAP_NOCORE;
    }

    /**
     * Notify platform that we will not be using these pages.
     *
     * We use the `MADV_FREE` flag to `madvise`. This allows the system to
     * discard the page and replace it with a CoW mapping of the zero page.
     */
    static void notify_not_using(void* p, size_t size) noexcept
    {
      SNMALLOC_ASSERT(is_aligned_block<page_size>(p, size));

      if constexpr (Debug)
        memset(p, 0x5a, size);

      madvise(p, size, MADV_FREE);

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
      madvise(p, size, MADV_CORE);
    }

    /**
     * Notify platform that these pages should not be included in a core dump.
     */
    static void notify_do_not_dump(void* p, size_t size) noexcept
    {
      madvise(p, size, MADV_NOCORE);
    }

#  if defined(__CHERI_PURE_CAPABILITY__)
    static_assert(
      aal_supports<StrictProvenance>,
      "CHERI purecap support requires StrictProvenance AAL");

    /**
     * On CheriBSD, exporting a pointer means stripping it of the authority to
     * manage the address space it references by clearing the SW_VMEM
     * permission bit.
     */
    template<typename T, SNMALLOC_CONCEPT(capptr::IsBound) B>
    static SNMALLOC_FAST_PATH CapPtr<T, capptr::user_address_control_type<B>>
    capptr_to_user_address_control(CapPtr<T, B> p)
    {
      if constexpr (Aal::aal_cheri_features & Aal::AndPermsTrapsUntagged)
      {
        if (p == nullptr)
        {
          return nullptr;
        }
      }
      return CapPtr<T, capptr::user_address_control_type<B>>::unsafe_from(
        __builtin_cheri_perms_and(
          p.unsafe_ptr(), ~static_cast<unsigned int>(CHERI_PERM_SW_VMEM)));
    }
#  endif

    using WaitingWord = unsigned int;

    template<typename T>
    static void wait_on_address(stl::Atomic<T>& addr, T expected)
    {
      static_assert(
        sizeof(T) == sizeof(WaitingWord) && alignof(T) == alignof(WaitingWord),
        "T must be the same size and alignment as WaitingWord");
      int backup = errno;
      while (addr.load(stl::memory_order_relaxed) == expected)
      {
        _umtx_op(
          &addr,
          UMTX_OP_WAIT_UINT_PRIVATE,
          static_cast<unsigned long>(expected),
          nullptr,
          nullptr);
      }
      errno = backup;
    }

    template<typename T>
    static void notify_one_on_address(stl::Atomic<T>& addr)
    {
      static_assert(
        sizeof(T) == sizeof(WaitingWord) && alignof(T) == alignof(WaitingWord),
        "T must be the same size and alignment as WaitingWord");
      _umtx_op(&addr, UMTX_OP_WAKE_PRIVATE, 1, nullptr, nullptr);
    }

    template<typename T>
    static void notify_all_on_address(stl::Atomic<T>& addr)
    {
      static_assert(
        sizeof(T) == sizeof(WaitingWord) && alignof(T) == alignof(WaitingWord),
        "T must be the same size and alignment as WaitingWord");
      _umtx_op(
        &addr,
        UMTX_OP_WAKE_PRIVATE,
        static_cast<unsigned long>(INT_MAX),
        nullptr,
        nullptr);
    }
  };
} // namespace snmalloc
#endif
