// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com


#ifndef CHAISCRIPT_FUNCTION_CALL_HPP_
#define CHAISCRIPT_FUNCTION_CALL_HPP_

#include <functional>
#include <string>
#include <vector>

#include "boxed_cast.hpp"
#include "function_call_detail.hpp"
#include "proxy_functions.hpp"
#include "callable_traits.hpp"

namespace chaiscript {
class Boxed_Value;
class Type_Conversions_State;
namespace detail {
template <typename T> struct Cast_Helper;
}  // namespace detail
}  // namespace chaiscript

namespace chaiscript
{
  namespace dispatch
  {
    /// Build a function caller that knows how to dispatch on a set of functions
    /// example:
    /// std::function<void (int)> f =
    ///      build_function_caller(dispatchkit.get_function("print"));
    /// \returns A std::function object for dispatching
    /// \param[in] funcs the set of functions to dispatch on.
    template<typename FunctionType>
      std::function<FunctionType> functor(const std::vector<Const_Proxy_Function> &funcs, const Type_Conversions_State *t_conversions)
      {
        const bool has_arity_match = std::any_of(funcs.begin(), funcs.end(),
            [](const Const_Proxy_Function &f) {
              return f->get_arity() == -1 || size_t(f->get_arity()) == chaiscript::dispatch::detail::Arity<FunctionType>::arity;
            });

        if (!has_arity_match) {
          throw exception::bad_boxed_cast(user_type<Const_Proxy_Function>(), typeid(std::function<FunctionType>));
        }

        FunctionType *p=nullptr;
        return detail::build_function_caller_helper(p, funcs, t_conversions);
      }

    /// Build a function caller for a particular Proxy_Function object
    /// useful in the case that a function is being pass out from scripting back
    /// into code
    /// example:
    /// void my_function(Proxy_Function f)
    /// {
    ///   std::function<void (int)> local_f =
    ///      build_function_caller(f);
    /// }
    /// \returns A std::function object for dispatching
    /// \param[in] func A function to execute.
    template<typename FunctionType>
      std::function<FunctionType> functor(Const_Proxy_Function func, const Type_Conversions_State *t_conversions)
      {
        return functor<FunctionType>(std::vector<Const_Proxy_Function>({std::move(func)}), t_conversions);
      }

    /// Helper for automatically unboxing a Boxed_Value that contains a function object
    /// and creating a typesafe C++ function caller from it.
    template<typename FunctionType>
      std::function<FunctionType> functor(const Boxed_Value &bv, const Type_Conversions_State *t_conversions)
      {
        return functor<FunctionType>(boxed_cast<Const_Proxy_Function >(bv, t_conversions), t_conversions);
      }
  }

  namespace detail{
    /// Cast helper to handle automatic casting to const std::function &
    template<typename Signature>
      struct Cast_Helper<const std::function<Signature> &>
      {
        static std::function<Signature> cast(const Boxed_Value &ob, const Type_Conversions_State *t_conversions)
        {
          if (ob.get_type_info().bare_equal(user_type<Const_Proxy_Function>()))
          {
            return dispatch::functor<Signature>(ob, t_conversions);
          } else {
            return Cast_Helper_Inner<const std::function<Signature> &>::cast(ob, t_conversions);
          }
        }
      };

    /// Cast helper to handle automatic casting to std::function
    template<typename Signature>
      struct Cast_Helper<std::function<Signature> >
      {
        static std::function<Signature> cast(const Boxed_Value &ob, const Type_Conversions_State *t_conversions)
        {
          if (ob.get_type_info().bare_equal(user_type<Const_Proxy_Function>()))
          {
            return dispatch::functor<Signature>(ob, t_conversions);
          } else {
            return Cast_Helper_Inner<std::function<Signature> >::cast(ob, t_conversions);
          }
        }
      };

    /// Cast helper to handle automatic casting to const std::function
    template<typename Signature>
      struct Cast_Helper<const std::function<Signature> >
      {
        static std::function<Signature> cast(const Boxed_Value &ob, const Type_Conversions_State *t_conversions)
        {
          if (ob.get_type_info().bare_equal(user_type<Const_Proxy_Function>()))
          {
            return dispatch::functor<Signature>(ob, t_conversions);
          } else {
            return Cast_Helper_Inner<const std::function<Signature> >::cast(ob, t_conversions);
          }
        }
      };
  }
}

#endif

