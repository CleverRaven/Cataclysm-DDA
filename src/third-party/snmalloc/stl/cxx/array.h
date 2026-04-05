#pragma once

#include <array>

namespace snmalloc
{
  namespace stl
  {
    template<typename T, size_t N>
    using Array = std::array<T, N>;

    using std::begin;
    using std::end;
  } // namespace stl
} // namespace snmalloc
