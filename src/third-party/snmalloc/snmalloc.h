#pragma once

// Core implementation of snmalloc independent of the configuration mode
#include "snmalloc_core.h"

// Provides the global configuration for the snmalloc implementation.
#include "backend/globalconfig.h"

#ifdef SNMALLOC_ENABLE_GWP_ASAN_INTEGRATION
#  include "snmalloc/mem/secondary/gwp_asan.h"
#endif

namespace snmalloc
{
// If you define SNMALLOC_PROVIDE_OWN_CONFIG then you must provide your own
// definition of `snmalloc::Alloc` before including any files that include
// `snmalloc.h` or consume the global allocation APIs.
#ifndef SNMALLOC_PROVIDE_OWN_CONFIG
#  ifdef SNMALLOC_ENABLE_GWP_ASAN_INTEGRATION
  using Config = snmalloc::StandardConfigClientMeta<
    NoClientMetaDataProvider,
    GwpAsanSecondaryAllocator>;
#  else
  using Config = snmalloc::StandardConfigClientMeta<NoClientMetaDataProvider>;
#  endif
#endif
  /**
   * Create allocator type for this configuration.
   */
  using Alloc = snmalloc::Allocator<Config>;
} // namespace snmalloc

// User facing API surface, needs to know what `Alloc` is.
#include "snmalloc_front.h"
