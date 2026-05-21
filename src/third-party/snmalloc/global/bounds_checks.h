#pragma once
#include "globalalloc.h"
#include "threadalloc.h"

namespace snmalloc
{
  /**
   * Should we check loads?  This defaults to on in debug builds, off in
   * release (store-only checks) and can be overridden by defining the macro
   * `SNMALLOC_CHECK_LOADS` to true or false.
   */
  static constexpr bool CheckReads =
#ifdef SNMALLOC_CHECK_LOADS
    SNMALLOC_CHECK_LOADS
#else
    Debug
#endif
    ;

  /**
   * Should we fail fast when we encounter an error?  With this set to true, we
   * just issue a trap instruction and crash the process once we detect an
   * error. With it set to false we print a helpful error message and then crash
   * the process.  The process may be in an undefined state by the time the
   * check fails, so there are potentially security implications to turning this
   * off. It defaults to false and can be overridden by defining the macro
   * `SNMALLOC_FAIL_FAST` to true.
   *
   * Current default to true will help with adoption experience.
   */
  static constexpr bool FailFast =
#ifdef SNMALLOC_FAIL_FAST
    SNMALLOC_FAIL_FAST
#else
    false
#endif
    ;

  /**
   * Report an error message for a failed bounds check and then abort the
   * program.
   * `p` is the input pointer and `len` is the offset from this pointer of the
   * bounds.  `msg` is the message that will be reported along with the
   * start and end of the real object's bounds.
   *
   * Note that this function never returns.  We do not mark it [[NoReturn]]
   * so as to generate better code, because [[NoReturn]] prevents tailcails
   * in GCC and Clang.
   *
   * The function claims to return a FakeReturn, this is so it can be tail
   * called where the bound checked function returns a value, for instance, in
   * memcpy it is specialised to void*.
   */
  template<typename FakeReturn = void*>
  SNMALLOC_SLOW_PATH SNMALLOC_UNUSED_FUNCTION inline FakeReturn
  report_fatal_bounds_error(const void* ptr, size_t len, const char* msg)
  {
    if constexpr (FailFast)
    {
      UNUSED(ptr, len, msg);
      SNMALLOC_FAST_FAIL();
    }
    else
    {
      void* p = const_cast<void*>(ptr);

      auto range_end = pointer_offset(p, len);
      auto object_end = external_pointer<OnePastEnd>(p);
      report_fatal_error(
        "Fatal Error!\n{}: \n\trange [{}, {})\n\tallocation [{}, "
        "{})\nrange goes beyond allocation by {} bytes \n",
        msg,
        p,
        range_end,
        external_pointer<Start>(p),
        object_end,
        pointer_diff(object_end, range_end));
    }
  }

  /**
   * Check whether a pointer + length is in the same object as the pointer.
   *
   * Returns the result of the supplied continuation
   *
   * The template parameter indicates whether the check should be performed.  It
   * defaults to true. If it is false, it short cuts to calling the continuation
   * directly.
   */
  template<
    bool PerformCheck = true,
    typename F,
    SNMALLOC_CONCEPT(IsConfig) Config = Config>
  SNMALLOC_FAST_PATH_INLINE auto check_bound(
    const void* ptr, size_t len, const char* msg, F f = []() {})
  {
    if constexpr (PerformCheck)
    {
      if (SNMALLOC_LIKELY(len != 0))
      {
        if (SNMALLOC_UNLIKELY(remaining_bytes<Config>(address_cast(ptr)) < len))
        {
          return report_fatal_bounds_error(ptr, len, msg);
        }
      }
    }
    else
    {
      UNUSED(ptr, len, msg);
    }
    return f();
  }
} // namespace snmalloc
