#pragma once

#include "snmalloc/ds_core/defines.h"
#include "snmalloc/stl/type_traits.h"

#include <stddef.h>

namespace snmalloc
{
  namespace stl
  {

    enum class MemoryOrder : int
    {
      RELAXED = __ATOMIC_RELAXED,
      CONSUME = __ATOMIC_CONSUME,
      ACQUIRE = __ATOMIC_ACQUIRE,
      RELEASE = __ATOMIC_RELEASE,
      ACQ_REL = __ATOMIC_ACQ_REL,
      SEQ_CST = __ATOMIC_SEQ_CST
    };

    constexpr MemoryOrder memory_order_relaxed = MemoryOrder::RELAXED;
    constexpr MemoryOrder memory_order_consume = MemoryOrder::CONSUME;
    constexpr MemoryOrder memory_order_acquire = MemoryOrder::ACQUIRE;
    constexpr MemoryOrder memory_order_release = MemoryOrder::RELEASE;
    constexpr MemoryOrder memory_order_acq_rel = MemoryOrder::ACQ_REL;
    constexpr MemoryOrder memory_order_seq_cst = MemoryOrder::SEQ_CST;

    template<typename T>
    class Atomic
    {
      static_assert(
        stl::is_trivially_copyable_v<T> && stl::is_copy_constructible_v<T> &&
          stl::is_move_constructible_v<T> && stl::is_copy_assignable_v<T> &&
          stl::is_move_assignable_v<T>,
        "Atomic<T> requires T to be trivially copyable, copy "
        "constructible, move constructible, copy assignable, "
        "and move assignable.");

      static_assert(
        stl::has_unique_object_representations_v<T>,
        "vendored Atomic only supports types with unique object "
        "representations");

      // type conversion helper to avoid long c++ style casts
      SNMALLOC_FAST_PATH static int order(MemoryOrder mem_ord)
      {
        return static_cast<int>(mem_ord);
      }

      SNMALLOC_FAST_PATH static int infer_failure_order(MemoryOrder mem_ord)
      {
        if (mem_ord == MemoryOrder::RELEASE)
          return order(MemoryOrder::RELAXED);
        if (mem_ord == MemoryOrder::ACQ_REL)
          return order(MemoryOrder::ACQUIRE);
        return order(mem_ord);
      }

      SNMALLOC_FAST_PATH static T* addressof(T& ref)
      {
        return __builtin_addressof(ref);
      }

      // From libc++:
      // require types that are 1, 2, 4, 8, or 16 bytes in length to be aligned
      // to at least their size to be potentially
      // used lock-free
      static constexpr size_t MIN_ALIGNMENT =
        (sizeof(T) & (sizeof(T) - 1)) || (sizeof(T) > 16) ? 0 : sizeof(T);

      static constexpr size_t REQUIRED_ALIGNMENT =
        alignof(T) > MIN_ALIGNMENT ? alignof(T) : MIN_ALIGNMENT;

      alignas(REQUIRED_ALIGNMENT) T val;

    public:
      SNMALLOC_FAST_PATH constexpr Atomic() = default;

      SNMALLOC_FAST_PATH constexpr Atomic(T v) : val(v) {}

      SNMALLOC_FAST_PATH Atomic(const Atomic&) = delete;
      SNMALLOC_FAST_PATH Atomic& operator=(const Atomic&) = delete;

      // Atomic load.
      SNMALLOC_FAST_PATH operator T()
      {
        return load();
      }

      SNMALLOC_FAST_PATH T load(MemoryOrder mem_ord = MemoryOrder::SEQ_CST)
      {
        T res;
        __atomic_load(addressof(val), addressof(res), order(mem_ord));
        return res;
      }

      // Atomic store.
      // NOLINTNEXTLINE(misc-unconventional-assign-operator)
      SNMALLOC_FAST_PATH
      T operator=(T rhs)
      {
        store(rhs);
        return rhs;
      }

      SNMALLOC_FAST_PATH void
      store(T rhs, MemoryOrder mem_ord = MemoryOrder::SEQ_CST)
      {
        __atomic_store(addressof(val), addressof(rhs), order(mem_ord));
      }

      // Atomic compare exchange
      SNMALLOC_FAST_PATH bool compare_exchange_strong(
        T& expected, T desired, MemoryOrder mem_ord = MemoryOrder::SEQ_CST)
      {
        return __atomic_compare_exchange(
          addressof(val),
          addressof(expected),
          addressof(desired),
          false,
          order(mem_ord),
          infer_failure_order(mem_ord));
      }

      // Atomic compare exchange (separate success and failure memory orders)
      SNMALLOC_FAST_PATH bool compare_exchange_strong(
        T& expected,
        T desired,
        MemoryOrder success_order,
        MemoryOrder failure_order)
      {
        return __atomic_compare_exchange(
          addressof(val),
          addressof(expected),
          addressof(desired),
          false,
          order(success_order),
          order(failure_order));
      }

      // Atomic compare exchange (weak version)
      SNMALLOC_FAST_PATH bool compare_exchange_weak(
        T& expected, T desired, MemoryOrder mem_ord = MemoryOrder::SEQ_CST)
      {
        return __atomic_compare_exchange(
          addressof(val),
          addressof(expected),
          addressof(desired),
          true,
          order(mem_ord),
          infer_failure_order(mem_ord));
      }

      // Atomic compare exchange (weak version with separate success and failure
      // memory orders)
      SNMALLOC_FAST_PATH bool compare_exchange_weak(
        T& expected,
        T desired,
        MemoryOrder success_order,
        MemoryOrder failure_order)
      {
        return __atomic_compare_exchange(
          addressof(val),
          addressof(expected),
          addressof(desired),
          true,
          order(success_order),
          order(failure_order));
      }

      SNMALLOC_FAST_PATH T
      exchange(T desired, MemoryOrder mem_ord = MemoryOrder::SEQ_CST)
      {
        T ret;
        __atomic_exchange(
          addressof(val), addressof(desired), addressof(ret), order(mem_ord));
        return ret;
      }

      SNMALLOC_FAST_PATH T
      fetch_add(T increment, MemoryOrder mem_ord = MemoryOrder::SEQ_CST)
      {
        static_assert(stl::is_integral_v<T>, "T must be an integral type.");
        return __atomic_fetch_add(addressof(val), increment, order(mem_ord));
      }

      SNMALLOC_FAST_PATH T
      fetch_or(T mask, MemoryOrder mem_ord = MemoryOrder::SEQ_CST)
      {
        static_assert(stl::is_integral_v<T>, "T must be an integral type.");
        return __atomic_fetch_or(addressof(val), mask, order(mem_ord));
      }

      SNMALLOC_FAST_PATH T
      fetch_and(T mask, MemoryOrder mem_ord = MemoryOrder::SEQ_CST)
      {
        static_assert(stl::is_integral_v<T>, "T must be an integral type.");
        return __atomic_fetch_and(addressof(val), mask, order(mem_ord));
      }

      SNMALLOC_FAST_PATH T
      fetch_sub(T decrement, MemoryOrder mem_ord = MemoryOrder::SEQ_CST)
      {
        static_assert(stl::is_integral_v<T>, "T must be an integral type.");
        return __atomic_fetch_sub(addressof(val), decrement, order(mem_ord));
      }

      SNMALLOC_FAST_PATH T operator++()
      {
        static_assert(stl::is_integral_v<T>, "T must be an integral type.");
        return __atomic_add_fetch(
          addressof(val), 1, order(MemoryOrder::SEQ_CST));
      }

      SNMALLOC_FAST_PATH T operator--()
      {
        static_assert(stl::is_integral_v<T>, "T must be an integral type.");
        return __atomic_sub_fetch(
          addressof(val), 1, order(MemoryOrder::SEQ_CST));
      }

      SNMALLOC_FAST_PATH const T operator++(int)
      {
        static_assert(stl::is_integral_v<T>, "T must be an integral type.");
        return __atomic_fetch_add(
          addressof(val), 1, order(MemoryOrder::SEQ_CST));
      }

      SNMALLOC_FAST_PATH const T operator--(int)
      {
        static_assert(stl::is_integral_v<T>, "T must be an integral type.");
        return __atomic_fetch_sub(
          addressof(val), 1, order(MemoryOrder::SEQ_CST));
      }

      SNMALLOC_FAST_PATH T operator-=(T decrement)
      {
        static_assert(stl::is_integral_v<T>, "T must be an integral type.");
        return __atomic_sub_fetch(
          addressof(val), decrement, order(MemoryOrder::SEQ_CST));
      }
    };

    using AtomicBool = Atomic<bool>;
  } // namespace stl
} // namespace snmalloc
