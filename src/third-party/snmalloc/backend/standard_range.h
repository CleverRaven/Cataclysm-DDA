

#pragma once

#include "../backend/backend.h"
#include "base_constants.h"

namespace snmalloc
{
  /**
   * Default configuration that does not provide any meta-data protection.
   *
   * PAL is the underlying PAL that is used to Commit memory ranges.
   *
   * Base is where memory is sourced from.
   *
   * MinSizeBits is the minimum request size that can be passed to Base.
   * On Windows this 16 as VirtualAlloc cannot reserve less than 64KiB.
   * Alternative configurations might make this 2MiB so that huge pages
   * can be used.
   */
  template<
    typename PAL,
    typename Pagemap,
    typename Base = EmptyRange<>,
    size_t MinSizeBits = MinBaseSizeBits<PAL>()>
  struct StandardLocalState : BaseLocalStateConstants
  {
    // Global range of memory, expose this so can be filled by init.
    using GlobalR = Pipe<
      Base,
      LargeBuddyRange<
        GlobalCacheSizeBits,
        bits::BITS - 1,
        Pagemap,
        MinSizeBits>,
      LogRange<2>,
      GlobalRange>;

    // Track stats of the committed memory
    using Stats = Pipe<GlobalR, CommitRange<PAL>, StatsRange>;

  private:
    static constexpr size_t page_size_bits =
      bits::next_pow2_bits_const(PAL::page_size);

  public:
    // Source for object allocations and metadata
    // Use buddy allocators to cache locally.
    using LargeObjectRange = Pipe<
      Stats,
      StaticConditionalRange<LargeBuddyRange<
        LocalCacheSizeBits,
        LocalCacheSizeBits,
        Pagemap,
        page_size_bits>>>;

  private:
    using ObjectRange = Pipe<LargeObjectRange, SmallBuddyRange>;

    ObjectRange object_range;

  public:
    // Expose a global range for the initial allocation of meta-data.
    using GlobalMetaRange = Pipe<ObjectRange, GlobalRange>;

    /**
     * Where we turn for allocations of user chunks.
     *
     * Reach over the SmallBuddyRange that's at the near end of the ObjectRange
     * pipe, rather than having that range adapter dynamically branch to its
     * parent.
     */
    LargeObjectRange* get_object_range()
    {
      return object_range.template ancestor<LargeObjectRange>();
    }

    /**
     * The backend has its own need for small objects without using the
     * frontend allocators; this range manages those.
     */
    ObjectRange& get_meta_range()
    {
      // Use the object range to service meta-data requests.
      return object_range;
    }

    static void set_small_heap()
    {
      // This disables the thread local caching of large objects.
      LargeObjectRange::disable_range();
    }
  };
} // namespace snmalloc
