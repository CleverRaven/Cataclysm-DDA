#pragma once

#include "globalalloc.h"

#include <errno.h>
#include <string.h>

namespace snmalloc::libc
{
  SNMALLOC_SLOW_PATH inline void* set_error(int err = ENOMEM)
  {
    errno = err;
    return nullptr;
  }

  SNMALLOC_SLOW_PATH inline int set_error_and_return(int err = ENOMEM)
  {
    errno = err;
    return err;
  }

  inline void* __malloc_end_pointer(void* ptr)
  {
    return snmalloc::external_pointer<OnePastEnd>(ptr);
  }

  SNMALLOC_FAST_PATH_INLINE void* malloc(size_t size)
  {
    return snmalloc::alloc(size);
  }

  SNMALLOC_FAST_PATH_INLINE void free(void* ptr)
  {
    dealloc(ptr);
  }

  SNMALLOC_FAST_PATH_INLINE void free_sized(void* ptr, size_t size)
  {
    dealloc(ptr, size);
  }

  SNMALLOC_FAST_PATH_INLINE void* calloc(size_t nmemb, size_t size)
  {
    bool overflow = false;
    size_t sz = bits::umul(size, nmemb, overflow);
    if (SNMALLOC_UNLIKELY(overflow))
    {
      return set_error();
    }
    return alloc<Zero>(sz);
  }

  SNMALLOC_FAST_PATH_INLINE void* realloc(void* ptr, size_t size)
  {
    // Glibc treats
    //   realloc(p, 0) as free(p)
    //   realloc(nullptr, s) as malloc(s)
    // and for the overlap
    //   realloc(nullptr, 0) as malloc(1)
    // Without the first two we don't pass the glibc tests.
    // The last one is required by various gnu utilities such as grep, sed.
    if (SNMALLOC_UNLIKELY(!is_small_sizeclass(size)))
    {
      if (SNMALLOC_UNLIKELY(size == 0))
      {
        if (SNMALLOC_UNLIKELY(ptr == nullptr))
          return malloc(1);

        dealloc(ptr);
        return nullptr;
      }
    }

    size_t sz = alloc_size(ptr);
    // Keep the current allocation if the given size is in the same sizeclass.
    if (sz == round_size(size))
    {
      return ptr;
    }

    void* p = alloc(size);
    if (SNMALLOC_LIKELY(p != nullptr))
    {
      sz = bits::min(size, sz);
      // Guard memcpy as GCC is assuming not nullptr for ptr after the memcpy
      // otherwise.
      if (SNMALLOC_UNLIKELY(sz != 0))
      {
        SNMALLOC_ASSUME(ptr != nullptr);
        ::memcpy(p, ptr, sz);
      }
      dealloc(ptr);
    }
    else
    {
      // Error should be set by alloc on this path already.
      SNMALLOC_ASSERT(errno == ENOMEM);
    }
    return p;
  }

  inline size_t malloc_usable_size(const void* ptr)
  {
    return alloc_size(ptr);
  }

  inline void* reallocarray(void* ptr, size_t nmemb, size_t size)
  {
    bool overflow = false;
    size_t sz = bits::umul(size, nmemb, overflow);
    if (SNMALLOC_UNLIKELY(overflow))
    {
      return set_error();
    }
    return realloc(ptr, sz);
  }

  inline int reallocarr(void* ptr_, size_t nmemb, size_t size)
  {
    int err = errno;
    bool overflow = false;
    size_t sz = bits::umul(size, nmemb, overflow);
    if (SNMALLOC_UNLIKELY(sz == 0))
    {
      errno = err;
      return 0;
    }
    if (SNMALLOC_UNLIKELY(overflow))
    {
      return set_error_and_return(EOVERFLOW);
    }

    void** ptr = reinterpret_cast<void**>(ptr_);
    void* p = alloc(sz);
    if (SNMALLOC_UNLIKELY(p == nullptr))
    {
      return set_error_and_return(ENOMEM);
    }

    sz = bits::min(sz, alloc_size(*ptr));

    SNMALLOC_ASSUME(*ptr != nullptr || sz == 0);
    // Guard memcpy as GCC is assuming not nullptr for ptr after the memcpy
    // otherwise.
    if (SNMALLOC_UNLIKELY(sz != 0))
      ::memcpy(p, *ptr, sz);
    errno = err;
    dealloc(*ptr);
    *ptr = p;
    return 0;
  }

  inline void* memalign(size_t alignment, size_t size)
  {
    if (SNMALLOC_UNLIKELY(alignment == 0 || !bits::is_pow2(alignment)))
    {
      return set_error(EINVAL);
    }

    return alloc_aligned(alignment, size);
  }

  inline void* aligned_alloc(size_t alignment, size_t size)
  {
    return memalign(alignment, size);
  }

  inline int posix_memalign(void** memptr, size_t alignment, size_t size)
  {
    if (SNMALLOC_UNLIKELY(
          (alignment < sizeof(uintptr_t) || !bits::is_pow2(alignment))))
    {
      return EINVAL;
    }

    void* p = memalign(alignment, size);
    if (SNMALLOC_UNLIKELY(p == nullptr))
    {
      if (size != 0)
        return ENOMEM;
    }
    *memptr = p;
    return 0;
  }
} // namespace snmalloc::libc
