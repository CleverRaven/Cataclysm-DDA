#pragma once

#ifdef SNMALLOC_USE_SELF_VENDORED_STL
#  include "snmalloc/stl/gnu/utility.h"
#else
#  include "snmalloc/stl/cxx/utility.h"
#endif
