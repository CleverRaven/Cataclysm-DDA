#pragma once
#include <stdint.h>

namespace snmalloc
{
  /**
   * Flags in a bitfield of attributes of this architecture, much like
   * PalFeatures.
   */
  enum AalFeatures : uint64_t
  {
    /**
     * This architecture does not discriminate between integers and pointers,
     * and so may use bit operations on pointer values.
     */
    IntegerPointers = (1 << 0),
    /**
     * This architecture cannot access cpu cycles counters.
     */
    NoCpuCycleCounters = (1 << 1),
    /**
     * This architecture enforces strict pointer provenance; we bound the
     * pointers given out on malloc() and friends and must, therefore retain
     * internal high-privilege pointers for recycling memory on free().
     */
    StrictProvenance = (1 << 2),
  };

  enum AalName : int
  {
    ARM,
    PowerPC,
    X86,
    X86_SGX,
    Sparc,
    RISCV
  };
} // namespace snmalloc
