// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_CALLABLE_TRAITS_HPP_
#define CHAISCRIPT_CALLABLE_TRAITS_HPP_

#include <memory>

namespace chaiscript {
  namespace dispatch {
    namespace detail {

      template<typename Class, typename ... Param>
      struct Constructor
      {
        template<typename ... Inner>
        std::shared_ptr<Class> operator()(Inner&& ... inner) const {
          return std::make_shared<Class>(std::forward<Inner>(inner)...);
        }
      };

      template<typename Ret, typename Class, typename ... Param>
      struct Const_Caller
      {
        explicit Const_Caller(Ret (Class::*t_func)(Param...) const) : m_func(t_func) {}

        template<typename ... Inner>
        Ret operator()(const Class &o, Inner&& ... inner) const {
          return (o.*m_func)(std::forward<Inner>(inner)...);
        }

        Ret (Class::*m_func)(Param...) const;
      };

      template<typename Ret, typename ... Param>
      struct Fun_Caller
      {
        explicit Fun_Caller(Ret( * t_func)(Param...) ) : m_func(t_func) {}

        template<typename ... Inner>
        Ret operator()(Inner&& ... inner) const {
          return (m_func)(std::forward<Inner>(inner)...);
        }

        Ret(*m_func)(Param...);
      };

      template<typename Ret, typename Class, typename ... Param>
      struct Caller
      {
        explicit Caller(Ret (Class::*t_func)(Param...)) : m_func(t_func) {}

        template<typename ... Inner>
        Ret operator()(Class &o, Inner&& ... inner) const {
          return (o.*m_func)(std::forward<Inner>(inner)...);
        }

        Ret (Class::*m_func)(Param...);
      };

      template<typename T>
        struct Arity
        {
        };

      template<typename Ret, typename ... Params>
      struct Arity<Ret (Params...)>
        {
          static const size_t arity = sizeof...(Params);
        };


      template<typename T>
        struct Function_Signature
        {
        };

      template<typename Ret, typename ... Params>
      struct Function_Signature<Ret (Params...)>
        {
          typedef Ret Return_Type;
          typedef Ret (Signature)(Params...);
        };

      template<typename Ret, typename T, typename ... Params>
      struct Function_Signature<Ret (T::*)(Params...) const>
        {
          typedef Ret Return_Type;
          typedef Ret (Signature)(Params...);
        };


      template<typename T>
        struct Callable_Traits
        {
          typedef typename Function_Signature<decltype(&T::operator())>::Signature Signature;
          typedef typename Function_Signature<decltype(&T::operator())>::Return_Type Return_Type;
        };
    }
  }
}

#endif

