#pragma once

#if __riscv_xlen == 64
#  define SNMALLOC_VA_BITS_64
#elif __riscv_xlen == 32
#  define SNMALLOC_VA_BITS_32
#endif

namespace snmalloc
{
  /**
   * RISC-V architecture layer, phrased as generically as possible.  Specific
   * implementations may need to adjust some of these.
   */
  class AAL_RISCV
  {
  public:
    static constexpr uint64_t aal_features = IntegerPointers;

    static constexpr size_t smallest_page_size = 0x1000;

    static constexpr AalName aal_name = RISCV;

    static void inline pause()
    {
      /*
       * The "Zihintpause" extension claims to be the right thing to do here,
       * and it is expected to be used in analogous places, e.g., Linux's
       * cpu_relax(), but...
       *
       *  its specification is somewhat unusual, in that it talks about the rate
       *  at which a HART's instructions retire rather than the rate at which
       *  they are dispatched (Intel's PAUSE instruction explicitly promises
       *  that it "de-pipelines" the spin-wait loop, for example) or anything
       *  about memory semantics (Intel's PAUSE docs talk about a possible
       *  memory order violation and pipeline flush upon loop exit).
       *
       *  we don't yet have examples of what implementations have done.
       *
       *  it's not yet understood by C frontends or assembler, meaning we'd have
       *  to spell it out by hand, as
       *      __asm__ volatile(".byte 0xF; .byte 0x0; .byte 0x0; .byte 0x1");
       *
       * All told, we just leave this function empty for the moment.  The good
       * news is that, if and when we do add a PAUSE, the instruction is encoded
       * by stealing some dead space of the FENCE instruction and so should be
       * available everywhere even if it doesn't do anything on a particular
       * microarchitecture.
       */
    }
  };

  using AAL_Arch = AAL_RISCV;
}
