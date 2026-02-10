

#pragma once

#include "../backend/backend.h"

namespace snmalloc
{
  /**
   * Base range configuration contains common parts of other ranges.
   */
  struct BaseLocalStateConstants
  {
  protected:
    // Size of requests that the global cache should use
    static constexpr size_t GlobalCacheSizeBits = 24;

    // Size of requests that the local cache should use
    static constexpr size_t LocalCacheSizeBits = 21;
  };
} // namespace snmalloc