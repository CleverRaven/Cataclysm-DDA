#pragma once

#include <utility>

namespace snmalloc
{
  namespace stl
  {
    using std::declval;
    using std::exchange;
    using std::forward;
    using std::move;
    template<class T1, class T2>
    using Pair = std::pair<T1, T2>;
  } // namespace stl
} // namespace snmalloc
