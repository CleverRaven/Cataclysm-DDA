#define SNMALLOC_NAME_MANGLE(a) sn_##a

// The libc API provided by malloc.cc will always be mangled per above.
#ifdef SNMALLOC_RUST_LIBC_API
#  include "malloc.cc"
#else
#  include "snmalloc/snmalloc.h"
#endif

#include <string.h>

#ifndef SNMALLOC_EXPORT
#  define SNMALLOC_EXPORT
#endif

using namespace snmalloc;

extern "C" SNMALLOC_EXPORT void*
SNMALLOC_NAME_MANGLE(rust_alloc)(size_t alignment, size_t size)
{
  return alloc(aligned_size(alignment, size));
}

extern "C" SNMALLOC_EXPORT void*
SNMALLOC_NAME_MANGLE(rust_alloc_zeroed)(size_t alignment, size_t size)
{
  return alloc<Zero>(aligned_size(alignment, size));
}

extern "C" SNMALLOC_EXPORT void
SNMALLOC_NAME_MANGLE(rust_dealloc)(void* ptr, size_t alignment, size_t size)
{
  dealloc(ptr, aligned_size(alignment, size));
}

extern "C" SNMALLOC_EXPORT void* SNMALLOC_NAME_MANGLE(rust_realloc)(
  void* ptr, size_t alignment, size_t old_size, size_t new_size)
{
  size_t aligned_old_size = aligned_size(alignment, old_size),
         aligned_new_size = aligned_size(alignment, new_size);
  if (
    size_to_sizeclass_full(aligned_old_size).raw() ==
    size_to_sizeclass_full(aligned_new_size).raw())
    return ptr;
  void* p = alloc(aligned_new_size);
  if (p)
  {
    memcpy(p, ptr, old_size < new_size ? old_size : new_size);
    dealloc(ptr, aligned_old_size);
  }
  return p;
}

extern "C" SNMALLOC_EXPORT void SNMALLOC_NAME_MANGLE(rust_statistics)(
  size_t* current_memory_usage, size_t* peak_memory_usage)
{
  *current_memory_usage = Alloc::Config::Backend::get_current_usage();
  *peak_memory_usage = Alloc::Config::Backend::get_peak_usage();
}

extern "C" SNMALLOC_EXPORT size_t
SNMALLOC_NAME_MANGLE(rust_usable_size)(const void* ptr)
{
  return alloc_size(ptr);
}
