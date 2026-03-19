#pragma once

#include "snmalloc/ds_core/defines.h"

namespace snmalloc
{

  // This is used in both vendored and non-vendored mode.
  struct PlacementToken
  {};

  inline constexpr PlacementToken placement_token{};
} // namespace snmalloc

SNMALLOC_FAST_PATH_INLINE void*
operator new(size_t, void* ptr, snmalloc::PlacementToken) noexcept
{
  return ptr;
}

SNMALLOC_FAST_PATH_INLINE void*
operator new[](size_t, void* ptr, snmalloc::PlacementToken) noexcept
{
  return ptr;
}

// The following is not really needed, but windows expects that new/delete
// definitions are paired.
SNMALLOC_FAST_PATH_INLINE void
operator delete(void*, void*, snmalloc::PlacementToken) noexcept
{}

SNMALLOC_FAST_PATH_INLINE void
operator delete[](void*, void*, snmalloc::PlacementToken) noexcept
{}
