// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_UTILITY_FNV1A_HPP_
#define CHAISCRIPT_UTILITY_FNV1A_HPP_


#include <cstdint>
#include "../chaiscript_defines.hpp"


namespace chaiscript
{


  namespace utility
  {


    static constexpr std::uint32_t fnv1a_32(const char *s, std::uint32_t h = 0x811c9dc5) {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#ifdef CHAISCRIPT_MSVC
#pragma warning(push)
#pragma warning(disable : 4307)
#endif
      return (*s == 0) ? h : fnv1a_32(s+1, ((h ^ (*s)) * 0x01000193));
#ifdef CHAISCRIPT_MSVC
#pragma warning(pop)
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    }


  }


}

#endif
