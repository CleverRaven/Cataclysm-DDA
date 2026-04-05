#include "override.h"

extern "C"
{
  /**
   * Snmalloc checked memcpy.
   */
  SNMALLOC_EXPORT void*
  SNMALLOC_NAME_MANGLE(memcpy)(void* dst, const void* src, size_t len)
  {
    return snmalloc::memcpy<true>(dst, src, len);
  }
}
