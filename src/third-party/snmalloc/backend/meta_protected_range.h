#pragma once

#include "../backend/backend.h"
#include "base_constants.h"

namespace snmalloc
{
  /**
   * Range that carefully ensures meta-data and object data cannot be in
   * the same memory range. Once memory has is used for either meta-data
   * or object data it can never be recycled to the other.
   *
   * This configuration also includes guard pages and randomisation.
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
    typename Base,
    size_t MinSizeBits = MinBaseSizeBits<PAL>()>
  struct MetaProtectedRangeLocalState : BaseLocalStateConstants
  {
  private:
    // Global range of memory
    using GlobalR = Pipe<
      Base,
      LargeBuddyRange<
        GlobalCacheSizeBits,
        bits::BITS - 1,
        Pagemap,
        MinSizeBits>,
      LogRange<2>,
      GlobalRange>;

    static constexpr size_t page_size_bits =
      bits::next_pow2_bits_const(PAL::page_size);

    static constexpr size_t max_page_chunk_size_bits =
      bits::max(page_size_bits, MIN_CHUNK_BITS);

    // Central source of object-range, does not pass back to GlobalR as
    // that would allow flows from Objects to Meta-data, and thus UAF
    // would be able to corrupt meta-data.
    using CentralObjectRange = Pipe<
      GlobalR,
      LargeBuddyRange<GlobalCacheSizeBits, bits::BITS - 1, Pagemap>,
      LogRange<3>,
      GlobalRange,
      CommitRange<PAL>,
      StatsRange>;

    // Controls the padding around the meta-data range.
    // The larger the padding range the more randomisation that
    // can be used.
    static constexpr size_t SubRangeRatioBits = 6;

    // Centralised source of meta-range
    using CentralMetaRange = Pipe<
      GlobalR,
      SubRange<PAL, SubRangeRatioBits>, // Use SubRange to introduce guard
                                        // pages.
      LargeBuddyRange<
        GlobalCacheSizeBits,
        bits::BITS - 1,
        Pagemap,
        page_size_bits>,
      CommitRange<PAL>,
      // In case of huge pages, we don't want to give each thread its own huge
      // page, so commit in the global range.
      stl::conditional_t<
        (max_page_chunk_size_bits > MIN_CHUNK_BITS),
        LargeBuddyRange<
          max_page_chunk_size_bits,
          max_page_chunk_size_bits,
          Pagemap,
          page_size_bits>,
        NopRange>,
      LogRange<4>,
      GlobalRange,
      StatsRange>;

    // Local caching of object range
    using ObjectRange = Pipe<
      CentralObjectRange,
      LargeBuddyRange<
        LocalCacheSizeBits,
        LocalCacheSizeBits,
        Pagemap,
        page_size_bits>,
      LogRange<5>>;

    // Local caching of meta-data range
    using MetaRange = Pipe<
      CentralMetaRange,
      LargeBuddyRange<
        LocalCacheSizeBits - SubRangeRatioBits,
        bits::BITS - 1,
        Pagemap>,
      SmallBuddyRange>;

    ObjectRange object_range;

    MetaRange meta_range;

  public:
    using Stats = StatsCombiner<CentralObjectRange, CentralMetaRange>;

    ObjectRange* get_object_range()
    {
      return &object_range;
    }

    MetaRange& get_meta_range()
    {
      return meta_range;
    }

    // Create global range that can service small meta-data requests.
    // Don't want to add the SmallBuddyRange to the CentralMetaRange as that
    // would require committing memory inside the main global lock.
    using GlobalMetaRange =
      Pipe<CentralMetaRange, SmallBuddyRange, GlobalRange>;
  };
} // namespace snmalloc
