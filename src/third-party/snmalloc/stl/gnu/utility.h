#pragma once

#include "snmalloc/ds_core/defines.h"
#include "snmalloc/stl/type_traits.h"

// This is used by clang to provide better analysis of lifetimes.
#if __has_cpp_attribute(_Clang::__lifetimebound__)
#  define SNMALLOC_LIFETIMEBOUND [[_Clang::__lifetimebound__]]
#else
#  define SNMALLOC_LIFETIMEBOUND
#endif

namespace snmalloc
{
  namespace stl
  {
    template<class T>
    [[nodiscard]] inline constexpr T&&
    forward(remove_reference_t<T>& ref) noexcept
    {
      return static_cast<T&&>(ref);
    }

    template<class T>
    [[nodiscard]] inline constexpr T&&
    forward(SNMALLOC_LIFETIMEBOUND remove_reference_t<T>&& ref) noexcept
    {
      static_assert(
        !is_lvalue_reference_v<T>, "cannot forward an rvalue as an lvalue");
      return static_cast<T&&>(ref);
    }

    template<class T>
    [[nodiscard]] inline constexpr remove_reference_t<T>&&
    move(SNMALLOC_LIFETIMEBOUND T&& ref) noexcept
    {
#ifdef __clang__
      using U [[gnu::nodebug]] = remove_reference_t<T>;
#else
      using U = remove_reference_t<T>;
#endif
      return static_cast<U&&>(ref);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
    // First try instantiation with reference types, then fall back to value
    // types. Use int to prioritize reference types.
    template<class T>
    T&& declval_impl(int);
    template<class T>
    T declval_impl(long);
#pragma GCC diagnostic pop

    template<class T>
    constexpr inline decltype(declval_impl<T>(0)) declval() noexcept
    {
      static_assert(
        !is_same_v<T, T>, "declval cannot be used in an evaluation context");
    }

    template<class T1, class T2>
    struct Pair
    {
      T1 first;
      T2 second;
    };

    template<typename A, typename B = A>
    constexpr SNMALLOC_FAST_PATH A exchange(A& obj, B&& new_value)
    {
      A old_value = move(obj);
      obj = forward<B>(new_value);
      return old_value;
    }
  } // namespace stl
} // namespace snmalloc
