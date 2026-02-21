#pragma once

#include "../ds/ds.h"
#include "empty_range.h"
#include "lockrange.h"
#include "staticrange.h"

namespace snmalloc
{
  /**
   * Makes the supplied ParentRange into a global variable,
   * and protects access with a lock.
   */
  struct GlobalRange
  {
    template<typename ParentRange = EmptyRange<>>
    class Type : public Pipe<ParentRange, LockRange, StaticRange>
    {};
  };
} // namespace snmalloc
