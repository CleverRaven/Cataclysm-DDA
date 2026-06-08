/**
 * The platform abstraction layer.  This defines an abstraction that exposes
 * services from the operating system or any equivalent environment.
 *
 * It is possible to have multiple PALs in a single snmalloc instance.  For
 * example, one may provide the base operating system functionality, another
 * may hide some of this or provide its own abstractions for sandboxing or
 * isolating heaps.
 *
 * Files in this directory may depend on the architecture abstraction and core
 * layers (`aal` and `ds_core`, respectively) but nothing else in snmalloc.
 */
#pragma once

#include "../aal/aal.h"
#include "pal_concept.h"
#include "pal_consts.h"

// If simulating OE, then we need the underlying platform
#if defined(OPEN_ENCLAVE)
#  include "pal_open_enclave.h"
#endif
#if !defined(OPEN_ENCLAVE) || defined(OPEN_ENCLAVE_SIMULATION)
#  include "pal_apple.h"
#  include "pal_dragonfly.h"
#  include "pal_freebsd.h"
#  include "pal_freebsd_kernel.h"
#  include "pal_haiku.h"
#  include "pal_linux.h"
#  include "pal_netbsd.h"
#  include "pal_openbsd.h"
#  include "pal_solaris.h"
#  include "pal_windows.h"
#endif
#include "pal_noalloc.h"
#include "pal_plain.h"

namespace snmalloc
{
  using DefaultPal =
#if defined(SNMALLOC_MEMORY_PROVIDER)
    SNMALLOC_MEMORY_PROVIDER;
#elif defined(OPEN_ENCLAVE)
    PALOpenEnclave;
#elif defined(_WIN32)
    PALWindows;
#elif defined(__APPLE__)
    PALApple<>;
#elif defined(__linux__)
    PALLinux;
#elif defined(FreeBSD_KERNEL)
    PALFreeBSDKernel;
#elif defined(__FreeBSD__)
    PALFreeBSD;
#elif defined(__HAIKU__)
    PALHaiku;
#elif defined(__NetBSD__)
    PALNetBSD;
#elif defined(__OpenBSD__)
    PALOpenBSD;
#elif defined(__sun)
    PALSolaris;
#elif defined(__DragonFly__)
    PALDragonfly;
#else
#  error Unsupported platform
#endif

  [[noreturn]] SNMALLOC_SLOW_PATH inline void error(const char* const str)
  {
    DefaultPal::error(str);
  }

  // Used to keep Superslab metadata committed.
  static constexpr size_t OS_PAGE_SIZE = DefaultPal::page_size;

  /**
   * Perform platform-specific adjustment of return pointers.
   *
   * This is here, rather than in every PAL proper, merely to minimize
   * disruption to PALs for platforms that do not support StrictProvenance AALs.
   */
  template<
    typename PAL = DefaultPal,
    typename AAL = Aal,
    typename T,
    SNMALLOC_CONCEPT(capptr::IsBound) B>
  static inline typename stl::enable_if_t<
    !aal_supports<StrictProvenance, AAL>,
    CapPtr<T, capptr::user_address_control_type<B>>>
  capptr_to_user_address_control(CapPtr<T, B> p)
  {
    return CapPtr<T, capptr::user_address_control_type<B>>::unsafe_from(
      p.unsafe_ptr());
  }

  template<
    typename PAL = DefaultPal,
    typename AAL = Aal,
    typename T,
    SNMALLOC_CONCEPT(capptr::IsBound) B>
  static SNMALLOC_FAST_PATH typename stl::enable_if_t<
    aal_supports<StrictProvenance, AAL>,
    CapPtr<T, capptr::user_address_control_type<B>>>
  capptr_to_user_address_control(CapPtr<T, B> p)
  {
    return PAL::capptr_to_user_address_control(p);
  }

  /**
   * A convenience wrapper that avoids the need to litter unsafe accesses with
   * every call to PAL::zero.
   *
   * We do this here rather than plumb CapPtr further just to minimize
   * disruption and avoid code bloat.  This wrapper ought to compile down to
   * nothing if SROA is doing its job.
   */
  template<
    typename PAL,
    bool page_aligned = false,
    typename T,
    SNMALLOC_CONCEPT(capptr::IsBound) B>
  static SNMALLOC_FAST_PATH void pal_zero(CapPtr<T, B> p, size_t sz)
  {
    static_assert(
      !page_aligned || B::spatial >= capptr::dimension::Spatial::Chunk);
    PAL::template zero<page_aligned>(p.unsafe_ptr(), sz);
  }

  static_assert(
    bits::is_pow2(OS_PAGE_SIZE), "OS_PAGE_SIZE must be a power of two");
  static_assert(
    OS_PAGE_SIZE % Aal::smallest_page_size == 0,
    "The smallest architectural page size must divide OS_PAGE_SIZE");

  // Some system headers (e.g. Linux' sys/user.h, FreeBSD's machine/param.h)
  // define `PAGE_SIZE` as a macro, while others (e.g. macOS 11's
  // mach/machine/vm_param.h) define `PAGE_SIZE` as an extern. We don't use
  // `PAGE_SIZE` as our variable name, to avoid conflicts, but if we do see a
  // macro definition evaluates to a constant then check that our value matches
  // the platform's expected value.
#ifdef PAGE_SIZE
  static_assert(
#  if __has_builtin(__builtin_constant_p)
    !__builtin_constant_p(PAGE_SIZE) || (PAGE_SIZE == OS_PAGE_SIZE),
#  else
    true,
#  endif
    "Page size from system header does not match snmalloc config page size.");
#endif

  /**
   * Report a fatal error via a PAL-specific error reporting mechanism.  This
   * takes a format string and a set of arguments.  The format string indicates
   * the remaining arguments with "{}".  This could be extended later to
   * support indexing fairly easily, if we ever want to localise these error
   * messages.
   *
   * The following are supported as arguments:
   *
   *  - Characters (`char`), printed verbatim.
   *  - Strings Literals (`const char*` or `const char[]`), printed verbatim.
   *  - Raw pointers (void*), printed as hex strings.
   *  - Integers (convertible to `size_t`), printed as hex strings.
   *
   *  These types should be sufficient for allocator-related error messages.
   */
  template<size_t BufferSize, typename... Args>
  [[noreturn]] inline void report_fatal_error(Args... args)
  {
    MessageBuilder<BufferSize> msg{stl::forward<Args>(args)...};
    DefaultPal::error(msg.get_message());
  }

  template<size_t BufferSize, typename... Args>
  inline void message(Args... args)
  {
    MessageBuilder<BufferSize> msg{stl::forward<Args>(args)...};
    MessageBuilder<BufferSize> msg_tid{
      "{}: {}", debug_get_tid(), msg.get_message()};
    DefaultPal::message(msg_tid.get_message());
  }
} // namespace snmalloc
