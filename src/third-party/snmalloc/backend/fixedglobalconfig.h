#pragma once

#include "../backend_helpers/backend_helpers.h"
#include "../mem/secondary/default.h"
#include "snmalloc/stl/type_traits.h"
#include "standard_range.h"

#include <stddef.h>

namespace snmalloc
{
  /**
   * A single fixed address range allocator configuration
   */
  template<
    SNMALLOC_CONCEPT(IsPAL) PAL,
    typename ClientMetaDataProvider = NoClientMetaDataProvider,
    typename SecondaryAllocator_ = DefaultSecondaryAllocator>
  class FixedRangeConfig final : public CommonConfig
  {
  public:
    using PagemapEntry = DefaultPagemapEntry<ClientMetaDataProvider>;
    using ClientMeta = ClientMetaDataProvider;
    using SecondaryAllocator = SecondaryAllocator_;

  private:
    using ConcretePagemap =
      FlatPagemap<MIN_CHUNK_BITS, PagemapEntry, PAL, true>;

    using Pagemap = BasicPagemap<PAL, ConcretePagemap, PagemapEntry, true>;

    struct Authmap
    {
      static inline capptr::Arena<void> arena;

      template<bool potentially_out_of_range = false>
      static SNMALLOC_FAST_PATH capptr::Arena<void>
      amplify(capptr::Alloc<void> c)
      {
        return Aal::capptr_rebound(arena, c);
      }
    };

  public:
    using LocalState = StandardLocalState<PAL, Pagemap>;

    using GlobalPoolState = PoolState<Allocator<FixedRangeConfig>>;

    using Backend =
      BackendAllocator<PAL, PagemapEntry, Pagemap, Authmap, LocalState>;
    using Pal = PAL;

  private:
    inline static GlobalPoolState alloc_pool;

  public:
    static GlobalPoolState& pool()
    {
      return alloc_pool;
    }

    /*
     * The obvious
     * `static constexpr Flags Options{.HasDomesticate = true};` fails on
     * Ubuntu 18.04 with an error "sorry, unimplemented: non-trivial
     * designated initializers not supported".
     * The following was copied from domestication.cc test with the following
     * comment:
     * C++, even as late as C++20, has some really quite strict limitations on
     * designated initializers.  However, as of C++17, we can have constexpr
     * lambdas and so can use more of the power of the statement fragment of
     * C++, and not just its initializer fragment, to initialize a non-prefix
     * subset of the flags (in any order, at that).
     */
    static constexpr Flags Options = []() constexpr {
      Flags opts = {};
      opts.HasDomesticate = true;
      return opts;
    }();

    static void init(LocalState* local_state, void* base, size_t length)
    {
      UNUSED(local_state);

      auto [heap_base, heap_length] =
        Pagemap::concretePagemap.init(base, length);

      // Make this a alloc_config constant.
      if (length < MIN_HEAP_SIZE_FOR_THREAD_LOCAL_BUDDY)
      {
        LocalState::set_small_heap();
      }

      Authmap::arena = capptr::Arena<void>::unsafe_from(heap_base);

      Pagemap::register_range(Authmap::arena, heap_length);

      // Push memory into the global range.
      range_to_pow_2_blocks<MIN_CHUNK_BITS>(
        capptr::Arena<void>::unsafe_from(heap_base),
        heap_length,
        [&](capptr::Arena<void> p, size_t sz, bool) {
          typename LocalState::GlobalR g;
          g.dealloc_range(p, sz);
        });
    }

    /* Verify that a pointer points into the region managed by this config */
    template<typename T, SNMALLOC_CONCEPT(capptr::IsBound) B>
    static SNMALLOC_FAST_PATH CapPtr<
      T,
      typename B::template with_wildness<capptr::dimension::Wildness::Tame>>
    capptr_domesticate(LocalState* ls, CapPtr<T, B> p)
    {
      static_assert(B::wildness == capptr::dimension::Wildness::Wild);

      static const size_t sz = sizeof(
        stl::
          conditional_t<stl::is_same_v<stl::remove_cv_t<T>, void>, void*, T>);

      UNUSED(ls);
      auto address = address_cast(p);
      auto [base, length] = Pagemap::get_bounds();
      if ((address - base > (length - sz)) || (length < sz))
      {
        return nullptr;
      }

      return CapPtr<
        T,
        typename B::template with_wildness<capptr::dimension::Wildness::Tame>>::
        unsafe_from(p.unsafe_ptr());
    }
  };
}
