// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_DYNAMIC_OBJECT_DETAIL_HPP_
#define CHAISCRIPT_DYNAMIC_OBJECT_DETAIL_HPP_

#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "../chaiscript_defines.hpp"
#include "boxed_cast.hpp"
#include "boxed_cast_helper.hpp"
#include "boxed_value.hpp"
#include "proxy_functions.hpp"
#include "type_info.hpp"
#include "dynamic_object.hpp"

namespace chaiscript {
class Type_Conversions;
namespace dispatch {
class Proxy_Function_Base;
}  // namespace dispatch
}  // namespace chaiscript

namespace chaiscript
{
  namespace dispatch
  {
    namespace detail
    {
      /// A Proxy_Function implementation designed for calling a function
      /// that is automatically guarded based on the first param based on the
      /// param's type name
      class Dynamic_Object_Function final : public Proxy_Function_Base
      {
        public:
          Dynamic_Object_Function(
              std::string t_type_name,
              const Proxy_Function &t_func,
              bool t_is_attribute = false)
            : Proxy_Function_Base(t_func->get_param_types(), t_func->get_arity()),
              m_type_name(std::move(t_type_name)), m_func(t_func), m_doti(user_type<Dynamic_Object>()),
              m_is_attribute(t_is_attribute)
          {
            assert( (t_func->get_arity() > 0 || t_func->get_arity() < 0)
                && "Programming error, Dynamic_Object_Function must have at least one parameter (this)");
          }

          Dynamic_Object_Function(
              std::string t_type_name,
              const Proxy_Function &t_func,
              const Type_Info &t_ti,
              bool t_is_attribute = false)
            : Proxy_Function_Base(build_param_types(t_func->get_param_types(), t_ti), t_func->get_arity()),
              m_type_name(std::move(t_type_name)), m_func(t_func), m_ti(t_ti.is_undef()?nullptr:new Type_Info(t_ti)), m_doti(user_type<Dynamic_Object>()),
              m_is_attribute(t_is_attribute)
          {
            assert( (t_func->get_arity() > 0 || t_func->get_arity() < 0)
                && "Programming error, Dynamic_Object_Function must have at least one parameter (this)");
          }


          Dynamic_Object_Function &operator=(const Dynamic_Object_Function) = delete;
          Dynamic_Object_Function(Dynamic_Object_Function &) = delete;

          bool operator==(const Proxy_Function_Base &f) const override
          {
            if (const auto *df = dynamic_cast<const Dynamic_Object_Function *>(&f))
            {
              return df->m_type_name == m_type_name && (*df->m_func) == (*m_func);
            } else {
              return false;
            }
          }

          bool is_attribute_function() const override { return m_is_attribute; } 

          bool call_match(const std::vector<Boxed_Value> &vals, const Type_Conversions_State &t_conversions) const override
          {
            if (dynamic_object_typename_match(vals, m_type_name, m_ti, t_conversions))
            {
              return m_func->call_match(vals, t_conversions);
            } else {
              return false;
            }
          }

          std::vector<Const_Proxy_Function> get_contained_functions() const override
          {
            return {m_func};
          }

        protected:
          Boxed_Value do_call(const std::vector<Boxed_Value> &params, const Type_Conversions_State &t_conversions) const override
          {
            if (dynamic_object_typename_match(params, m_type_name, m_ti, t_conversions))
            {
              return (*m_func)(params, t_conversions);
            } else {
              throw exception::guard_error();
            } 
          }

          bool compare_first_type(const Boxed_Value &bv, const Type_Conversions_State &t_conversions) const override
          {
            return dynamic_object_typename_match(bv, m_type_name, m_ti, t_conversions);
          }

        private:
          static std::vector<Type_Info> build_param_types(
              const std::vector<Type_Info> &t_inner_types, const Type_Info& t_objectti)
          {
            std::vector<Type_Info> types(t_inner_types);

            assert(types.size() > 1);
            //assert(types[1].bare_equal(user_type<Boxed_Value>()));
            types[1] = t_objectti;
            return types;
          }

          bool dynamic_object_typename_match(const Boxed_Value &bv, const std::string &name,
              const std::unique_ptr<Type_Info> &ti, const Type_Conversions_State &t_conversions) const
          {
            if (bv.get_type_info().bare_equal(m_doti))
            {
              try {
                const Dynamic_Object &d = boxed_cast<const Dynamic_Object &>(bv, &t_conversions);
                return name == "Dynamic_Object" || d.get_type_name() == name;
              } catch (const std::bad_cast &) {
                return false;
              } 
            } else {
              if (ti)
              {
                return bv.get_type_info().bare_equal(*ti);
              } else {
                return false;
              }
            }

          }

          bool dynamic_object_typename_match(const std::vector<Boxed_Value> &bvs, const std::string &name,
              const std::unique_ptr<Type_Info> &ti, const Type_Conversions_State &t_conversions) const
          {
            if (!bvs.empty())
            {
              return dynamic_object_typename_match(bvs[0], name, ti, t_conversions);
            } else {
              return false;
            }
          }

          std::string m_type_name;
          Proxy_Function m_func;
          std::unique_ptr<Type_Info> m_ti;
          const Type_Info m_doti;
          const bool m_is_attribute;
      };


      /**
       * A Proxy_Function implementation designed for creating a new
       * Dynamic_Object
       * that is automatically guarded based on the first param based on the
       * param's type name
       */
      class Dynamic_Object_Constructor final : public Proxy_Function_Base
      {
        public:
          Dynamic_Object_Constructor(
              std::string t_type_name,
              const Proxy_Function &t_func)
            : Proxy_Function_Base(build_type_list(t_func->get_param_types()), t_func->get_arity() - 1),
              m_type_name(std::move(t_type_name)), m_func(t_func)
          {
            assert( (t_func->get_arity() > 0 || t_func->get_arity() < 0)
                && "Programming error, Dynamic_Object_Function must have at least one parameter (this)");
          }

          static std::vector<Type_Info> build_type_list(const std::vector<Type_Info> &tl)
          {
            auto begin = tl.begin();
            auto end = tl.end();

            if (begin != end)
            {
              ++begin;
            }

            return std::vector<Type_Info>(begin, end);
          }

          bool operator==(const Proxy_Function_Base &f) const override
          {
            const Dynamic_Object_Constructor *dc = dynamic_cast<const Dynamic_Object_Constructor*>(&f);
            return (dc != nullptr) && dc->m_type_name == m_type_name && (*dc->m_func) == (*m_func);
          }

          bool call_match(const std::vector<Boxed_Value> &vals, const Type_Conversions_State &t_conversions) const override
          {
            std::vector<Boxed_Value> new_vals{Boxed_Value(Dynamic_Object(m_type_name))};
            new_vals.insert(new_vals.end(), vals.begin(), vals.end());

            return m_func->call_match(new_vals, t_conversions);
          }

        protected:
          Boxed_Value do_call(const std::vector<Boxed_Value> &params, const Type_Conversions_State &t_conversions) const override
          {
            auto bv = Boxed_Value(Dynamic_Object(m_type_name), true);
            std::vector<Boxed_Value> new_params{bv};
            new_params.insert(new_params.end(), params.begin(), params.end());

            (*m_func)(new_params, t_conversions);

            return bv;
          }

        private:
          const std::string m_type_name;
          const Proxy_Function m_func;

      };
    }
  }
}
#endif

