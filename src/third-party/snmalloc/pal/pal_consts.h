#pragma once

#include "../ds_core/ds_core.h"
#include "snmalloc/stl/atomic.h"

namespace snmalloc
{
  /**
   * Flags in a bitfield of optional features that a PAL may support.  These
   * should be set in the PAL's `pal_features` static constexpr field.
   */
  enum PalFeatures : uint64_t
  {
    /**
     * This PAL supports low memory notifications.  It must implement a
     * `register_for_low_memory_callback` method that allows callbacks to be
     * registered when the platform detects low-memory and an
     * `expensive_low_memory_check()` method that returns a `bool` indicating
     * whether low memory conditions are still in effect.
     */
    LowMemoryNotification = (1 << 0),

    /**
     * This PAL natively supports allocation with a guaranteed alignment.  If
     * this is not supported, then we will over-allocate and round the
     * allocation.
     *
     * A PAL that does supports this must expose a `request()` method that takes
     * a size and alignment.  A PAL that does *not* support it must expose a
     * `request()` method that takes only a size.
     */
    AlignedAllocation = (1 << 1),

    /**
     * This PAL natively supports lazy commit of pages. This means have large
     * allocations and not touching them does not increase memory usage. This is
     * exposed in the Pal.
     */
    LazyCommit = (1 << 2),

    /**
     * This Pal does not support allocation.  All memory used with this Pal
     * should be pre-allocated.
     */
    NoAllocation = (1 << 3),

    /**
     * This Pal provides a source of Entropy
     */
    Entropy = (1 << 4),

    /**
     * This Pal provides a millisecond time source
     */
    Time = (1 << 5),

    /**
     * This Pal provides selective core dumps, so
     * modify which parts get dumped.
     */
    CoreDump = (1 << 6),

    /**
     * This Pal provides a way for parking threads at a specific address.
     */
    WaitOnAddress = (1 << 7),
  };

  /**
   * Flag indicating whether requested memory should be zeroed.
   */
  enum ZeroMem
  {
    /**
     * Memory should not be zeroed, contents are undefined.
     */
    NoZero,

    /**
     * Memory must be zeroed.  This can be lazily allocated via a copy-on-write
     * mechanism as long as any load from the memory returns zero.
     */
    YesZero
  };

  /**
   * Default Tag ID for the Apple class
   */
  static const int PALAnonDefaultID = 241;

  /**
   * Query whether the PAL supports a specific feature.
   */
  template<PalFeatures F, typename PAL>
  static constexpr bool pal_supports = (PAL::pal_features & F) == F;
} // namespace snmalloc
