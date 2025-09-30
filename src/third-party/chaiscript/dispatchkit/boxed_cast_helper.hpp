// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com


#ifndef CHAISCRIPT_BOXED_CAST_HELPER_HPP_
#define CHAISCRIPT_BOXED_CAST_HELPER_HPP_

#include <memory>
#include <type_traits>

#include "boxed_value.hpp"
#include "type_info.hpp"


namespace chaiscript 
{
  class Type_Conversions_State;

  namespace detail
  {
    // Cast_Helper_Inner helper classes

    template<typename T>
      T* throw_if_null(T *t)
      {
        if (t) { return t; }
        throw std::runtime_error("Attempted to dereference null Boxed_Value");
      }

    template<typename T>
    static const T *verify_type_no_throw(const Boxed_Value &ob, const std::type_info &ti, const T *ptr) {
      if (ob.get_type_info() == ti) {
        return ptr;
      } else {
        throw chaiscript::detail::exception::bad_any_cast();
      }
    }

    template<typename T>
    static T *verify_type_no_throw(const Boxed_Value &ob, const std::type_info &ti, T *ptr) {
      if (!ob.is_const() && ob.get_type_info() == ti) {
        return ptr;
      } else {
        throw chaiscript::detail::exception::bad_any_cast();
      }
    }


    template<typename T>
    static const T *verify_type(const Boxed_Value &ob, const std::type_info &ti, const T *ptr) {
      if (ob.get_type_info().bare_equal_type_info(ti)) {
        return throw_if_null(ptr);
      } else {
        throw chaiscript::detail::exception::bad_any_cast();
      }
    }

    template<typename T>
    static T *verify_type(const Boxed_Value &ob, const std::type_info &ti, T *ptr) {
      if (!ob.is_const() && ob.get_type_info().bare_equal_type_info(ti)) {
        return throw_if_null(ptr);
      } else {
        throw chaiscript::detail::exception::bad_any_cast();
      }
    }

    /// Generic Cast_Helper_Inner, for casting to any type
    template<typename Result>
      struct Cast_Helper_Inner
      {
        static Result cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return *static_cast<const Result *>(verify_type(ob, typeid(Result), ob.get_const_ptr()));
        }
      };

    template<typename Result>
      struct Cast_Helper_Inner<const Result> : Cast_Helper_Inner<Result>
      {
      };


    /// Cast_Helper_Inner for casting to a const * type
    template<typename Result>
      struct Cast_Helper_Inner<const Result *>
      {
        static const Result * cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return static_cast<const Result *>(verify_type_no_throw(ob, typeid(Result), ob.get_const_ptr()));
        }
      };

    /// Cast_Helper_Inner for casting to a * type
    template<typename Result>
      struct Cast_Helper_Inner<Result *>
      {
        static Result * cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return static_cast<Result *>(verify_type_no_throw(ob, typeid(Result), ob.get_ptr()));
        }
      };

    template<typename Result>
    struct Cast_Helper_Inner<Result * const &> : public Cast_Helper_Inner<Result *>
    {
    };

    template<typename Result>
    struct Cast_Helper_Inner<const Result * const &> : public Cast_Helper_Inner<const Result *>
    {
    };


    /// Cast_Helper_Inner for casting to a & type
    template<typename Result>
      struct Cast_Helper_Inner<const Result &>
      {
        static const Result & cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return *static_cast<const Result *>(verify_type(ob, typeid(Result), ob.get_const_ptr()));
        }
      };



    /// Cast_Helper_Inner for casting to a & type
    template<typename Result>
      struct Cast_Helper_Inner<Result &>
      {
        static Result& cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return *static_cast<Result *>(verify_type(ob, typeid(Result), ob.get_ptr()));
        }
      };

    /// Cast_Helper_Inner for casting to a && type
    template<typename Result>
      struct Cast_Helper_Inner<Result &&>
      {
        static Result&& cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return std::move(*static_cast<Result *>(verify_type(ob, typeid(Result), ob.get_ptr())));
        }
      };

    /// Cast_Helper_Inner for casting to a std::unique_ptr<> && type
    /// \todo Fix the fact that this has to be in a shared_ptr for now
    template<typename Result>
      struct Cast_Helper_Inner<std::unique_ptr<Result> &&>
      {
        static std::unique_ptr<Result> &&cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return std::move(*(ob.get().cast<std::shared_ptr<std::unique_ptr<Result>>>()));
        }
      };

    /// Cast_Helper_Inner for casting to a std::unique_ptr<> & type
    /// \todo Fix the fact that this has to be in a shared_ptr for now
    template<typename Result>
      struct Cast_Helper_Inner<std::unique_ptr<Result> &>
      {
        static std::unique_ptr<Result> &cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return *(ob.get().cast<std::shared_ptr<std::unique_ptr<Result>>>());
        }
      };

    /// Cast_Helper_Inner for casting to a std::unique_ptr<> & type
    /// \todo Fix the fact that this has to be in a shared_ptr for now
    template<typename Result>
      struct Cast_Helper_Inner<const std::unique_ptr<Result> &>
      {
        static std::unique_ptr<Result> &cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return *(ob.get().cast<std::shared_ptr<std::unique_ptr<Result>>>());
        }
      };


    /// Cast_Helper_Inner for casting to a std::shared_ptr<> type
    template<typename Result>
      struct Cast_Helper_Inner<std::shared_ptr<Result> >
      {
        static auto cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return ob.get().cast<std::shared_ptr<Result> >();
        }
      };

    /// Cast_Helper_Inner for casting to a std::shared_ptr<const> type
    template<typename Result>
      struct Cast_Helper_Inner<std::shared_ptr<const Result> >
      {
        static auto cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          if (!ob.get_type_info().is_const())
          {
            return std::const_pointer_cast<const Result>(ob.get().cast<std::shared_ptr<Result> >());
          } else {
            return ob.get().cast<std::shared_ptr<const Result> >();
          }
        }
      };

    /// Cast_Helper_Inner for casting to a const std::shared_ptr<> & type
    template<typename Result>
      struct Cast_Helper_Inner<const std::shared_ptr<Result> > : Cast_Helper_Inner<std::shared_ptr<Result> >
      {
      };

    template<typename Result>
      struct Cast_Helper_Inner<const std::shared_ptr<Result> &> : Cast_Helper_Inner<std::shared_ptr<Result> >
      {
      };

    template<typename Result>
      struct Cast_Helper_Inner<std::shared_ptr<Result> &>
      {
        static_assert(!std::is_const<Result>::value, "Non-const reference to std::shared_ptr<const T> is not supported");
        static auto cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          std::shared_ptr<Result> &res = ob.get().cast<std::shared_ptr<Result> >();
          return ob.pointer_sentinel(res);
        }
      };


    /// Cast_Helper_Inner for casting to a const std::shared_ptr<const> & type
    template<typename Result>
      struct Cast_Helper_Inner<const std::shared_ptr<const Result> > : Cast_Helper_Inner<std::shared_ptr<const Result> >
      {
      };

    template<typename Result>
      struct Cast_Helper_Inner<const std::shared_ptr<const Result> &> : Cast_Helper_Inner<std::shared_ptr<const Result> >
      {
      };


    /// Cast_Helper_Inner for casting to a Boxed_Value type
    template<>
      struct Cast_Helper_Inner<Boxed_Value>
      {
        static Boxed_Value cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return ob;
        }
      };

    /// Cast_Helper_Inner for casting to a Boxed_Value & type
    template<>
      struct Cast_Helper_Inner<Boxed_Value &>
      {
        static std::reference_wrapper<Boxed_Value> cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return std::ref(const_cast<Boxed_Value &>(ob));
        }
      };


    /// Cast_Helper_Inner for casting to a const Boxed_Value & type
    template<>
      struct Cast_Helper_Inner<const Boxed_Value> : Cast_Helper_Inner<Boxed_Value>
      {
      };

    template<>
      struct Cast_Helper_Inner<const Boxed_Value &> : Cast_Helper_Inner<Boxed_Value>
      {
      };


    /// Cast_Helper_Inner for casting to a std::reference_wrapper type
    template<typename Result>
      struct Cast_Helper_Inner<std::reference_wrapper<Result> > : Cast_Helper_Inner<Result &>
      {
      };

    template<typename Result>
      struct Cast_Helper_Inner<const std::reference_wrapper<Result> > : Cast_Helper_Inner<Result &>
      {
      };

    template<typename Result>
      struct Cast_Helper_Inner<const std::reference_wrapper<Result> &> : Cast_Helper_Inner<Result &>
      {
      };

    template<typename Result>
      struct Cast_Helper_Inner<std::reference_wrapper<const Result> > : Cast_Helper_Inner<const Result &>
      {
      };

    template<typename Result>
      struct Cast_Helper_Inner<const std::reference_wrapper<const Result> > : Cast_Helper_Inner<const Result &>
      {
      };

    template<typename Result>
      struct Cast_Helper_Inner<const std::reference_wrapper<const Result> & > : Cast_Helper_Inner<const Result &>
      {
      };

    /// The exposed Cast_Helper object that by default just calls the Cast_Helper_Inner
    template<typename T>
      struct Cast_Helper
      {
        static decltype(auto) cast(const Boxed_Value &ob, const Type_Conversions_State *t_conversions)
        {
          return(Cast_Helper_Inner<T>::cast(ob, t_conversions));
        }
      };
  }
  
}

#endif
