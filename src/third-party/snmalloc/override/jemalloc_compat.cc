#include "override.h"

#include <errno.h>
#include <limits.h>
#include <string.h>

using namespace snmalloc;

namespace
{
  /**
   * Helper for JEMalloc-compatible non-standard APIs.  These take a flags
   * argument as an `int`.  This class provides a wrapper for extracting the
   * fields embedded in this API.
   */
  class JEMallocFlags
  {
    /**
     * The raw flags.
     */
    int flags;

  public:
    /**
     * Constructor, takes a `flags` parameter from one of the `*allocx()`
     * JEMalloc APIs.
     */
    constexpr JEMallocFlags(int flags) : flags(flags) {}

    /**
     * Jemalloc's *allocx APIs store the alignment in the low 6 bits of the
     * flags, allowing any alignment up to 2^63.
     */
    constexpr int log2align()
    {
      return flags & 0x3f;
    }

    /**
     * Jemalloc's *allocx APIs use bit 6 to indicate whether memory should be
     * zeroed.
     */
    constexpr bool should_zero()
    {
      return (flags & 0x40) == 0x40;
    }

    /**
     * Jemalloc's *allocm APIs use bit 7 to indicate whether reallocation may
     * move.  This is ignored by the `*allocx` functions.
     */
    constexpr bool may_not_move()
    {
      return (flags & 0x80) == 0x80;
    }

    size_t aligned_size(size_t size)
    {
      return ::aligned_size(bits::one_at_bit<size_t>(log2align()), size);
    }
  };

  /**
   * Error codes from Jemalloc 3's experimental API.
   */
  enum JEMalloc3Result
  {
    /**
     * Allocation succeeded.
     */
    allocm_success = 0,

    /**
     * Allocation failed because memory was not available.
     */
    allocm_err_oom = 1,

    /**
     * Reallocation failed because it would have required moving.
     */
    allocm_err_not_moved = 2
  };
} // namespace

extern "C"
{
  // Stub implementations for jemalloc compatibility.
  // These are called by FreeBSD's libthr (pthreads) to notify malloc of
  // various events.  They are currently unused, though we may wish to reset
  // statistics on fork if built with statistics.

  SNMALLOC_EXPORT SNMALLOC_USED_FUNCTION inline void _malloc_prefork(void) {}

  SNMALLOC_EXPORT SNMALLOC_USED_FUNCTION inline void _malloc_postfork(void) {}

  SNMALLOC_EXPORT SNMALLOC_USED_FUNCTION inline void _malloc_first_thread(void)
  {}

  /**
   * Jemalloc API provides a way of avoiding name lookup when calling
   * `mallctl`.  For now, always return an error.
   */
  int SNMALLOC_NAME_MANGLE(mallctlnametomib)(const char*, size_t*, size_t*)
  {
    return ENOENT;
  }

  /**
   * Jemalloc API provides a generic entry point for various functions.  For
   * now, this is always implemented to return an error.
   */
  int SNMALLOC_NAME_MANGLE(mallctlbymib)(
    const size_t*, size_t, void*, size_t*, void*, size_t)
  {
    return ENOENT;
  }

  /**
   * Jemalloc API provides a generic entry point for various functions.  For
   * now, this is always implemented to return an error.
   */
  SNMALLOC_EXPORT int
  SNMALLOC_NAME_MANGLE(mallctl)(const char*, void*, size_t*, void*, size_t)
  {
    return ENOENT;
  }

#ifdef SNMALLOC_JEMALLOC3_EXPERIMENTAL
  /**
   * Jemalloc 3 experimental API.  Allocates  at least `size` bytes and returns
   * the result in `*ptr`, if `rsize` is not null then writes the allocated size
   * into `*rsize`.  `flags` controls whether the memory is zeroed and what
   * alignment is requested.
   */
  int SNMALLOC_NAME_MANGLE(allocm)(
    void** ptr, size_t* rsize, size_t size, int flags)
  {
    auto f = JEMallocFlags(flags);
    size = f.aligned_size(size);
    if (rsize != nullptr)
    {
      *rsize = round_size(size);
    }
    if (f.should_zero())
    {
      *ptr = alloc<Zero>(size);
    }
    else
    {
      *ptr = alloc(size);
    }
    return (*ptr != nullptr) ? allocm_success : allocm_err_oom;
  }

  /**
   * Jemalloc 3 experimental API.  Reallocates the allocation in `*ptr` to be at
   * least `size` bytes and returns the result in `*ptr`, if `rsize` is not null
   * then writes the allocated size into `*rsize`.  `flags` controls whether the
   * memory is zeroed and what alignment is requested and whether reallocation
   * is permitted.  If reallocating, the size will be at least `size` + `extra`
   * bytes.
   */
  int SNMALLOC_NAME_MANGLE(rallocm)(
    void** ptr, size_t* rsize, size_t size, size_t extra, int flags)
  {
    auto f = JEMallocFlags(flags);
    auto asize = f.aligned_size(size);

    size_t sz = alloc_size(*ptr);
    // Keep the current allocation if the given size is in the same sizeclass.
    if (sz == round_size(asize))
    {
      if (rsize != nullptr)
      {
        *rsize = sz;
      }
      return allocm_success;
    }

    if (f.may_not_move())
    {
      return allocm_err_not_moved;
    }

    if (SIZE_MAX - size > extra)
    {
      asize = f.aligned_size(size + extra);
    }

    void* p = f.should_zero() ? alloc<Zero>(asize) : alloc(asize);
    if (SNMALLOC_LIKELY(p != nullptr))
    {
      sz = bits::min(asize, sz);
      // Guard memcpy as GCC is assuming not nullptr for ptr after the memcpy
      // otherwise.
      if (sz != 0)
      {
        memcpy(p, *ptr, sz);
      }
      dealloc(*ptr);
      *ptr = p;
      if (rsize != nullptr)
      {
        *rsize = asize;
      }
      return allocm_success;
    }
    return allocm_err_oom;
  }

  /**
   * Jemalloc 3 experimental API.  Sets `*rsize` to the size of the allocation
   * at `*ptr`.  The third argument contains some flags relating to arenas that
   * we ignore.
   */
  int SNMALLOC_NAME_MANGLE(sallocm)(const void* ptr, size_t* rsize, int)
  {
    *rsize = alloc_size(ptr);
    return allocm_success;
  }

  /**
   * Jemalloc 3 experimental API.  Deallocates the allocation
   * at `*ptr`.  The second argument contains some flags relating to arenas that
   * we ignore.
   */
  int SNMALLOC_NAME_MANGLE(dallocm)(void* ptr, int)
  {
    dealloc(ptr);
    return allocm_success;
  }

  /**
   * Jemalloc 3 experimental API.  Returns in `*rsize` the size of the
   * allocation that would be returned if `size` and `flags` are passed to
   * `allocm`.
   */
  int SNMALLOC_NAME_MANGLE(nallocm)(size_t* rsize, size_t size, int flags)
  {
    *rsize = round_size(JEMallocFlags(flags).aligned_size(size));
    return allocm_success;
  }
#endif

#ifdef SNMALLOC_JEMALLOC_NONSTANDARD
  /**
   * Jemalloc function that provides control over alignment and zeroing
   * behaviour via the `flags` argument.  This argument also includes control
   * over the thread cache and arena to use.  These don't translate directly to
   * snmalloc and so are ignored.
   */
  SNMALLOC_EXPORT void* SNMALLOC_NAME_MANGLE(mallocx)(size_t size, int flags)
  {
    auto f = JEMallocFlags(flags);
    size = f.aligned_size(size);
    if (f.should_zero())
    {
      return alloc<Zero>(size);
    }
    return alloc(size);
  }

  /**
   * Jemalloc non-standard function that is similar to `realloc`.  This can
   * request zeroed memory for any newly allocated memory, though only if the
   * object grows (which, for snmalloc, means if it's copied).  The flags
   * controlling the thread cache and arena are ignored.
   */
  SNMALLOC_EXPORT void*
  SNMALLOC_NAME_MANGLE(rallocx)(void* ptr, size_t size, int flags)
  {
    auto f = JEMallocFlags(flags);
    size = f.aligned_size(size);

    size_t sz = round_size(alloc_size(ptr));
    // Keep the current allocation if the given size is in the same sizeclass.
    if (sz == size)
    {
      return ptr;
    }

    if (size == (size_t)-1)
    {
      return nullptr;
    }

    // We have a choice here of either asking for zeroed memory, or trying to
    // zero the remainder.  The former is *probably* faster for large
    // allocations, because we get zeroed memory from the PAL and don't zero it
    // twice.  This is not profiled and so should be considered for refactoring
    // if anyone cares about the performance of these APIs.
    void* p = f.should_zero() ? alloc<Zero>(size) : alloc(size);
    if (SNMALLOC_LIKELY(p != nullptr))
    {
      sz = bits::min(size, sz);
      // Guard memcpy as GCC is assuming not nullptr for ptr after the memcpy
      // otherwise.
      if (sz != 0)
        memcpy(p, ptr, sz);
      dealloc(ptr);
    }
    return p;
  }

  /**
   * Jemalloc non-standard API that performs a `realloc` only if it can do so
   * without copying and returns the size of the underlying object.  With
   * snmalloc, this simply returns the size of the sizeclass backing the
   * object.
   */
  size_t SNMALLOC_NAME_MANGLE(xallocx)(void* ptr, size_t, size_t, int)
  {
    return alloc_size(ptr);
  }

  /**
   * Jemalloc non-standard API that queries the underlying size of the
   * allocation.
   */
  size_t SNMALLOC_NAME_MANGLE(sallocx)(const void* ptr, int)
  {
    return alloc_size(ptr);
  }

  /**
   * Jemalloc non-standard API that frees `ptr`.  The second argument allows
   * specifying a thread cache or arena but this is currently unused in
   * snmalloc.
   */
  void SNMALLOC_NAME_MANGLE(dallocx)(void* ptr, int)
  {
    dealloc(ptr);
  }

  /**
   * Jemalloc non-standard API that frees `ptr`.  The second argument specifies
   * a size, which is intended to speed up the operation.  This could improve
   * performance for snmalloc, if we could guarantee that this is allocated by
   * the current thread but is otherwise not helpful.  The third argument allows
   * specifying a thread cache or arena but this is currently unused in
   * snmalloc.
   */
  void SNMALLOC_NAME_MANGLE(sdallocx)(void* ptr, size_t, int)
  {
    dealloc(ptr);
  }

  /**
   * Jemalloc non-standard API that returns the size of memory that would be
   * allocated if the same arguments were passed to `mallocx`.
   */
  size_t SNMALLOC_NAME_MANGLE(nallocx)(size_t size, int flags)
  {
    return round_size(JEMallocFlags(flags).aligned_size(size));
  }
#endif

#if !defined(__PIC__) && defined(SNMALLOC_BOOTSTRAP_ALLOCATOR)
  // The following functions are required to work before TLS is set up, in
  // statically-linked programs.  These temporarily grab an allocator from the
  // pool and return it.

  void* __je_bootstrap_malloc(size_t size)
  {
    return get_scoped_allocator()->alloc(size);
  }

  void* __je_bootstrap_calloc(size_t nmemb, size_t size)
  {
    bool overflow = false;
    size_t sz = bits::umul(size, nmemb, overflow);
    if (overflow)
    {
      errno = ENOMEM;
      return nullptr;
    }
    // Include size 0 in the first sizeclass.
    sz = ((sz - 1) >> (bits::BITS - 1)) + sz;
    return get_scoped_allocator()->alloc<Zero>(sz);
  }

  void __je_bootstrap_free(void* ptr)
  {
    get_scoped_allocator()->dealloc(ptr);
  }
#endif
}
