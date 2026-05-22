#pragma once
#include "../backend_helpers/backend_helpers.h"

namespace snmalloc
{
  /**
   * This class implements the standard backend for handling allocations.
   * It is parameterised by its Pagemap management and
   * address space management (LocalState).
   */
  template<
    SNMALLOC_CONCEPT(IsPAL) PAL,
    typename PagemapEntry,
    typename Pagemap,
    typename Authmap,
    typename LocalState>
  class BackendAllocator
  {
    using GlobalMetaRange = typename LocalState::GlobalMetaRange;
    using Stats = typename LocalState::Stats;

  public:
    using Pal = PAL;
    using SlabMetadata = typename PagemapEntry::SlabMetadata;

  public:
    /**
     * Provide a block of meta-data with size and align.
     *
     * Backend allocator may use guard pages and separate area of
     * address space to protect this from corruption.
     *
     * The template argument is the type of the metadata being allocated.  This
     * allows the backend to allocate different types of metadata in different
     * places or with different policies.  The default implementation, here,
     * does not avail itself of this degree of freedom.
     */
    template<typename T>
    static capptr::Alloc<void>
    alloc_meta_data(LocalState* local_state, size_t size)
    {
      capptr::Arena<void> p;

      // Meta-data does not use our sizeclass machinery, so have Aal round up
      size = Aal::capptr_size_round(size);

      if (local_state != nullptr)
      {
        p = local_state->get_meta_range().alloc_range_with_leftover(size);
      }
      else
      {
        static_assert(
          GlobalMetaRange::ConcurrencySafe,
          "Global meta data range needs to be concurrency safe.");
        GlobalMetaRange global_state;
        p = global_state.alloc_range(bits::next_pow2(size));
      }

      if (p == nullptr)
      {
        return nullptr;
      }

      return capptr_to_user_address_control(
        Aal::capptr_bound<void, capptr::bounds::AllocFull>(p, size));
    }

    /**
     * Returns unused meta-data to the system.  This must have come from a call
     * to alloc_meta_data, but can be a sub-range of the original allocation.
     */
    static void dealloc_meta_data(
      LocalState& local_state, capptr::Alloc<void> p, size_t size)
    {
      auto arena = Authmap::amplify(p);
      local_state.get_meta_range().dealloc_range(arena, size);
    }

    /**
     * Returns a chunk of memory with alignment and size of `size`, and a
     * block containing metadata about the slab.
     *
     * It additionally set the meta-data for this chunk of memory to
     * be
     *   (remote, sizeclass, slab_metadata)
     * where slab_metadata, is the second element of the pair return.
     */
    static stl::Pair<capptr::Chunk<void>, SlabMetadata*> alloc_chunk(
      LocalState& local_state,
      size_t size,
      uintptr_t ras,
      sizeclass_t sizeclass)
    {
      SNMALLOC_ASSERT(bits::is_pow2(size));
      SNMALLOC_ASSERT(size >= MIN_CHUNK_SIZE);

      // Calculate the extra bytes required to store the client meta-data.
      size_t extra_bytes = SlabMetadata::get_extra_bytes(sizeclass);

      auto meta_size = bits::next_pow2(sizeof(SlabMetadata) + extra_bytes);

#ifdef SNMALLOC_TRACING
      message<1024>(
        "Allocating metadata of size: {} ({})", meta_size, extra_bytes);
#endif

      auto meta_cap = local_state.get_meta_range().alloc_range(meta_size);

      auto meta = meta_cap.template as_reinterpret<SlabMetadata>().unsafe_ptr();

      if (meta == nullptr)
      {
        return {nullptr, nullptr};
      }

      capptr::Arena<void> p = local_state.get_object_range()->alloc_range(size);

#ifdef SNMALLOC_TRACING
      message<1024>("Alloc chunk: {} ({})", p.unsafe_ptr(), size);
#endif
      if (p == nullptr)
      {
        local_state.get_meta_range().dealloc_range(meta_cap, meta_size);
#ifdef SNMALLOC_TRACING
        message<1024>("Out of memory");
#endif
        return {nullptr, nullptr};
      }

      typename Pagemap::Entry t(meta, ras);
      Pagemap::set_metaentry(address_cast(p), size, t);

      return {Aal::capptr_bound<void, capptr::bounds::Chunk>(p, size), meta};
    }

    /**
     * Deallocate a chunk of memory of size `size` and base `alloc`.
     * The `slab_metadata` is the meta-data block associated with this
     * chunk.  The backend can recalculate this, but as the callee will
     * already have it, we take it for possibly more optimal code.
     *
     * LocalState contains all the information about the various ranges
     * that are used by the backend to manage the address space.
     */
    static void dealloc_chunk(
      LocalState& local_state,
      SlabMetadata& slab_metadata,
      capptr::Alloc<void> alloc,
      size_t size,
      sizeclass_t sizeclass)
    {
      /*
       * The backend takes possession of these chunks now, by disassociating
       * any existing remote allocator and metadata structure.  If
       * interrogated, the sizeclass reported by the FrontendMetaEntry is 0,
       * which has size 0.
       */
      typename Pagemap::Entry t(nullptr, 0);
      t.claim_for_backend();
      SNMALLOC_ASSERT_MSG(
        Pagemap::get_metaentry(address_cast(alloc)).get_slab_metadata() ==
          &slab_metadata,
        "Slab metadata {} passed for address {} does not match the meta entry "
        "{} that is used for that address",
        &slab_metadata,
        address_cast(alloc),
        Pagemap::get_metaentry(address_cast(alloc)).get_slab_metadata());
      Pagemap::set_metaentry(address_cast(alloc), size, t);

      /*
       * On CHERI, the passed alloc has had its bounds narrowed to just the
       * Chunk, and so we retrieve the Arena-bounded cap for use in the
       * remainder of the backend.
       */
      capptr::Arena<void> arena = Authmap::amplify(alloc);

      // Calculate the extra bytes required to store the client meta-data.
      size_t extra_bytes = SlabMetadata::get_extra_bytes(sizeclass);

      auto meta_size = bits::next_pow2(sizeof(SlabMetadata) + extra_bytes);
      local_state.get_meta_range().dealloc_range(
        capptr::Arena<void>::unsafe_from(&slab_metadata), meta_size);

      local_state.get_object_range()->dealloc_range(arena, size);
    }

    SNMALLOC_FAST_PATH static capptr::Alloc<void>
    capptr_rederive_alloc(capptr::Alloc<void> a, size_t objsize)
    {
      return capptr_to_user_address_control(
        Aal::capptr_bound<void, capptr::bounds::AllocFull>(
          Authmap::amplify(a), objsize));
    }

    template<bool potentially_out_of_range = false>
    SNMALLOC_FAST_PATH static const PagemapEntry& get_metaentry(address_t p)
    {
      return Pagemap::template get_metaentry<potentially_out_of_range>(p);
    }

    static size_t get_current_usage()
    {
      Stats stats_state;
      return stats_state.get_current_usage();
    }

    static size_t get_peak_usage()
    {
      Stats stats_state;
      return stats_state.get_peak_usage();
    }
  };
} // namespace snmalloc
