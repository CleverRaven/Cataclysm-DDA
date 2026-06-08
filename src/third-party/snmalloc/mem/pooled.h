#pragma once

#include "../ds/ds.h"
#include "backend_concept.h"

namespace snmalloc
{
  template<SNMALLOC_CONCEPT(capptr::IsBound) bounds>
  struct Range
  {
    CapPtr<void, bounds> base;
    size_t length;
  };

  template<class T>
  class PoolState;

  // clang-format off
#ifdef __cpp_concepts
  template<typename C, typename T>
  concept Constructable = requires() {
    { C::make() } -> ConceptSame<capptr::Alloc<T>>;
  };
#endif // __cpp_concepts
  

  /**
   * Required to be implemented by all types that are pooled.
   *
   * The constructor of any inherited type must take a Range& as its first
   * argument.  This represents the leftover from pool allocation rounding up to
   * the nearest power of 2. It is valid to ignore this argument, but can be
   * used to optimise meta-data usage at startup.
   */
  template<class T>
  class Pooled
  {
  public:
    template<
      typename TT,
      SNMALLOC_CONCEPT(Constructable<TT>) Construct,
      PoolState<TT>& get_state()>
    friend class Pool;

    /// Used by the pool for chaining together entries when not in use.
    capptr::Alloc<T> next{nullptr};
    /// Used by the pool to keep the list of all entries ever created.
    capptr::Alloc<T> list_next;
    stl::Atomic<bool> in_use{false};

  public:
    void set_in_use()
    {
#ifndef NDEBUG
      if (in_use.exchange(true, stl::memory_order_acq_rel))
        error("Critical error: double use of Pooled Type!");
#else
      in_use.store(true, stl::memory_order_relaxed);
#endif
    }

    void reset_in_use()
    {
#ifndef NDEBUG
      in_use.store(false, stl::memory_order_release);
#else
      in_use.store(false, stl::memory_order_relaxed);
#endif
    }

    bool debug_is_in_use()
    {
      bool result = in_use.exchange(true);
      if (!result)
        in_use.store(false);
      return result;
    }
  };
} // namespace snmalloc
