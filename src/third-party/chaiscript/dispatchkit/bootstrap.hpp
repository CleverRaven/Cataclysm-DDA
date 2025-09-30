// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com


#ifndef CHAISCRIPT_BOOTSTRAP_HPP_
#define CHAISCRIPT_BOOTSTRAP_HPP_

#include "../utility/utility.hpp"
#include "register_function.hpp"

namespace chaiscript 
{
  /// \brief Classes and functions useful for bootstrapping of ChaiScript and adding of new types
  namespace bootstrap
  {
    template<typename T, typename = typename std::enable_if<std::is_array<T>::value>::type >
      void array(const std::string &type, Module& m)
      {
        typedef typename std::remove_extent<T>::type ReturnType;
        m.add(user_type<T>(), type);
        m.add(fun(
              [](T& t, size_t index)->ReturnType &{
                constexpr auto extent = std::extent<T>::value;
                if (extent > 0 && index >= extent) {
                  throw std::range_error("Array index out of range. Received: " + std::to_string(index)  + " expected < " + std::to_string(extent));
                } else {
                  return t[index];
                }
              }
              ), "[]"
            );

        m.add(fun(
              [](const T &t, size_t index)->const ReturnType &{
                constexpr auto extent = std::extent<T>::value;
                if (extent > 0 && index >= extent) {
                  throw std::range_error("Array index out of range. Received: " + std::to_string(index)  + " expected < " + std::to_string(extent));
                } else {
                  return t[index];
                }
              }
              ), "[]"
            );

        m.add(fun(
              [](const T &) {
                constexpr auto extent = std::extent<T>::value;
                return extent;
              }), "size");
      }

    /// \brief Adds a copy constructor for the given type to the given Model
    /// \param[in] type The name of the type. The copy constructor will be named "type".
    /// \param[in,out] m The Module to add the copy constructor to
    /// \tparam T The type to add a copy constructor for
    /// \returns The passed in Module
    template<typename T>
    void copy_constructor(const std::string &type, Module& m)
    {
      m.add(constructor<T (const T &)>(), type);
    }

    /// \brief Add all comparison operators for the templated type. Used during bootstrap, also available to users.
    /// \tparam T Type to create comparison operators for
    /// \param[in,out] m module to add comparison operators to
    /// \returns the passed in Module.
    template<typename T>
    void opers_comparison(Module& m)
    {
      operators::equal<T>(m);
      operators::greater_than<T>(m);
      operators::greater_than_equal<T>(m);
      operators::less_than<T>(m);
      operators::less_than_equal<T>(m);
      operators::not_equal<T>(m);
    }



    /// \brief Adds default and copy constructors for the given type
    /// \param[in] type The name of the type to add the constructors for.
    /// \param[in,out] m The Module to add the basic constructors to
    /// \tparam T Type to generate basic constructors for
    /// \returns The passed in Module
    /// \sa copy_constructor
    /// \sa constructor
    template<typename T>
    void basic_constructors(const std::string &type, Module& m)
    {
      m.add(constructor<T ()>(), type);
      copy_constructor<T>(type, m);
    }

    /// \brief Adds a constructor for a POD type 
    /// \tparam T The type to add the constructor for
    /// \param[in] type The name of the type
    /// \param[in,out] m The Module to add the constructor to
    template<typename T>
    void construct_pod(const std::string &type, Module& m)
    {
      m.add(fun([](const Boxed_Number &bn){ return bn.get_as<T>(); }), type);
    }


    /// Internal function for converting from a string to a value
    /// uses ostream operator >> to perform the conversion
    template<typename Input>
    auto parse_string(const std::string &i)
      -> typename std::enable_if<
             !std::is_same<Input, wchar_t>::value
             && !std::is_same<Input, char16_t>::value
             && !std::is_same<Input, char32_t>::value,
      Input>::type
    {
      std::stringstream ss(i);
      Input t;
      ss >> t;
      return t;
    }

    template<typename Input>
    auto parse_string(const std::string &) 
      -> typename std::enable_if<
             std::is_same<Input, wchar_t>::value
             || std::is_same<Input, char16_t>::value
             || std::is_same<Input, char32_t>::value,
      Input>::type
    {
      throw std::runtime_error("Parsing of wide characters is not yet supported");
    }


    /// Add all common functions for a POD type. All operators, and
    /// common conversions
    template<typename T>
    void bootstrap_pod_type(const std::string &name, Module& m)
    {
      m.add(user_type<T>(), name);
      m.add(constructor<T()>(), name);
      construct_pod<T>(name, m);

      m.add(fun(&parse_string<T>), "to_" + name);
      m.add(fun([](const T t){ return t; }), "to_" + name);
    }


    /// "clone" function for a shared_ptr type. This is used in the case
    /// where you do not want to make a deep copy of an object during cloning
    /// but want to instead maintain the shared_ptr. It is needed internally
    /// for handling of Proxy_Function object (that is,
    /// function variables.
    template<typename Type>
    auto shared_ptr_clone(const std::shared_ptr<Type> &p)
    {
      return p;
    }

    /// Specific version of shared_ptr_clone just for Proxy_Functions
    template<typename Type>
    std::shared_ptr<typename std::remove_const<Type>::type> shared_ptr_unconst_clone(const std::shared_ptr<typename std::add_const<Type>::type> &p)
    {
      return std::const_pointer_cast<typename std::remove_const<Type>::type>(p);
    }



    /// Assignment function for shared_ptr objects, does not perform a copy of the
    /// object pointed to, instead maintains the shared_ptr concept.
    /// Similar to shared_ptr_clone. Used for Proxy_Function.
    template<typename Type>
    Boxed_Value ptr_assign(Boxed_Value lhs, const std::shared_ptr<Type> &rhs)
    {
      if (lhs.is_undef() 
          || (!lhs.get_type_info().is_const() && lhs.get_type_info().bare_equal(chaiscript::detail::Get_Type_Info<Type>::get())))
      {
        lhs.assign(Boxed_Value(rhs));
        return lhs;
      } else {
        throw exception::bad_boxed_cast("type mismatch in pointer assignment");
      }
    }

    /// Class consisting of only static functions. All default bootstrapping occurs
    /// from this class.
    class Bootstrap
    {
    private:
      /// Function allowing for assignment of an unknown type to any other value
      static Boxed_Value unknown_assign(Boxed_Value lhs, Boxed_Value rhs)
      {
        if (lhs.is_undef())
        {
          return (lhs.assign(rhs));
        } else {
          throw exception::bad_boxed_cast("boxed_value has a set type already");
        }
      }

      static void print(const std::string &s)
      {
        fwrite(s.c_str(), 1, s.size(), stdout);
      }

      static void println(const std::string &s)
      {
        puts(s.c_str());
      }


      /// Add all arithmetic operators for PODs
      static void opers_arithmetic_pod(Module& m)
      {
        m.add(fun(&Boxed_Number::equals), "==");
        m.add(fun(&Boxed_Number::less_than), "<");
        m.add(fun(&Boxed_Number::greater_than), ">");
        m.add(fun(&Boxed_Number::greater_than_equal), ">=");
        m.add(fun(&Boxed_Number::less_than_equal), "<=");
        m.add(fun(&Boxed_Number::not_equal), "!=");

        m.add(fun(&Boxed_Number::pre_decrement), "--");
        m.add(fun(&Boxed_Number::pre_increment), "++");
        m.add(fun(&Boxed_Number::sum), "+");
        m.add(fun(&Boxed_Number::unary_plus), "+");
        m.add(fun(&Boxed_Number::unary_minus), "-");
        m.add(fun(&Boxed_Number::difference), "-");
        m.add(fun(&Boxed_Number::assign_bitwise_and), "&=");
        m.add(fun(&Boxed_Number::assign), "=");
        m.add(fun(&Boxed_Number::assign_bitwise_or), "|=");
        m.add(fun(&Boxed_Number::assign_bitwise_xor), "^=");
        m.add(fun(&Boxed_Number::assign_remainder), "%=");
        m.add(fun(&Boxed_Number::assign_shift_left), "<<=");
        m.add(fun(&Boxed_Number::assign_shift_right), ">>=");
        m.add(fun(&Boxed_Number::bitwise_and), "&");
        m.add(fun(&Boxed_Number::bitwise_complement), "~");
        m.add(fun(&Boxed_Number::bitwise_xor), "^");
        m.add(fun(&Boxed_Number::bitwise_or), "|");
        m.add(fun(&Boxed_Number::assign_product), "*=");
        m.add(fun(&Boxed_Number::assign_quotient), "/=");
        m.add(fun(&Boxed_Number::assign_sum), "+=");
        m.add(fun(&Boxed_Number::assign_difference), "-=");
        m.add(fun(&Boxed_Number::quotient), "/");
        m.add(fun(&Boxed_Number::shift_left), "<<");
        m.add(fun(&Boxed_Number::product), "*");
        m.add(fun(&Boxed_Number::remainder), "%");
        m.add(fun(&Boxed_Number::shift_right), ">>");
     }

      /// Create a bound function object. The first param is the function to bind
      /// the remaining parameters are the args to bind into the result
      static Boxed_Value bind_function(const std::vector<Boxed_Value> &params)
      {
        if (params.empty()) {
          throw exception::arity_error(0, 1);
        }

        Const_Proxy_Function f = boxed_cast<Const_Proxy_Function>(params[0]);

        if (f->get_arity() != -1 && size_t(f->get_arity()) != params.size() - 1)
        {
          throw exception::arity_error(static_cast<int>(params.size()), f->get_arity());
        }

        return Boxed_Value(Const_Proxy_Function(std::make_shared<dispatch::Bound_Function>(std::move(f),
          std::vector<Boxed_Value>(params.begin() + 1, params.end()))));
      }


      static bool has_guard(const Const_Proxy_Function &t_pf)
      {
        auto pf = std::dynamic_pointer_cast<const dispatch::Dynamic_Proxy_Function>(t_pf);
        return pf && pf->get_guard();
      }

      static Const_Proxy_Function get_guard(const Const_Proxy_Function &t_pf)
      {
        const auto pf = std::dynamic_pointer_cast<const dispatch::Dynamic_Proxy_Function>(t_pf);
        if (pf && pf->get_guard())
        {
          return pf->get_guard();
        } else {
          throw std::runtime_error("Function does not have a guard");
        }
      }

      template<typename FunctionType>
        static std::vector<Boxed_Value> do_return_boxed_value_vector(FunctionType f,
            const dispatch::Proxy_Function_Base *b)
        {
          auto v = (b->*f)();
 
          std::vector<Boxed_Value> vbv;

          for (const auto &o: v)
          {
            vbv.push_back(const_var(o));
          }

          return vbv;
        }


      static bool has_parse_tree(const chaiscript::Const_Proxy_Function &t_pf)
      {
        const auto pf = std::dynamic_pointer_cast<const chaiscript::dispatch::Dynamic_Proxy_Function>(t_pf);
        return bool(pf);
      }

      static const chaiscript::AST_Node &get_parse_tree(const chaiscript::Const_Proxy_Function &t_pf)
      {
        const auto pf = std::dynamic_pointer_cast<const chaiscript::dispatch::Dynamic_Proxy_Function>(t_pf);
        if (pf)
        {
          return pf->get_parse_tree();
        } else {
          throw std::runtime_error("Function does not have a parse tree");
        }
      }

      template<typename Function>
      static auto return_boxed_value_vector(const Function &f)
      {
        return [f](const dispatch::Proxy_Function_Base *b) {
          return do_return_boxed_value_vector(f, b);
        };
      }


    public:
      /// \brief perform all common bootstrap functions for std::string, void and POD types
      /// \param[in,out] m Module to add bootstrapped functions to
      /// \returns passed in Module
      static void bootstrap(Module& m)
      {
        m.add(user_type<void>(), "void");
        m.add(user_type<bool>(), "bool");
        m.add(user_type<Boxed_Value>(), "Object");
        m.add(user_type<Boxed_Number>(), "Number");
        m.add(user_type<Proxy_Function>(), "Function");
        m.add(user_type<dispatch::Assignable_Proxy_Function>(), "Assignable_Function");
        m.add(user_type<std::exception>(), "exception");

        m.add(fun(&dispatch::Proxy_Function_Base::get_arity), "get_arity");
        m.add(fun(&dispatch::Proxy_Function_Base::operator==), "==");


        m.add(fun(return_boxed_value_vector(&dispatch::Proxy_Function_Base::get_param_types)), "get_param_types");
        m.add(fun(return_boxed_value_vector(&dispatch::Proxy_Function_Base::get_contained_functions)), "get_contained_functions");

        m.add(fun([](const std::exception &e){ return std::string(e.what()); }), "what");

        m.add(user_type<std::out_of_range>(), "out_of_range");
        m.add(user_type<std::logic_error>(), "logic_error");
        m.add(chaiscript::base_class<std::exception, std::logic_error>());
        m.add(chaiscript::base_class<std::logic_error, std::out_of_range>());
        m.add(chaiscript::base_class<std::exception, std::out_of_range>());

        m.add(user_type<std::runtime_error>(), "runtime_error");
        m.add(chaiscript::base_class<std::exception, std::runtime_error>());

        m.add(constructor<std::runtime_error (const std::string &)>(), "runtime_error");

        m.add(user_type<dispatch::Dynamic_Object>(), "Dynamic_Object");
        m.add(constructor<dispatch::Dynamic_Object (const std::string &)>(), "Dynamic_Object");
        m.add(constructor<dispatch::Dynamic_Object ()>(), "Dynamic_Object");
        m.add(fun(&dispatch::Dynamic_Object::get_type_name), "get_type_name");
        m.add(fun(&dispatch::Dynamic_Object::get_attrs), "get_attrs");
        m.add(fun(&dispatch::Dynamic_Object::set_explicit), "set_explicit");
        m.add(fun(&dispatch::Dynamic_Object::is_explicit), "is_explicit");
        m.add(fun(&dispatch::Dynamic_Object::has_attr), "has_attr");

        m.add(fun(static_cast<Boxed_Value & (dispatch::Dynamic_Object::*)(const std::string &)>(&dispatch::Dynamic_Object::get_attr)), "get_attr");
        m.add(fun(static_cast<const Boxed_Value & (dispatch::Dynamic_Object::*)(const std::string &) const>(&dispatch::Dynamic_Object::get_attr)), "get_attr");

        m.add(fun(static_cast<Boxed_Value & (dispatch::Dynamic_Object::*)(const std::string &)>(&dispatch::Dynamic_Object::method_missing)), "method_missing");
        m.add(fun(static_cast<const Boxed_Value & (dispatch::Dynamic_Object::*)(const std::string &) const>(&dispatch::Dynamic_Object::method_missing)), "method_missing");

        m.add(fun(static_cast<Boxed_Value & (dispatch::Dynamic_Object::*)(const std::string &)>(&dispatch::Dynamic_Object::get_attr)), "[]");
        m.add(fun(static_cast<const Boxed_Value & (dispatch::Dynamic_Object::*)(const std::string &) const>(&dispatch::Dynamic_Object::get_attr)), "[]");

        m.eval(R"chaiscript(
          def Dynamic_Object::clone() { 
            auto &new_o = Dynamic_Object(this.get_type_name()); 
            for_each(this.get_attrs(), fun[new_o](x) { new_o.get_attr(x.first) = x.second; } ); 
            new_o; 
          }

          def `=`(Dynamic_Object lhs, Dynamic_Object rhs) : lhs.get_type_name() == rhs.get_type_name()
          {
            for_each(rhs.get_attrs(), fun[lhs](x) { lhs.get_attr(x.first) = clone(x.second); } ); 
          }

          def `!=`(Dynamic_Object lhs, Dynamic_Object rhs) : lhs.get_type_name() == rhs.get_type_name()
          {
            var rhs_attrs := rhs.get_attrs();
            var lhs_attrs := lhs.get_attrs();

            if (rhs_attrs.size() != lhs_attrs.size()) {
              true;
            } else {
              return any_of(rhs_attrs, fun[lhs](x) { !lhs.has_attr(x.first) || lhs.get_attr(x.first) != x.second; } );
            }
          }

          def `==`(Dynamic_Object lhs, Dynamic_Object rhs) : lhs.get_type_name() == rhs.get_type_name()
          {
            var rhs_attrs := rhs.get_attrs();
            var lhs_attrs := lhs.get_attrs();

            if (rhs_attrs.size() != lhs_attrs.size()) {
              false;
            } else {
              return all_of(rhs_attrs, fun[lhs](x) { lhs.has_attr(x.first) && lhs.get_attr(x.first) == x.second; } );
            }
          }
        )chaiscript");

        m.add(fun(&has_guard), "has_guard");
        m.add(fun(&get_guard), "get_guard");

        m.add(fun(&Boxed_Value::is_undef), "is_var_undef");
        m.add(fun(&Boxed_Value::is_null), "is_var_null");
        m.add(fun(&Boxed_Value::is_const), "is_var_const");
        m.add(fun(&Boxed_Value::is_ref), "is_var_reference");
        m.add(fun(&Boxed_Value::is_pointer), "is_var_pointer");
        m.add(fun(&Boxed_Value::is_return_value), "is_var_return_value");
        m.add(fun(&Boxed_Value::reset_return_value), "reset_var_return_value");
        m.add(fun(&Boxed_Value::is_type), "is_type");
        m.add(fun(&Boxed_Value::get_attr), "get_var_attr");
        m.add(fun(&Boxed_Value::copy_attrs), "copy_var_attrs");
        m.add(fun(&Boxed_Value::clone_attrs), "clone_var_attrs");

        m.add(fun(&Boxed_Value::get_type_info), "get_type_info");
        m.add(user_type<Type_Info>(), "Type_Info");
        m.add(constructor<Type_Info (const Type_Info &)>(), "Type_Info");


        operators::equal<Type_Info>(m);

        m.add(fun(&Type_Info::is_const), "is_type_const");
        m.add(fun(&Type_Info::is_reference), "is_type_reference");
        m.add(fun(&Type_Info::is_void), "is_type_void");
        m.add(fun(&Type_Info::is_undef), "is_type_undef");
        m.add(fun(&Type_Info::is_pointer), "is_type_pointer");
        m.add(fun(&Type_Info::is_arithmetic), "is_type_arithmetic");
        m.add(fun(&Type_Info::name), "cpp_name");
        m.add(fun(&Type_Info::bare_name), "cpp_bare_name");
        m.add(fun(&Type_Info::bare_equal), "bare_equal");


        basic_constructors<bool>("bool", m);
        operators::assign<bool>(m);
        operators::equal<bool>(m);
        operators::not_equal<bool>(m);

        m.add(fun([](const std::string &s) { return s; }), "to_string");
        m.add(fun([](const bool b) { return std::string(b?"true":"false"); }), "to_string");
        m.add(fun(&unknown_assign), "=");
        m.add(fun([](const Boxed_Value &bv) { throw bv; }), "throw");

        m.add(fun([](const char c) { return std::string(1, c); }), "to_string");
        m.add(fun(&Boxed_Number::to_string), "to_string");


        bootstrap_pod_type<double>("double", m);
        bootstrap_pod_type<long double>("long_double", m);
        bootstrap_pod_type<float>("float", m);
        bootstrap_pod_type<int>("int", m);
        bootstrap_pod_type<long>("long", m);
        bootstrap_pod_type<unsigned int>("unsigned_int", m);
        bootstrap_pod_type<unsigned long>("unsigned_long", m);
        bootstrap_pod_type<long long>("long_long", m);
        bootstrap_pod_type<unsigned long long>("unsigned_long_long", m);
        bootstrap_pod_type<size_t>("size_t", m);
        bootstrap_pod_type<char>("char", m);
        bootstrap_pod_type<wchar_t>("wchar_t", m);
        bootstrap_pod_type<char16_t>("char16_t", m);
        bootstrap_pod_type<char32_t>("char32_t", m);
        bootstrap_pod_type<std::int8_t>("int8_t", m);
        bootstrap_pod_type<std::int16_t>("int16_t", m);
        bootstrap_pod_type<std::int32_t>("int32_t", m);
        bootstrap_pod_type<std::int64_t>("int64_t", m);
        bootstrap_pod_type<std::uint8_t>("uint8_t", m);
        bootstrap_pod_type<std::uint16_t>("uint16_t", m);
        bootstrap_pod_type<std::uint32_t>("uint32_t", m);
        bootstrap_pod_type<std::uint64_t>("uint64_t", m);


        operators::logical_compliment<bool>(m);

        opers_arithmetic_pod(m);


        m.add(fun(&Build_Info::version_major), "version_major");
        m.add(fun(&Build_Info::version_minor), "version_minor");
        m.add(fun(&Build_Info::version_patch), "version_patch");
        m.add(fun(&Build_Info::version), "version");
        m.add(fun(&Build_Info::compiler_version), "compiler_version");
        m.add(fun(&Build_Info::compiler_name), "compiler_name");
        m.add(fun(&Build_Info::compiler_id), "compiler_id");
        m.add(fun(&Build_Info::debug_build), "debug_build");


        m.add(fun(&print), "print_string");
        m.add(fun(&println), "println_string");

        m.add(dispatch::make_dynamic_proxy_function(&bind_function), "bind");

        m.add(fun(&shared_ptr_unconst_clone<dispatch::Proxy_Function_Base>), "clone");
        m.add(fun(&ptr_assign<std::remove_const<dispatch::Proxy_Function_Base>::type>), "=");
        m.add(fun(&ptr_assign<std::add_const<dispatch::Proxy_Function_Base>::type>), "=");
        m.add(chaiscript::base_class<dispatch::Proxy_Function_Base, dispatch::Assignable_Proxy_Function>());
        m.add(fun(
                  [](dispatch::Assignable_Proxy_Function &t_lhs, const std::shared_ptr<const dispatch::Proxy_Function_Base> &t_rhs) {
                    t_lhs.assign(t_rhs);
                  }
                ), "="
              );

        m.add(fun(&Boxed_Value::type_match), "type_match");


        m.add(chaiscript::fun(&has_parse_tree), "has_parse_tree");
        m.add(chaiscript::fun(&get_parse_tree), "get_parse_tree");

        m.add(chaiscript::base_class<std::runtime_error, chaiscript::exception::eval_error>());
        m.add(chaiscript::base_class<std::exception, chaiscript::exception::eval_error>());

        m.add(chaiscript::user_type<chaiscript::exception::arithmetic_error>(), "arithmetic_error");
        m.add(chaiscript::base_class<std::runtime_error, chaiscript::exception::arithmetic_error>());
        m.add(chaiscript::base_class<std::exception, chaiscript::exception::arithmetic_error>());


//        chaiscript::bootstrap::standard_library::vector_type<std::vector<std::shared_ptr<chaiscript::AST_Node> > >("AST_NodeVector", m);


        chaiscript::utility::add_class<chaiscript::exception::eval_error>(m,
            "eval_error",
            { },
            { {fun(&chaiscript::exception::eval_error::reason), "reason"},
              {fun(&chaiscript::exception::eval_error::pretty_print), "pretty_print"},
              {fun([](const chaiscript::exception::eval_error &t_eval_error) { 
                  std::vector<Boxed_Value> retval;
                  std::transform(t_eval_error.call_stack.begin(), t_eval_error.call_stack.end(),
                                 std::back_inserter(retval),
                                 &chaiscript::var<const chaiscript::AST_Node_Trace &>);
                  return retval;
                }), "call_stack"} }
            );


        chaiscript::utility::add_class<chaiscript::File_Position>(m,
            "File_Position",
            { constructor<File_Position()>(),
              constructor<File_Position(int, int)>() },
            { {fun(&File_Position::line), "line"},
              {fun(&File_Position::column), "column"} }
            );


        chaiscript::utility::add_class<AST_Node>(m,
            "AST_Node",
            {  },
            { {fun(&AST_Node::text), "text"},
              {fun(&AST_Node::identifier), "identifier"},
              {fun(&AST_Node::filename), "filename"},
              {fun(&AST_Node::start), "start"},
              {fun(&AST_Node::end), "end"},
              {fun(&AST_Node::to_string), "to_string"},
              {fun([](const chaiscript::AST_Node &t_node) -> std::vector<Boxed_Value> { 
                std::vector<Boxed_Value> retval;
                const auto children = t_node.get_children();
                std::transform(children.begin(), children.end(),
                               std::back_inserter(retval),
                               &chaiscript::var<const std::reference_wrapper<chaiscript::AST_Node> &>);
                return retval;
              }), "children"}
            }
            );

      }
    };
  }
}

#endif

