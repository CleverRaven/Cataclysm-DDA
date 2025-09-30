// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_UTILITY_STATIC_STRING_HPP_
#define CHAISCRIPT_UTILITY_STATIC_STRING_HPP_

namespace chaiscript
{
  namespace utility
  {

    struct Static_String
    {
      template<size_t N>
        constexpr Static_String(const char (&str)[N])
        : m_size(N-1), data(&str[0])
        {
        }

      constexpr size_t size() const {
        return m_size;
      }

      constexpr const char *c_str() const {
        return data;
      }

      const size_t m_size;
      const char *data = nullptr;
    };
  }
}

#endif
