#pragma once
#include "../ds_core/ds_core.h"

#include <stdint.h>

namespace snmalloc
{
  /**
   * The type used for an address.  On CHERI, this is not a provenance-carrying
   * value and so cannot be converted back to a pointer.
   */
  using address_t = Aal::address_t;

  /**
   * Perform arithmetic on a uintptr_t.
   */
  SNMALLOC_FAST_PATH_INLINE uintptr_t
  pointer_offset(uintptr_t base, size_t diff)
  {
    return base + diff;
  }

  /**
   * Perform pointer arithmetic and return the adjusted pointer.
   */
  template<typename U = void, typename T>
  SNMALLOC_FAST_PATH_INLINE U* pointer_offset(T* base, size_t diff)
  {
    SNMALLOC_ASSERT(base != nullptr); /* Avoid UB */
    return unsafe_from_uintptr<U>(
      unsafe_to_uintptr<T>(base) + static_cast<uintptr_t>(diff));
  }

  template<SNMALLOC_CONCEPT(capptr::IsBound) bounds, typename T>
  SNMALLOC_FAST_PATH_INLINE CapPtr<void, bounds>
  pointer_offset(CapPtr<T, bounds> base, size_t diff)
  {
    return CapPtr<void, bounds>::unsafe_from(
      pointer_offset(base.unsafe_ptr(), diff));
  }

  /**
   * Perform pointer arithmetic and return the adjusted pointer.
   */
  template<typename U = void, typename T>
  SNMALLOC_FAST_PATH_INLINE U* pointer_offset_signed(T* base, ptrdiff_t diff)
  {
    SNMALLOC_ASSERT(base != nullptr); /* Avoid UB */
    return reinterpret_cast<U*>(reinterpret_cast<char*>(base) + diff);
  }

  template<SNMALLOC_CONCEPT(capptr::IsBound) bounds, typename T>
  SNMALLOC_FAST_PATH_INLINE CapPtr<void, bounds>
  pointer_offset_signed(CapPtr<T, bounds> base, ptrdiff_t diff)
  {
    return CapPtr<void, bounds>::unsafe_from(
      pointer_offset_signed(base.unsafe_ptr(), diff));
  }

  /**
   * Cast from a pointer type to an address.
   */
  template<typename T>
  SNMALLOC_FAST_PATH_INLINE address_t address_cast(T* ptr)
  {
    return reinterpret_cast<address_t>(ptr);
  }

  /*
   * Provide address_cast methods for the provenance-hinting pointer wrapper
   * types as well.  While we'd prefer that these be methods on the wrapper
   * type, they have to be defined later, because the AAL both define address_t,
   * as per above, and uses the wrapper types in its own definition, e.g., of
   * capptr_bound.
   */
  template<typename T, SNMALLOC_CONCEPT(capptr::IsBound) bounds>
  SNMALLOC_FAST_PATH_INLINE address_t address_cast(CapPtr<T, bounds> a)
  {
    return address_cast(a.unsafe_ptr());
  }

  SNMALLOC_FAST_PATH_INLINE address_t address_cast(uintptr_t a)
  {
    return static_cast<address_t>(a);
  }

  /**
   * Test if a pointer is aligned to a given size, which must be a power of
   * two.
   */
  template<size_t alignment>
  SNMALLOC_FAST_PATH_INLINE bool is_aligned_block(address_t p, size_t size)
  {
    static_assert(bits::is_pow2(alignment));

    return ((p | size) & (alignment - 1)) == 0;
  }

  template<size_t alignment>
  SNMALLOC_FAST_PATH_INLINE bool is_aligned_block(void* p, size_t size)
  {
    return is_aligned_block<alignment>(address_cast(p), size);
  }

  /**
   * Align a uintptr_t down to a statically specified granularity, which must be
   * a power of two.
   */
  template<size_t alignment>
  SNMALLOC_FAST_PATH_INLINE uintptr_t pointer_align_down(uintptr_t p)
  {
    static_assert(alignment > 0);
    static_assert(bits::is_pow2(alignment));
    if constexpr (alignment == 1)
      return p;
    else
    {
#if __has_builtin(__builtin_align_down)
      return __builtin_align_down(p, alignment);
#else
      return bits::align_down(p, alignment);
#endif
    }
  }

  /**
   * Align a pointer down to a statically specified granularity, which must be a
   * power of two.
   */
  template<size_t alignment, typename T = void>
  SNMALLOC_FAST_PATH_INLINE T* pointer_align_down(void* p)
  {
    return unsafe_from_uintptr<T>(
      pointer_align_down<alignment>(unsafe_to_uintptr<void>(p)));
  }

  template<
    size_t alignment,
    typename T,
    SNMALLOC_CONCEPT(capptr::IsBound) bounds>
  SNMALLOC_FAST_PATH_INLINE CapPtr<T, bounds>
  pointer_align_down(CapPtr<void, bounds> p)
  {
    return CapPtr<T, bounds>::unsafe_from(
      pointer_align_down<alignment, T>(p.unsafe_ptr()));
  }

  template<size_t alignment>
  SNMALLOC_FAST_PATH_INLINE address_t address_align_down(address_t p)
  {
    return bits::align_down(p, alignment);
  }

  /**
   * Align a pointer up to a statically specified granularity, which must be a
   * power of two.
   */
  template<size_t alignment, typename T = void>
  SNMALLOC_FAST_PATH_INLINE T* pointer_align_up(void* p)
  {
    static_assert(alignment > 0);
    static_assert(bits::is_pow2(alignment));
    if constexpr (alignment == 1)
      return static_cast<T*>(p);
    else
    {
#if __has_builtin(__builtin_align_up)
      return static_cast<T*>(__builtin_align_up(p, alignment));
#else
      return unsafe_from_uintptr<T>(
        bits::align_up(unsafe_to_uintptr<void>(p), alignment));
#endif
    }
  }

  template<
    size_t alignment,
    typename T = void,
    SNMALLOC_CONCEPT(capptr::IsBound) bounds>
  SNMALLOC_FAST_PATH_INLINE CapPtr<T, bounds>
  pointer_align_up(CapPtr<void, bounds> p)
  {
    return CapPtr<T, bounds>::unsafe_from(
      pointer_align_up<alignment, T>(p.unsafe_ptr()));
  }

  template<size_t alignment>
  SNMALLOC_FAST_PATH_INLINE address_t address_align_up(address_t p)
  {
    return bits::align_up(p, alignment);
  }

  /**
   * Align a pointer down to a dynamically specified granularity, which must be
   * a power of two.
   */
  template<typename T = void>
  SNMALLOC_FAST_PATH_INLINE T* pointer_align_down(void* p, size_t alignment)
  {
    SNMALLOC_ASSERT(alignment > 0);
    SNMALLOC_ASSERT(bits::is_pow2(alignment));
#if __has_builtin(__builtin_align_down)
    return static_cast<T*>(__builtin_align_down(p, alignment));
#else
    return unsafe_from_uintptr<T>(
      bits::align_down(unsafe_to_uintptr<void>(p), alignment));
#endif
  }

  template<typename T = void, SNMALLOC_CONCEPT(capptr::IsBound) bounds>
  SNMALLOC_FAST_PATH_INLINE CapPtr<T, bounds>
  pointer_align_down(CapPtr<void, bounds> p, size_t alignment)
  {
    return CapPtr<T, bounds>::unsafe_from(
      pointer_align_down<T>(p.unsafe_ptr(), alignment));
  }

  /**
   * Align a pointer up to a dynamically specified granularity, which must
   * be a power of two.
   */
  template<typename T = void>
  SNMALLOC_FAST_PATH_INLINE T* pointer_align_up(void* p, size_t alignment)
  {
    SNMALLOC_ASSERT(alignment > 0);
    SNMALLOC_ASSERT(bits::is_pow2(alignment));
#if __has_builtin(__builtin_align_up)
    return static_cast<T*>(__builtin_align_up(p, alignment));
#else
    return unsafe_from_uintptr<T>(
      bits::align_up(unsafe_to_uintptr<void>(p), alignment));
#endif
  }

  template<typename T = void, SNMALLOC_CONCEPT(capptr::IsBound) bounds>
  SNMALLOC_FAST_PATH_INLINE CapPtr<T, bounds>
  pointer_align_up(CapPtr<void, bounds> p, size_t alignment)
  {
    return CapPtr<T, bounds>::unsafe_from(
      pointer_align_up<T>(p.unsafe_ptr(), alignment));
  }

  /**
   * Compute the difference in pointers in units of char.  base is
   * expected to point to the base of some (sub)allocation into which cursor
   * points; would-be negative answers trip an assertion in debug builds.
   */
  SNMALLOC_FAST_PATH_INLINE size_t
  pointer_diff(const void* base, const void* cursor)
  {
    SNMALLOC_ASSERT(cursor >= base);
    return static_cast<size_t>(
      static_cast<const char*>(cursor) - static_cast<const char*>(base));
  }

  template<
    typename T = void,
    typename U = void,
    SNMALLOC_CONCEPT(capptr::IsBound) Tbounds,
    SNMALLOC_CONCEPT(capptr::IsBound) Ubounds>
  SNMALLOC_FAST_PATH_INLINE size_t
  pointer_diff(CapPtr<T, Tbounds> base, CapPtr<U, Ubounds> cursor)
  {
    return pointer_diff(base.unsafe_ptr(), cursor.unsafe_ptr());
  }

  /**
   * Compute the difference in pointers in units of char. This can be used
   * across allocations.
   */
  SNMALLOC_FAST_PATH_INLINE ptrdiff_t
  pointer_diff_signed(void* base, void* cursor)
  {
    return static_cast<ptrdiff_t>(
      static_cast<char*>(cursor) - static_cast<char*>(base));
  }

  template<
    typename T = void,
    typename U = void,
    SNMALLOC_CONCEPT(capptr::IsBound) Tbounds,
    SNMALLOC_CONCEPT(capptr::IsBound) Ubounds>
  SNMALLOC_FAST_PATH_INLINE ptrdiff_t
  pointer_diff_signed(CapPtr<T, Tbounds> base, CapPtr<U, Ubounds> cursor)
  {
    return pointer_diff_signed(base.unsafe_ptr(), cursor.unsafe_ptr());
  }

  /**
   * Compute the degree to which an address is misaligned relative to some
   * putative alignment.
   */
  template<size_t alignment>
  SNMALLOC_FAST_PATH_INLINE size_t address_misalignment(address_t a)
  {
    return static_cast<size_t>(a - pointer_align_down<alignment>(a));
  }

  /**
   * Convert an address_t to a pointer.  The returned pointer should never be
   * followed. On CHERI following this pointer will result in a capability
   * violation.
   */
  template<typename T>
  SNMALLOC_FAST_PATH_INLINE T* useless_ptr_from_addr(address_t p)
  {
    return reinterpret_cast<T*>(static_cast<uintptr_t>(p));
  }
} // namespace snmalloc
