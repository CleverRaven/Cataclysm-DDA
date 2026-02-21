#pragma once

#include "../aal/aal.h"

/**
 * This file contains an abstraction of ABA protection. This API should be
 * implementable with double-word compare and exchange or with load-link
 * store conditional.
 *
 * We provide a lock based implementation as a backup for other platforms
 * without appropriate intrinsics.
 */
namespace snmalloc
{
#if !defined(NDEBUG) && !defined(SNMALLOC_DISABLE_ABA_VERIFY)
  // LL/SC typically can only perform one operation at a time
  // check this on other platforms using a thread_local.
  inline thread_local bool operation_in_flight = false;
#endif
  // The !(defined(GCC_NOT_CLANG) && defined(OPEN_ENCLAVE)) is required as
  // GCC is outputing a function call to libatomic, rather than just the x86
  // instruction, this causes problems for linking later. For this case
  // fall back to locked implementation.
#if defined(PLATFORM_IS_X86) && \
  !(defined(GCC_NOT_CLANG) && defined(OPEN_ENCLAVE))
  template<typename T, Construction c = RequiresInit>
  class ABA
  {
  public:
    struct alignas(2 * sizeof(size_t)) Linked
    {
      T* ptr{nullptr};
      uintptr_t aba{0};
    };

    struct Independent
    {
      stl::Atomic<T*> ptr{nullptr};
      stl::Atomic<uintptr_t> aba{0};
    };

    static_assert(
      sizeof(Linked) == sizeof(Independent),
      "Expecting identical struct sizes in union");
    static_assert(
      sizeof(Linked) == (2 * sizeof(size_t)),
      "Expecting ABA to be the size of two pointers");

  private:
    union
    {
      alignas(2 * sizeof(size_t)) stl::Atomic<Linked> linked;
      Independent independent;
    };

  public:
    constexpr ABA() : independent() {}

    void init(T* x)
    {
      independent.ptr.store(x, stl::memory_order_relaxed);
      independent.aba.store(0, stl::memory_order_relaxed);
    }

    struct Cmp;

    Cmp read()
    {
#  if !defined(NDEBUG) && !defined(SNMALLOC_DISABLE_ABA_VERIFY)
      if (operation_in_flight)
        error("Only one inflight ABA operation at a time is allowed.");
      operation_in_flight = true;
#  endif
      return Cmp{
        {independent.ptr.load(stl::memory_order_relaxed),
         independent.aba.load(stl::memory_order_relaxed)},
        this};
    }

    struct Cmp
    {
      Linked old;
      ABA* parent;

      /*
       * MSVC apparently does not like the implicit constructor it creates when
       * asked to interpret its input as C++20; it rejects the construction up
       * in read(), above.  Help it out by making the constructor explicit.
       */
      Cmp(Linked old, ABA* parent) : old(old), parent(parent) {}

      T* ptr()
      {
        return old.ptr;
      }

      bool store_conditional(T* value)
      {
#  if defined(_MSC_VER) && defined(SNMALLOC_VA_BITS_64)
        auto result = _InterlockedCompareExchange128(
          (volatile __int64*)parent,
          (__int64)(old.aba + (uintptr_t)1),
          (__int64)address_cast(value),
          (__int64*)&old);
#  else
#    if defined(__GNUC__) && defined(SNMALLOC_VA_BITS_64) && \
      !defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
#error You must compile with -mcx16 to enable 16-byte atomic compare and swap.
#    endif

        Linked xchg{value, old.aba + 1};
        stl::Atomic<Linked>& addr = parent->linked;

        auto result = addr.compare_exchange_weak(
          old, xchg, stl::memory_order_acq_rel, stl::memory_order_relaxed);
#  endif
        return result;
      }

      ~Cmp()
      {
#  if !defined(NDEBUG) && !defined(SNMALLOC_DISABLE_ABA_VERIFY)
        operation_in_flight = false;
#  endif
      }

      Cmp(const Cmp&) = delete;
      Cmp(Cmp&&) noexcept = default;
    };

    // This method is used in Verona
    T* peek()
    {
      return independent.ptr.load(stl::memory_order_relaxed);
    }
  };
#else
  /**
   * Naive implementation of ABA protection using a spin lock.
   */
  template<typename T, Construction c = RequiresInit>
  class ABA
  {
    stl::Atomic<T*> ptr = nullptr;
    stl::Atomic<bool> lock{false};

  public:
    // This method is used in Verona
    void init(T* x)
    {
      ptr.store(x, stl::memory_order_relaxed);
    }

    struct Cmp;

    Cmp read()
    {
      while (lock.exchange(true, stl::memory_order_acquire))
        Aal::pause();

#  if !defined(NDEBUG) && !defined(SNMALLOC_DISABLE_ABA_VERIFY)
      if (operation_in_flight)
        error("Only one inflight ABA operation at a time is allowed.");
      operation_in_flight = true;
#  endif

      return Cmp{this};
    }

    struct Cmp
    {
      ABA* parent;

    public:
      T* ptr()
      {
        return parent->ptr;
      }

      bool store_conditional(T* t)
      {
        parent->ptr = t;
        return true;
      }

      ~Cmp()
      {
        parent->lock.store(false, stl::memory_order_release);
#  if !defined(NDEBUG) && !defined(SNMALLOC_DISABLE_ABA_VERIFY)
        operation_in_flight = false;
#  endif
      }
    };

    // This method is used in Verona
    T* peek()
    {
      return ptr.load(stl::memory_order_relaxed);
    }
  };
#endif
} // namespace snmalloc
