// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com


#ifndef CHAISCRIPT_UTILITY_UTILITY_HPP_
#define CHAISCRIPT_UTILITY_UTILITY_HPP_

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../language/chaiscript_common.hpp"
#include "../dispatchkit/register_function.hpp"
#include "../dispatchkit/operators.hpp"


namespace chaiscript 
{
  namespace utility
  {

    /// Single step command for registering a class with ChaiScript
    /// 
    /// \param[in,out] t_module Model to add class to
    /// \param[in] t_class_name Name of the class being registered
    /// \param[in] t_constructors Vector of constructors to add
    /// \param[in] t_funcs Vector of methods to add
    ///
    /// \example Adding a basic class to ChaiScript in one step
    /// 
    /// \code
    /// chaiscript::utility::add_class<test>(*m,
    ///      "test",
    ///      { constructor<test ()>(),
    ///        constructor<test (const test &)>() },
    ///      { {fun(&test::function), "function"},
    ///        {fun(&test::function2), "function2"},
    ///        {fun(&test::function3), "function3"},
    ///        {fun(static_cast<std::string(test::*)(double)>(&test::function_overload)), "function_overload" },
    ///        {fun(static_cast<std::string(test::*)(int)>(&test::function_overload)), "function_overload" },
    ///        {fun(static_cast<test & (test::*)(const test &)>(&test::operator=)), "=" }
    ///        }
    ///      );
    /// 
    template<typename Class, typename ModuleType>
      void add_class(ModuleType &t_module,
          const std::string &t_class_name,
          const std::vector<chaiscript::Proxy_Function> &t_constructors,
          const std::vector<std::pair<chaiscript::Proxy_Function, std::string>> &t_funcs)
      {
        t_module.add(chaiscript::user_type<Class>(), t_class_name); 

        for(const chaiscript::Proxy_Function &ctor: t_constructors)
        {
          t_module.add(ctor, t_class_name);
        }

        for(const auto &fun: t_funcs)
        {
          t_module.add(fun.first, fun.second);
        }
      }

    template<typename Enum, typename ModuleType>
      typename std::enable_if<std::is_enum<Enum>::value, void>::type
      add_class(ModuleType &t_module,
        const std::string &t_class_name,
        const std::vector<std::pair<typename std::underlying_type<Enum>::type, std::string>> &t_constants
        )
      {
        t_module.add(chaiscript::user_type<Enum>(), t_class_name);

        t_module.add(chaiscript::constructor<Enum ()>(), t_class_name);
        t_module.add(chaiscript::constructor<Enum (const Enum &)>(), t_class_name);

        using namespace chaiscript::bootstrap::operators;
        equal<Enum>(t_module);
        not_equal<Enum>(t_module);
        assign<Enum>(t_module);

        t_module.add(chaiscript::fun([](const Enum &e, const int &i) { return e == i; }), "==");
        t_module.add(chaiscript::fun([](const int &i, const Enum &e) { return i == e; }), "==");

        for (const auto &constant : t_constants)
        {
          t_module.add_global_const(chaiscript::const_var(Enum(constant.first)), constant.second);
        }
      }

    template<typename EnumClass, typename ModuleType>
      typename std::enable_if<std::is_enum<EnumClass>::value, void>::type
      add_class(ModuleType &t_module,
        const std::string &t_class_name,
        const std::vector<std::pair<EnumClass, std::string>> &t_constants
        )
      {
        t_module.add(chaiscript::user_type<EnumClass>(), t_class_name);

        t_module.add(chaiscript::constructor<EnumClass()>(), t_class_name);
        t_module.add(chaiscript::constructor<EnumClass(const EnumClass &)>(), t_class_name);

        using namespace chaiscript::bootstrap::operators;
        equal<EnumClass>(t_module);
        not_equal<EnumClass>(t_module);
        assign<EnumClass>(t_module);

        for (const auto &constant : t_constants)
        {
          t_module.add_global_const(chaiscript::const_var(EnumClass(constant.first)), constant.second);
        }
      }
  }
}

#endif

