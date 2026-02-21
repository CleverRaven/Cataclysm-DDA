#pragma once

#ifdef __cpp_concepts
#  include "../ds/ds.h"
#  include "sizeclasstable.h"

#  include <stddef.h>

namespace snmalloc
{
  /**
   * The core of the static pagemap accessor interface: {get,set}_metadata.
   *
   * get_metadata takes a boolean template parameter indicating whether it may
   * be accessing memory that is not known to be committed.
   */
  template<typename Pagemap>
  concept IsReadablePagemap =
    requires(address_t addr, size_t sz, const typename Pagemap::Entry& t) {
      {
        Pagemap::template get_metaentry<true>(addr)
        } -> ConceptSame<const typename Pagemap::Entry&>;

      {
        Pagemap::template get_metaentry<false>(addr)
        } -> ConceptSame<const typename Pagemap::Entry&>;
    };

  /**
   * The core of the static pagemap accessor interface: {get,set}_metadata.
   *
   * get_metadata_mut takes a boolean template parameter indicating whether it
   * may be accessing memory that is not known to be committed.
   *
   * set_metadata updates the entry in the pagemap.
   */
  template<typename Pagemap>
  concept IsWritablePagemap = IsReadablePagemap<Pagemap> &&
    requires(address_t addr, size_t sz, const typename Pagemap::Entry& t) {
      {
        Pagemap::template get_metaentry_mut<true>(addr)
        } -> ConceptSame<typename Pagemap::Entry&>;

      {
        Pagemap::template get_metaentry_mut<false>(addr)
        } -> ConceptSame<typename Pagemap::Entry&>;

      {
        Pagemap::set_metaentry(addr, sz, t)
        } -> ConceptSame<void>;
    };

  /**
   * The pagemap can also be told to commit backing storage for a range of
   * addresses.  This is broken out to a separate concept so that we can
   * annotate which functions expect to do this vs. which merely use the core
   * interface above.  In practice, use IsWritablePagemapWithRegister below,
   * which combines this and the core concept, above.
   */
  template<typename Pagemap>
  concept IsPagemapWithRegister = requires(capptr::Arena<void> p, size_t sz) {
                                    {
                                      Pagemap::register_range(p, sz)
                                      } -> ConceptSame<bool>;
                                  };

  /**
   * The full pagemap accessor interface, with all of {get,set}_metadata and
   * register_range.  Use this to annotate callers that need the full interface
   * and use IsReadablePagemap for callers that merely need {get,set}_metadata,
   * but note that the difference is just for humans and not compilers (since
   * concept checking is lower bounding and does not constrain the templatized
   * code to use only those affordances given by the concept).
   */
  template<typename Pagemap>
  concept IsWritablePagemapWithRegister =
    IsWritablePagemap<Pagemap> && IsPagemapWithRegister<Pagemap>;

  /**
   * The configuration also defines domestication (that is, the difference
   * between Tame and Wild CapPtr bounds).  It exports the intended affordance
   * for testing a Wild pointer and either returning nullptr or the original
   * pointer, now Tame.
   */
  template<typename Config>
  concept IsConfigDomestication =
    requires(typename Config::LocalState* ls, capptr::AllocWild<void> ptr) {
      {
        Config::capptr_domesticate(ls, ptr)
        } -> ConceptSame<capptr::Alloc<void>>;

      {
        Config::capptr_domesticate(ls, ptr.template as_static<char>())
        } -> ConceptSame<capptr::Alloc<char>>;
    };

  class CommonConfig;
  struct Flags;

  template<typename LocalState, typename PagemapEntry, typename Backend>
  concept IsBackend =
    requires(
      LocalState& local_state,
      size_t size,
      uintptr_t ras,
      sizeclass_t sizeclass) {
      {
        Backend::alloc_chunk(local_state, size, ras, sizeclass)
        } -> ConceptSame<
          stl::Pair<capptr::Chunk<void>, typename Backend::SlabMetadata*>>;
    } &&
    requires(LocalState* local_state, size_t size) {
      {
        Backend::template alloc_meta_data<void*>(local_state, size)
        } -> ConceptSame<capptr::Alloc<void>>;
    } &&
    requires(
      LocalState& local_state,
      typename Backend::SlabMetadata& slab_metadata,
      capptr::Alloc<void> alloc,
      size_t size,
      sizeclass_t sizeclass) {
      {
        Backend::dealloc_chunk(
          local_state, slab_metadata, alloc, size, sizeclass)
        } -> ConceptSame<void>;
    } &&
    requires(address_t p) {
      {
        Backend::template get_metaentry<true>(p)
        } -> ConceptSame<const PagemapEntry&>;

      {
        Backend::template get_metaentry<false>(p)
        } -> ConceptSame<const PagemapEntry&>;
    };

  /**
   * Config objects of type T must obey a number of constraints.  They
   * must...
   *
   *  * inherit from CommonConfig (see commonconfig.h)
   *  * specify which PAL is in use via T::Pal
   *  * define a T::LocalState type (and alias it as T::Pagemap::LocalState)
   *  * define T::Options of type snmalloc::Flags
   *  * expose the global allocator pool via T::pool() if pool allocation is
   * used.
   *
   */
  template<typename Config>
  concept IsConfig = stl::is_base_of_v<CommonConfig, Config> &&
    IsPAL<typename Config::Pal> &&
    IsBackend<typename Config::LocalState,
              typename Config::PagemapEntry,
              typename Config::Backend> &&
    requires() {
      typename Config::LocalState;
      typename Config::Backend;
      typename Config::PagemapEntry;

      {
        Config::Options
        } -> ConceptSameModRef<const Flags>;
    } &&
    (
                       requires() {
                         Config::Options.AllocIsPoolAllocated == true;
                         typename Config::GlobalPoolState;
                         {
                           Config::pool()
                           } -> ConceptSame<typename Config::GlobalPoolState&>;
                       } ||
                       requires() {
                         Config::Options.AllocIsPoolAllocated == false;
                       });

  /**
   * The lazy version of the above; please see ds_core/concept.h and use
   * sparingly.
   */
  template<typename Config>
  concept IsConfigLazy = !
  is_type_complete_v<Config> || IsConfig<Config>;

} // namespace snmalloc

#endif
