// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com


#ifndef CHAISCRIPT_DISPATCHKIT_HPP_
#define CHAISCRIPT_DISPATCHKIT_HPP_

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "../chaiscript_defines.hpp"
#include "../chaiscript_threading.hpp"
#include "bad_boxed_cast.hpp"
#include "boxed_cast.hpp"
#include "boxed_cast_helper.hpp"
#include "boxed_value.hpp"
#include "type_conversions.hpp"
#include "dynamic_object.hpp"
#include "proxy_constructors.hpp"
#include "proxy_functions.hpp"
#include "type_info.hpp"
#include "short_alloc.hpp"

namespace chaiscript {
class Boxed_Number;
}  // namespace chaiscript

namespace chaiscript {
  namespace parser {
    class ChaiScript_Parser_Base;
  }
namespace dispatch {
class Dynamic_Proxy_Function;
class Proxy_Function_Base;
struct Placeholder_Object;
}  // namespace dispatch
}  // namespace chaiscript



/// \namespace chaiscript::dispatch
/// \brief Classes and functions specific to the runtime dispatch side of ChaiScript. Some items may be of use to the end user.

namespace chaiscript
{
  namespace exception
  {
    /// Exception thrown in the case that an object name is invalid because it is a reserved word
    class reserved_word_error : public std::runtime_error
    {
      public:
        explicit reserved_word_error(const std::string &t_word) noexcept
          : std::runtime_error("Reserved word not allowed in object name: " + t_word), m_word(t_word)
        {
        }

        reserved_word_error(const reserved_word_error &) = default;

        ~reserved_word_error() noexcept override = default;

        std::string word() const
        {
          return m_word;
        }

      private:
        std::string m_word;
    };

    /// Exception thrown in the case that an object name is invalid because it contains illegal characters
    class illegal_name_error : public std::runtime_error
    {
      public:
        explicit illegal_name_error(const std::string &t_name) noexcept
          : std::runtime_error("Reserved name not allowed in object name: " + t_name), m_name(t_name)
        {
        }

        illegal_name_error(const illegal_name_error &) = default;

        ~illegal_name_error() noexcept override = default;

        std::string name() const
        {
          return m_name;
        }

      private:
        std::string m_name;
    };


    /// Exception thrown in the case that an object name is invalid because it already exists in current context
    class name_conflict_error : public std::runtime_error
    {
      public:
        explicit name_conflict_error(const std::string &t_name) noexcept
          : std::runtime_error("Name already exists in current context " + t_name), m_name(t_name)
        {
        }

        name_conflict_error(const name_conflict_error &) = default;

        ~name_conflict_error() noexcept override = default;

        std::string name() const
        {
          return m_name;
        }

      private:
        std::string m_name;

    };


    /// Exception thrown in the case that a non-const object was added as a shared object
    class global_non_const : public std::runtime_error
    {
      public:
        global_non_const() noexcept
          : std::runtime_error("a global object must be const")
        {
        }

        global_non_const(const global_non_const &) = default;
        ~global_non_const() noexcept override = default;
    };
  }


  /// \brief Holds a collection of ChaiScript settings which can be applied to the ChaiScript runtime.
  ///        Used to implement loadable module support.
  class Module
  {
    public:
      Module &add(Type_Info ti, std::string name)
      {
        m_typeinfos.emplace_back(ti, std::move(name));
        return *this;
      }

      Module &add(Type_Conversion d)
      {
        m_conversions.push_back(std::move(d));
        return *this;
      }

      Module &add(Proxy_Function f, std::string name)
      {
        m_funcs.emplace_back(std::move(f), std::move(name));
        return *this;
      }

      Module &add_global_const(Boxed_Value t_bv, std::string t_name)
      {
        if (!t_bv.is_const())
        {
          throw chaiscript::exception::global_non_const();
        }

        m_globals.emplace_back(std::move(t_bv), std::move(t_name));
        return *this;
      }


      //Add a bit of ChaiScript to eval during module implementation
      Module &eval(const std::string &str)
      {
        m_evals.push_back(str);
        return *this;
      }

      template<typename Eval, typename Engine>
        void apply(Eval &t_eval, Engine &t_engine) const
        {
          apply(m_typeinfos.begin(), m_typeinfos.end(), t_engine);
          apply(m_funcs.begin(), m_funcs.end(), t_engine);
          apply_eval(m_evals.begin(), m_evals.end(), t_eval);
          apply_single(m_conversions.begin(), m_conversions.end(), t_engine);
          apply_globals(m_globals.begin(), m_globals.end(), t_engine);
        }

      bool has_function(const Proxy_Function &new_f, const std::string &name)
      {
        return std::any_of(m_funcs.begin(), m_funcs.end(), 
            [&](const std::pair<Proxy_Function, std::string> &existing_f) {
              return existing_f.second == name && *(existing_f.first) == *(new_f);
            }
          );
      }
      

    private:
      std::vector<std::pair<Type_Info, std::string>> m_typeinfos;
      std::vector<std::pair<Proxy_Function, std::string>> m_funcs;
      std::vector<std::pair<Boxed_Value, std::string>> m_globals;
      std::vector<std::string> m_evals;
      std::vector<Type_Conversion> m_conversions;

      template<typename T, typename InItr>
        static void apply(InItr begin, const InItr end, T &t) 
        {
          for_each(begin, end, 
              [&t](const auto &obj) {
                try {
                  t.add(obj.first, obj.second);
                } catch (const chaiscript::exception::name_conflict_error &) {
                  /// \todo Should we throw an error if there's a name conflict 
                  ///       while applying a module?
                }
              }
            );
        }

      template<typename T, typename InItr>
        static void apply_globals(InItr begin, InItr end, T &t)
        {
          while (begin != end)
          {
            t.add_global_const(begin->first, begin->second);
            ++begin;
          }
        }

      template<typename T, typename InItr>
        static void apply_single(InItr begin, InItr end, T &t)
        {
          while (begin != end)
          {
            t.add(*begin);
            ++begin;
          }
        }

      template<typename T, typename InItr>
        static void apply_eval(InItr begin, InItr end, T &t)
        {
          while (begin != end)
          {
            t.eval(*begin);
            ++begin;
          }
        }
  };

  /// Convenience typedef for Module objects to be added to the ChaiScript runtime
  typedef std::shared_ptr<Module> ModulePtr;

  namespace detail
  {
    /// A Proxy_Function implementation that is able to take
    /// a vector of Proxy_Functions and perform a dispatch on them. It is 
    /// used specifically in the case of dealing with Function object variables
    class Dispatch_Function final : public dispatch::Proxy_Function_Base
    {
      public:
        explicit Dispatch_Function(std::vector<Proxy_Function> t_funcs)
          : Proxy_Function_Base(build_type_infos(t_funcs), calculate_arity(t_funcs)),
            m_funcs(std::move(t_funcs))
        {
        }

        bool operator==(const dispatch::Proxy_Function_Base &rhs) const override
        {
          try {
            const auto &dispatch_fun = dynamic_cast<const Dispatch_Function &>(rhs);
            return m_funcs == dispatch_fun.m_funcs;
          } catch (const std::bad_cast &) {
            return false;
          }
        }

        std::vector<Const_Proxy_Function> get_contained_functions() const override
        {
          return std::vector<Const_Proxy_Function>(m_funcs.begin(), m_funcs.end());
        }


        static int calculate_arity(const std::vector<Proxy_Function> &t_funcs)
        {
          if (t_funcs.empty()) {
            return -1;
          }

          const auto arity = t_funcs.front()->get_arity();

          for (const auto &func : t_funcs)
          {
            if (arity != func->get_arity())
            {
              // The arities in the list do not match, so it's unspecified
              return -1;
            }
          }

          return arity;
        }

        bool call_match(const std::vector<Boxed_Value> &vals, const Type_Conversions_State &t_conversions) const override
        {
          return std::any_of(std::begin(m_funcs), std::end(m_funcs),
                             [&vals, &t_conversions](const Proxy_Function &f){ return f->call_match(vals, t_conversions); });
        }

      protected:
        Boxed_Value do_call(const std::vector<Boxed_Value> &params, const Type_Conversions_State &t_conversions) const override
        {
          return dispatch::dispatch(m_funcs, params, t_conversions);
        }

      private:
        std::vector<Proxy_Function> m_funcs;

        static std::vector<Type_Info> build_type_infos(const std::vector<Proxy_Function> &t_funcs)
        {
          auto begin = t_funcs.cbegin();
          const auto &end = t_funcs.cend();

          if (begin != end)
          {
            std::vector<Type_Info> type_infos = (*begin)->get_param_types();

            ++begin;

            bool size_mismatch = false;

            while (begin != end)
            {
              std::vector<Type_Info> param_types = (*begin)->get_param_types();

              if (param_types.size() != type_infos.size())
              {
                size_mismatch = true;
              }

              for (size_t i = 0; i < type_infos.size() && i < param_types.size(); ++i)
              {
                if (!(type_infos[i] == param_types[i]))
                {
                  type_infos[i] = detail::Get_Type_Info<Boxed_Value>::get();
                }
              }

              ++begin;
            }

            assert(!type_infos.empty() && " type_info vector size is < 0, this is only possible if something else is broken");

            if (size_mismatch)
            {
              type_infos.resize(1);
            }

            return type_infos;
          }

          return std::vector<Type_Info>();
        }
    };
  }


  namespace detail
  {
    struct Stack_Holder
    {
      //template <class T, std::size_t BufSize = sizeof(T)*20000>
      //  using SmallVector = std::vector<T, short_alloc<T, BufSize>>;

      template <class T>
        using SmallVector = std::vector<T>;
      

      typedef SmallVector<std::pair<std::string, Boxed_Value>> Scope;
      typedef SmallVector<Scope> StackData;
      typedef SmallVector<StackData> Stacks;
      typedef SmallVector<Boxed_Value> Call_Param_List;
      typedef SmallVector<Call_Param_List> Call_Params;

      Stack_Holder()
      {
        push_stack();
        push_call_params();
      }

      void push_stack_data()
      {
        stacks.back().emplace_back();
//        stacks.back().emplace_back(Scope(scope_allocator));
      }

      void push_stack()
      {
        stacks.emplace_back(1);
//        stacks.emplace_back(StackData(1, Scope(scope_allocator), stack_data_allocator));
      }

      void push_call_params()
      {
        call_params.emplace_back();
//        call_params.emplace_back(Call_Param_List(call_param_list_allocator));
      }

      //Scope::allocator_type::arena_type scope_allocator;
      //StackData::allocator_type::arena_type stack_data_allocator;
      //Stacks::allocator_type::arena_type stacks_allocator;
      //Call_Param_List::allocator_type::arena_type call_param_list_allocator;
      //Call_Params::allocator_type::arena_type call_params_allocator;

//      Stacks stacks = Stacks(stacks_allocator);
//      Call_Params call_params = Call_Params(call_params_allocator);

      Stacks stacks;
      Call_Params call_params;

      int call_depth = 0;
    };

    /// Main class for the dispatchkit. Handles management
    /// of the object stack, functions and registered types.
    class Dispatch_Engine
    {

      public:
        typedef std::map<std::string, chaiscript::Type_Info> Type_Name_Map;
        typedef std::vector<std::pair<std::string, Boxed_Value>> Scope;
        typedef Stack_Holder::StackData StackData;

        struct State
        {
          std::vector<std::pair<std::string, std::shared_ptr<std::vector<Proxy_Function>>>> m_functions;
          std::vector<std::pair<std::string, Proxy_Function>> m_function_objects;
          std::vector<std::pair<std::string, Boxed_Value>> m_boxed_functions;
          std::map<std::string, Boxed_Value> m_global_objects;
          Type_Name_Map m_types;
        };

        explicit Dispatch_Engine(chaiscript::parser::ChaiScript_Parser_Base &parser)
          : m_stack_holder(),
            m_parser(parser)
        {
        }

        /// \brief casts an object while applying any Dynamic_Conversion available
        template<typename Type>
          decltype(auto) boxed_cast(const Boxed_Value &bv) const
          {
            Type_Conversions_State state(m_conversions, m_conversions.conversion_saves());
            return(chaiscript::boxed_cast<Type>(bv, &state));
          }

        /// Add a new conversion for upcasting to a base class
        void add(const Type_Conversion &d)
        {
          m_conversions.add_conversion(d);
        }

        /// Add a new named Proxy_Function to the system
        void add(const Proxy_Function &f, const std::string &name)
        {
          add_function(f, name);
        }

        /// Set the value of an object, by name. If the object
        /// is not available in the current scope it is created
        void add(Boxed_Value obj, const std::string &name)
        {
          auto &stack = get_stack_data();

          for (auto stack_elem = stack.rbegin(); stack_elem != stack.rend(); ++stack_elem)
          {
            auto itr = std::find_if(stack_elem->begin(), stack_elem->end(),
                [&](const std::pair<std::string, Boxed_Value> &o) {
                  return o.first == name;
                });

            if (itr != stack_elem->end())
            {
              itr->second = std::move(obj);
              return;
            }
          }

          add_object(name, std::move(obj));
        }

        /// Adds a named object to the current scope
        /// \warning This version does not check the validity of the name
        /// it is meant for internal use only
        Boxed_Value &add_get_object(const std::string &t_name, Boxed_Value obj, Stack_Holder &t_holder)
        {
          auto &stack_elem = get_stack_data(t_holder).back();

          if (std::any_of(stack_elem.begin(), stack_elem.end(),
              [&](const std::pair<std::string, Boxed_Value> &o) {
                return o.first == t_name;
              }))
          {
            throw chaiscript::exception::name_conflict_error(t_name);
          }

          stack_elem.emplace_back(t_name, std::move(obj));
          return stack_elem.back().second;
        }


        /// Adds a named object to the current scope
        /// \warning This version does not check the validity of the name
        /// it is meant for internal use only
        void add_object(const std::string &t_name, Boxed_Value obj, Stack_Holder &t_holder)
        {
          auto &stack_elem = get_stack_data(t_holder).back();

          if (std::any_of(stack_elem.begin(), stack_elem.end(),
              [&](const std::pair<std::string, Boxed_Value> &o) {
                return o.first == t_name;
              }))
          {
            throw chaiscript::exception::name_conflict_error(t_name);
          }

          stack_elem.emplace_back(t_name, std::move(obj));
        }


        /// Adds a named object to the current scope
        /// \warning This version does not check the validity of the name
        /// it is meant for internal use only
        void add_object(const std::string &name, Boxed_Value obj)
        {
          add_object(name, std::move(obj), get_stack_holder());
        }

        /// Adds a new global shared object, between all the threads
        void add_global_const(const Boxed_Value &obj, const std::string &name)
        {
          if (!obj.is_const())
          {
            throw chaiscript::exception::global_non_const();
          }

          chaiscript::detail::threading::unique_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          if (m_state.m_global_objects.find(name) != m_state.m_global_objects.end())
          {
            throw chaiscript::exception::name_conflict_error(name);
          } else {
            m_state.m_global_objects.insert(std::make_pair(name, obj));
          }
        }

        /// Adds a new global (non-const) shared object, between all the threads
        Boxed_Value add_global_no_throw(const Boxed_Value &obj, const std::string &name)
        {
          chaiscript::detail::threading::unique_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          const auto itr = m_state.m_global_objects.find(name);
          if (itr == m_state.m_global_objects.end())
          {
            m_state.m_global_objects.insert(std::make_pair(name, obj));
            return obj;
          } else {
            return itr->second;
          }
        }


        /// Adds a new global (non-const) shared object, between all the threads
        void add_global(const Boxed_Value &obj, const std::string &name)
        {
          chaiscript::detail::threading::unique_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          if (m_state.m_global_objects.find(name) != m_state.m_global_objects.end())
          {
            throw chaiscript::exception::name_conflict_error(name);
          } else {
            m_state.m_global_objects.insert(std::make_pair(name, obj));
          }
        }

        /// Updates an existing global shared object or adds a new global shared object if not found
        void set_global(const Boxed_Value &obj, const std::string &name)
        {
          chaiscript::detail::threading::unique_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          const auto itr = m_state.m_global_objects.find(name);
          if (itr != m_state.m_global_objects.end())
          {
            itr->second.assign(obj);
          } else {
            m_state.m_global_objects.insert(std::make_pair(name, obj));
          }
        }

        /// Adds a new scope to the stack
        void new_scope()
        {
          new_scope(*m_stack_holder);
        }

        /// Pops the current scope from the stack
        void pop_scope()
        {
          pop_scope(*m_stack_holder);
        }

        /// Adds a new scope to the stack
        static void new_scope(Stack_Holder &t_holder)
        {
          t_holder.push_stack_data();
          t_holder.push_call_params();
        }

        /// Pops the current scope from the stack
        static void pop_scope(Stack_Holder &t_holder)
        {
          t_holder.call_params.pop_back();
          StackData &stack = get_stack_data(t_holder);

          assert(!stack.empty());

          stack.pop_back();
        }


        /// Pushes a new stack on to the list of stacks
        static void new_stack(Stack_Holder &t_holder)
        {
          // add a new Stack with 1 element
          t_holder.push_stack();
        }

        static void pop_stack(Stack_Holder &t_holder)
        {
          t_holder.stacks.pop_back();
        }

        /// Searches the current stack for an object of the given name
        /// includes a special overload for the _ place holder object to
        /// ensure that it is always in scope.
        Boxed_Value get_object(const std::string &name, std::atomic_uint_fast32_t &t_loc, Stack_Holder &t_holder) const
        {
          enum class Loc : uint_fast32_t {
            located    = 0x80000000,
            is_local   = 0x40000000,
            stack_mask = 0x0FFF0000,
            loc_mask   = 0x0000FFFF
          };

          uint_fast32_t loc = t_loc;

          if (loc == 0)
          {
            auto &stack = get_stack_data(t_holder);

            // Is it in the stack?
            for (auto stack_elem = stack.rbegin(); stack_elem != stack.rend(); ++stack_elem)
            {
              for (auto s = stack_elem->begin(); s != stack_elem->end(); ++s )
              {
                if (s->first == name) {
                  t_loc = static_cast<uint_fast32_t>(std::distance(stack.rbegin(), stack_elem) << 16)
                              | static_cast<uint_fast32_t>(std::distance(stack_elem->begin(), s))
                              | static_cast<uint_fast32_t>(Loc::located)
                              | static_cast<uint_fast32_t>(Loc::is_local);
                  return s->second;
                }
              }
            }

            t_loc = static_cast<uint_fast32_t>(Loc::located);
          } else if ((loc & static_cast<uint_fast32_t>(Loc::is_local)) != 0u) {
            auto &stack = get_stack_data(t_holder);

            return stack[stack.size() - 1 - ((loc & static_cast<uint_fast32_t>(Loc::stack_mask)) >> 16)][loc & static_cast<uint_fast32_t>(Loc::loc_mask)].second;
          }

          // Is the value we are looking for a global or function?
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          const auto itr = m_state.m_global_objects.find(name);
          if (itr != m_state.m_global_objects.end())
          {
            return itr->second;
          }

          // no? is it a function object?
          auto obj = get_function_object_int(name, loc);
          if (obj.first != loc) { t_loc = uint_fast32_t(obj.first); }

          return obj.second;

        }

        /// Registers a new named type
        void add(const Type_Info &ti, const std::string &name)
        {
          add_global_const(const_var(ti), name + "_type");

          chaiscript::detail::threading::unique_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          m_state.m_types.insert(std::make_pair(name, ti));
        }

        /// Returns the type info for a named type
        Type_Info get_type(const std::string &name, bool t_throw = true) const
        {
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          const auto itr = m_state.m_types.find(name);

          if (itr != m_state.m_types.end())
          {
            return itr->second;
          }

          if (t_throw) {
            throw std::range_error("Type Not Known: " + name);
          } else {
            return Type_Info();
          }
        }

        /// Returns the registered name of a known type_info object
        /// compares the "bare_type_info" for the broadest possible
        /// match
        std::string get_type_name(const Type_Info &ti) const
        {
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          for (const auto & elem : m_state.m_types)
          {
            if (elem.second.bare_equal(ti))
            {
              return elem.first;
            }
          }

          return ti.bare_name();
        }

        /// Return all registered types
        std::vector<std::pair<std::string, Type_Info> > get_types() const
        {
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          return std::vector<std::pair<std::string, Type_Info> >(m_state.m_types.begin(), m_state.m_types.end());
        }

        std::shared_ptr<std::vector<Proxy_Function>> get_method_missing_functions() const
        {
          uint_fast32_t method_missing_loc = m_method_missing_loc;
          auto method_missing_funs = get_function("method_missing", method_missing_loc);
          if (method_missing_funs.first != method_missing_loc) {
            m_method_missing_loc = uint_fast32_t(method_missing_funs.first);
          }

          return std::move(method_missing_funs.second);
        }


        /// Return a function by name
        std::pair<size_t, std::shared_ptr<std::vector< Proxy_Function>>> get_function(const std::string &t_name, const size_t t_hint) const
        {
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          const auto &funs = get_functions_int();

          auto itr = find_keyed_value(funs, t_name, t_hint);

          if (itr != funs.end())
          {
            return std::make_pair(std::distance(funs.begin(), itr), itr->second);
          } else {
            return std::make_pair(size_t(0), std::make_shared<std::vector<Proxy_Function>>());
          }
        }

        /// \returns a function object (Boxed_Value wrapper) if it exists
        /// \throws std::range_error if it does not
        Boxed_Value get_function_object(const std::string &t_name) const
        {
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          return get_function_object_int(t_name, 0).second;
        }

        /// \returns a function object (Boxed_Value wrapper) if it exists
        /// \throws std::range_error if it does not
        /// \warn does not obtain a mutex lock. \sa get_function_object for public version
        std::pair<size_t, Boxed_Value> get_function_object_int(const std::string &t_name, const size_t t_hint) const
        {
          const auto &funs = get_boxed_functions_int();

          auto itr = find_keyed_value(funs, t_name, t_hint);

          if (itr != funs.end())
          {
            return std::make_pair(std::distance(funs.begin(), itr), itr->second);
          } else {
            throw std::range_error("Object not found: " + t_name);
          }
        }


        /// Return true if a function exists
        bool function_exists(const std::string &name) const
        {
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          const auto &functions = get_functions_int();
          return find_keyed_value(functions, name) != functions.end();
        }

        /// \returns All values in the local thread state in the parent scope, or if it doesn't exist,
        ///          the current scope.
        std::map<std::string, Boxed_Value> get_parent_locals() const
        {
          auto &stack = get_stack_data();
          if (stack.size() > 1)
          {
            return std::map<std::string, Boxed_Value>(stack[1].begin(), stack[1].end());
          } else {
            return std::map<std::string, Boxed_Value>(stack[0].begin(), stack[0].end());
          }
        }

        /// \returns All values in the local thread state, added through the add() function
        std::map<std::string, Boxed_Value> get_locals() const
        {
          auto &stack = get_stack_data();
          auto &scope = stack.front();
          return std::map<std::string, Boxed_Value>(scope.begin(), scope.end());
        }

        /// \brief Sets all of the locals for the current thread state.
        ///
        /// \param[in] t_locals The map<name, value> set of variables to replace the current state with
        ///
        /// Any existing locals are removed and the given set of variables is added
        void set_locals(const std::map<std::string, Boxed_Value> &t_locals)
        {
          auto &stack = get_stack_data();
          auto &scope = stack.front();
          scope.assign(t_locals.begin(), t_locals.end());
        }



        ///
        /// Get a map of all objects that can be seen from the current scope in a scripting context
        ///
        std::map<std::string, Boxed_Value> get_scripting_objects() const
        {
          const Stack_Holder &s = *m_stack_holder;

          // We don't want the current context, but one up if it exists
          const StackData &stack = (s.stacks.size()==1)?(s.stacks.back()):(s.stacks[s.stacks.size()-2]);

          std::map<std::string, Boxed_Value> retval;

          // note: map insert doesn't overwrite existing values, which is why this works
          for (auto itr = stack.rbegin(); itr != stack.rend(); ++itr)
          {
            retval.insert(itr->begin(), itr->end());
          }

          // add the global values
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);
          retval.insert(m_state.m_global_objects.begin(), m_state.m_global_objects.end());

          return retval;
        }


        ///
        /// Get a map of all functions that can be seen from a scripting context
        ///
        std::map<std::string, Boxed_Value> get_function_objects() const
        {
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          const auto &funs = get_function_objects_int();

          std::map<std::string, Boxed_Value> objs;

          for (const auto & fun : funs)
          {
            objs.insert(std::make_pair(fun.first, const_var(fun.second)));
          }

          return objs;
        }


        /// Get a vector of all registered functions
        std::vector<std::pair<std::string, Proxy_Function > > get_functions() const
        {
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          std::vector<std::pair<std::string, Proxy_Function> > rets;

          const auto &functions = get_functions_int();

          for (const auto & function : functions)
          {
            for (const auto & internal_func : *function.second)
            {
              rets.emplace_back(function.first, internal_func);
            }
          }

          return rets;
        }


        const Type_Conversions &conversions() const
        {
          return m_conversions;
        }

        static bool is_attribute_call(const std::vector<Proxy_Function> &t_funs, const std::vector<Boxed_Value> &t_params,
            bool t_has_params, const Type_Conversions_State &t_conversions)
        {
          if (!t_has_params || t_params.empty()) {
            return false;
          }

          return std::any_of(std::begin(t_funs), std::end(t_funs),
              [&](const auto &fun) {
                return fun->is_attribute_function() && fun->compare_first_type(t_params[0], t_conversions);
              }
            );

        }

#ifdef CHAISCRIPT_MSVC
// MSVC is unable to recognize that "rethrow_exception" causes the function to return
// so we must disable it here.
#pragma warning(push)
#pragma warning(disable : 4715)
#endif
        Boxed_Value call_member(const std::string &t_name, std::atomic_uint_fast32_t &t_loc, const std::vector<Boxed_Value> &params, bool t_has_params,
                                const Type_Conversions_State &t_conversions)
        {
          uint_fast32_t loc = t_loc;
          const auto funs = get_function(t_name, loc);
          if (funs.first != loc) { t_loc = uint_fast32_t(funs.first); }

          const auto do_attribute_call =
            [this](int l_num_params, const std::vector<Boxed_Value> &l_params, const std::vector<Proxy_Function> &l_funs, const Type_Conversions_State &l_conversions)->Boxed_Value
            {
              std::vector<Boxed_Value> attr_params{l_params.begin(), l_params.begin() + l_num_params};
              Boxed_Value bv = dispatch::dispatch(l_funs, attr_params, l_conversions);
              if (l_num_params < int(l_params.size()) || bv.get_type_info().bare_equal(user_type<dispatch::Proxy_Function_Base>())) {
                struct This_Foist {
                  This_Foist(Dispatch_Engine &e, const Boxed_Value &t_bv) : m_e(e) {
                    m_e.get().new_scope();
                    m_e.get().add_object("__this", t_bv);
                  }

                  ~This_Foist() {
                    m_e.get().pop_scope();
                  }

                  std::reference_wrapper<Dispatch_Engine> m_e;
                };

                This_Foist fi(*this, l_params.front());

                try {
                  auto func = boxed_cast<const dispatch::Proxy_Function_Base *>(bv);
                  try {
                    return (*func)({l_params.begin() + l_num_params, l_params.end()}, l_conversions);
                  } catch (const chaiscript::exception::bad_boxed_cast &) {
                  } catch (const chaiscript::exception::arity_error &) {
                  } catch (const chaiscript::exception::guard_error &) {
                  }
                  throw chaiscript::exception::dispatch_error({l_params.begin() + l_num_params, l_params.end()},
                      std::vector<Const_Proxy_Function>{boxed_cast<Const_Proxy_Function>(bv)});
                } catch (const chaiscript::exception::bad_boxed_cast &) {
                  // unable to convert bv into a Proxy_Function_Base
                  throw chaiscript::exception::dispatch_error({l_params.begin() + l_num_params, l_params.end()},
                      std::vector<Const_Proxy_Function>(l_funs.begin(), l_funs.end()));
                }
              } else {
                return bv;
              }
            };

          if (is_attribute_call(*funs.second, params, t_has_params, t_conversions)) {
            return do_attribute_call(1, params, *funs.second, t_conversions);
          } else {
            std::exception_ptr except;

            if (!funs.second->empty()) {
              try {
                return dispatch::dispatch(*funs.second, params, t_conversions);
              } catch(chaiscript::exception::dispatch_error&) {
                except = std::current_exception();
              }
            }

            // If we get here we know that either there was no method with that name,
            // or there was no matching method

            const auto functions = [&]()->std::vector<Proxy_Function> {
              std::vector<Proxy_Function> fs;

              const auto method_missing_funs = get_method_missing_functions();

              for (const auto &f : *method_missing_funs)
              {
                if(f->compare_first_type(params[0], t_conversions)) {
                  fs.push_back(f);
                }
              }

              return fs;
            }();



            const bool is_no_param = [&]()->bool{
              for (const auto &f : functions) {
                if (f->get_arity() != 2) {
                  return false;
                }
              }
              return true;
            }();

            if (!functions.empty()) {
              try {
                if (is_no_param) {
                  std::vector<Boxed_Value> tmp_params(params);
                  tmp_params.insert(tmp_params.begin() + 1, var(t_name));
                  return do_attribute_call(2, tmp_params, functions, t_conversions);
                } else {
                  return dispatch::dispatch(functions, {params[0], var(t_name), var(std::vector<Boxed_Value>(params.begin()+1, params.end()))}, t_conversions);
                }
              } catch (const dispatch::option_explicit_set &e) {
                throw chaiscript::exception::dispatch_error(params, std::vector<Const_Proxy_Function>(funs.second->begin(), funs.second->end()),
                    e.what());
              }
            }

            // If we get all the way down here we know there was no "method_missing"
            // method at all.
            if (except) {
              std::rethrow_exception(except);
            } else {
              throw chaiscript::exception::dispatch_error(params, std::vector<Const_Proxy_Function>(funs.second->begin(), funs.second->end()));
            }
          }
        }
#ifdef CHAISCRIPT_MSVC
#pragma warning(pop)
#endif



        Boxed_Value call_function(const std::string &t_name, std::atomic_uint_fast32_t &t_loc, const std::vector<Boxed_Value> &params,
            const Type_Conversions_State &t_conversions) const
        {
          uint_fast32_t loc = t_loc;
          const auto funs = get_function(t_name, loc);
          if (funs.first != loc) { t_loc = uint_fast32_t(funs.first);
}
          return dispatch::dispatch(*funs.second, params, t_conversions);
        }


        /// Dump object info to stdout
        void dump_object(const Boxed_Value &o) const
        {
          std::cout << (o.is_const()?"const ":"") << type_name(o) << '\n';
        }

        /// Dump type info to stdout
        void dump_type(const Type_Info &type) const
        {
          std::cout << (type.is_const()?"const ":"") << get_type_name(type);
        }

        /// Dump function to stdout
        void dump_function(const std::pair<const std::string, Proxy_Function > &f) const
        {
          std::vector<Type_Info> params = f.second->get_param_types();

          dump_type(params.front());
          std::cout << " " << f.first << "(";

          for (std::vector<Type_Info>::const_iterator itr = params.begin() + 1;
              itr != params.end();
              )
          {
            dump_type(*itr);
            ++itr;

            if (itr != params.end())
            {
              std::cout << ", ";
            }
          }

          std::cout << ") \n";
        }

        /// Returns true if a call can be made that consists of the first parameter
        /// (the function) with the remaining parameters as its arguments.
        Boxed_Value call_exists(const std::vector<Boxed_Value> &params) const
        {
          if (params.empty())
          {
            throw chaiscript::exception::arity_error(static_cast<int>(params.size()), 1);
          }

          const Const_Proxy_Function &f = this->boxed_cast<Const_Proxy_Function>(params[0]);
          const Type_Conversions_State convs(m_conversions, m_conversions.conversion_saves());

          return const_var(f->call_match(std::vector<Boxed_Value>(params.begin() + 1, params.end()), convs));
        }

        /// Dump all system info to stdout
        void dump_system() const
        {
          std::cout << "Registered Types: \n";
          for (auto const &type: get_types())
          {
            std::cout << type.first << ": " << type.second.bare_name() << '\n';
          }

          std::cout << '\n';

          std::cout << "Functions: \n";
          for (auto const &func: get_functions())
          {
            dump_function(func);
          }
          std::cout << '\n';
        }

        /// return true if the Boxed_Value matches the registered type by name
        bool is_type(const Boxed_Value &r, const std::string &user_typename) const
        {
          try {
            if (get_type(user_typename).bare_equal(r.get_type_info()))
            {
              return true;
            }
          } catch (const std::range_error &) {
          }

          try {
            const dispatch::Dynamic_Object &d = boxed_cast<const dispatch::Dynamic_Object &>(r);
            return d.get_type_name() == user_typename;
          } catch (const std::bad_cast &) {
          }

          return false;
        }

        std::string type_name(const Boxed_Value &obj) const
        {
          return get_type_name(obj.get_type_info());
        }

        State get_state() const
        {
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          return m_state;
        }

        void set_state(const State &t_state)
        {
          chaiscript::detail::threading::unique_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          m_state = t_state;
        }

        static void save_function_params(Stack_Holder &t_s, std::initializer_list<Boxed_Value> t_params)
        {
          t_s.call_params.back().insert(t_s.call_params.back().begin(), t_params);
        }

        static void save_function_params(Stack_Holder &t_s, std::vector<Boxed_Value> &&t_params)
        {
          for (auto &&param : t_params)
          {
            t_s.call_params.back().insert(t_s.call_params.back().begin(), std::move(param));
          }
        }

        static void save_function_params(Stack_Holder &t_s, const std::vector<Boxed_Value> &t_params)
        {
          t_s.call_params.back().insert(t_s.call_params.back().begin(), t_params.begin(), t_params.end());
        }

        void save_function_params(std::initializer_list<Boxed_Value> t_params)
        {
          save_function_params(*m_stack_holder, t_params);
        }

        void save_function_params(std::vector<Boxed_Value> &&t_params)
        {
          save_function_params(*m_stack_holder, std::move(t_params));
        }

        void save_function_params(const std::vector<Boxed_Value> &t_params)
        {
          save_function_params(*m_stack_holder, t_params);
        }

        void new_function_call(Stack_Holder &t_s, Type_Conversions::Conversion_Saves &t_saves)
        {
          if (t_s.call_depth == 0)
          {
            m_conversions.enable_conversion_saves(t_saves, true);
          }

          ++t_s.call_depth;

          save_function_params(m_conversions.take_saves(t_saves));
        }

        void pop_function_call(Stack_Holder &t_s, Type_Conversions::Conversion_Saves &t_saves)
        {
          --t_s.call_depth;

          assert(t_s.call_depth >= 0);

          if (t_s.call_depth == 0)
          {
            t_s.call_params.back().clear();
            m_conversions.enable_conversion_saves(t_saves, false);
          }
        }

        void new_function_call()
        {
          new_function_call(*m_stack_holder, m_conversions.conversion_saves());
        }

        void pop_function_call()
        {
          pop_function_call(*m_stack_holder, m_conversions.conversion_saves());
        }

        Stack_Holder &get_stack_holder()
        {
          return *m_stack_holder;
        }

        /// Returns the current stack
        /// make const/non const versions
        const StackData &get_stack_data() const
        {
          return m_stack_holder->stacks.back();
        }

        static StackData &get_stack_data(Stack_Holder &t_holder)
        {
          return t_holder.stacks.back();
        }

        StackData &get_stack_data()
        {
          return m_stack_holder->stacks.back();
        }

        parser::ChaiScript_Parser_Base &get_parser()
        {
          return m_parser.get();
        }

      private:

        const std::vector<std::pair<std::string, Boxed_Value>> &get_boxed_functions_int() const
        {
          return m_state.m_boxed_functions;
        }

        std::vector<std::pair<std::string, Boxed_Value>> &get_boxed_functions_int()
        {
          return m_state.m_boxed_functions;
        }

        const std::vector<std::pair<std::string, Proxy_Function>> &get_function_objects_int() const
        {
          return m_state.m_function_objects;
        }

        std::vector<std::pair<std::string, Proxy_Function>> &get_function_objects_int()
        {
          return m_state.m_function_objects;
        }

        const std::vector<std::pair<std::string, std::shared_ptr<std::vector<Proxy_Function>>>> &get_functions_int() const
        {
          return m_state.m_functions;
        }

        std::vector<std::pair<std::string, std::shared_ptr<std::vector<Proxy_Function>>>> &get_functions_int()
        {
          return m_state.m_functions;
        }

        static bool function_less_than(const Proxy_Function &lhs, const Proxy_Function &rhs)
        {

          auto dynamic_lhs(std::dynamic_pointer_cast<const dispatch::Dynamic_Proxy_Function>(lhs));
          auto dynamic_rhs(std::dynamic_pointer_cast<const dispatch::Dynamic_Proxy_Function>(rhs));

          if (dynamic_lhs && dynamic_rhs)
          {
            if (dynamic_lhs->get_guard())
            {
              return dynamic_rhs->get_guard() ? false : true;
            } else {
              return false;
            }
          }

          if (dynamic_lhs && !dynamic_rhs)
          {
            return false;
          }

          if (!dynamic_lhs && dynamic_rhs)
          {
            return true;
          }

          const auto &lhsparamtypes = lhs->get_param_types();
          const auto &rhsparamtypes = rhs->get_param_types();

          const auto lhssize = lhsparamtypes.size();
          const auto rhssize = rhsparamtypes.size();

          static const auto boxed_type = user_type<Boxed_Value>();
          static const auto boxed_pod_type = user_type<Boxed_Number>();

          for (size_t i = 1; i < lhssize && i < rhssize; ++i)
          {
            const Type_Info &lt = lhsparamtypes[i];
            const Type_Info &rt = rhsparamtypes[i];

            if (lt.bare_equal(rt) && lt.is_const() == rt.is_const())
            {
              continue; // The first two types are essentially the same, next iteration
            }

            // const is after non-const for the same type
            if (lt.bare_equal(rt) && lt.is_const() && !rt.is_const())
            {
              return false;
            }

            if (lt.bare_equal(rt) && !lt.is_const())
            {
              return true;
            }

            // boxed_values are sorted last
            if (lt.bare_equal(boxed_type))
            {
              return false;
            }

            if (rt.bare_equal(boxed_type))
            {
              return true;
            }

            if (lt.bare_equal(boxed_pod_type))
            {
              return false;
            }

            if (rt.bare_equal(boxed_pod_type))
            {
              return true;
            }

            // otherwise, we want to sort by typeid
            return lt < rt;
          }

          return false;
        }



        template<typename Container, typename Key, typename Value>
          static void add_keyed_value(Container &t_c, const Key &t_key, Value &&t_value)
          {
            auto itr = find_keyed_value(t_c, t_key);

            if (itr == t_c.end()) {
              t_c.reserve(t_c.size() + 1); // tightly control growth of memory usage here
              t_c.emplace_back(t_key, std::forward<Value>(t_value));
            } else {
              typedef typename Container::value_type value_type;
              *itr = value_type(t_key, std::forward<Value>(t_value));
            }
          }

        template<typename Container, typename Key>
        static typename Container::iterator find_keyed_value(Container &t_c, const Key &t_key)
          {
            return std::find_if(t_c.begin(), t_c.end(),
                [&t_key](const typename Container::value_type &o) {
                  return o.first == t_key;
                });
          }

        template<typename Container, typename Key>
        static typename Container::const_iterator find_keyed_value(const Container &t_c, const Key &t_key)
          {
            return std::find_if(t_c.begin(), t_c.end(), 
                [&t_key](const typename Container::value_type &o) {
                  return o.first == t_key;
                });
          }

        template<typename Container, typename Key>
        static typename Container::const_iterator find_keyed_value(const Container &t_c, const Key &t_key, const size_t t_hint)
          {
            if (t_c.size() > t_hint && t_c[t_hint].first == t_key) {
              return std::next(t_c.begin(), static_cast<typename std::iterator_traits<typename Container::const_iterator>::difference_type>(t_hint));
            } else {
              return find_keyed_value(t_c, t_key);
            }
          }


        /// Implementation detail for adding a function. 
        /// \throws exception::name_conflict_error if there's a function matching the given one being added
        void add_function(const Proxy_Function &t_f, const std::string &t_name)
        {
          chaiscript::detail::threading::unique_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          auto &funcs = get_functions_int();

          auto itr = find_keyed_value(funcs, t_name);

          Proxy_Function new_func =
            [&]() -> Proxy_Function {
              if (itr != funcs.end())
              {
                auto vec = *itr->second;
                for (const auto &func : vec)
                {
                  if ((*t_f) == *(func))
                  {
                    throw chaiscript::exception::name_conflict_error(t_name);
                  }
                }

                vec.reserve(vec.size() + 1); // tightly control vec growth
                vec.push_back(t_f);
                std::stable_sort(vec.begin(), vec.end(), &function_less_than);
                itr->second = std::make_shared<std::vector<Proxy_Function>>(vec);
                return std::make_shared<Dispatch_Function>(std::move(vec));
              } else if (t_f->has_arithmetic_param()) {
                // if the function is the only function but it also contains
                // arithmetic operators, we must wrap it in a dispatch function
                // to allow for automatic arithmetic type conversions
                std::vector<Proxy_Function> vec({t_f});
                funcs.emplace_back(t_name, std::make_shared<std::vector<Proxy_Function>>(vec));
                return std::make_shared<Dispatch_Function>(std::move(vec));
              } else {
                funcs.emplace_back(t_name, std::make_shared<std::vector<Proxy_Function>>(std::initializer_list<Proxy_Function>({t_f})));
                return t_f;
              }
            }();

          add_keyed_value(get_boxed_functions_int(), t_name, const_var(new_func));
          add_keyed_value(get_function_objects_int(), t_name, std::move(new_func));
        }

        mutable chaiscript::detail::threading::shared_mutex m_mutex;


        Type_Conversions m_conversions;
        chaiscript::detail::threading::Thread_Storage<Stack_Holder> m_stack_holder;
        std::reference_wrapper<parser::ChaiScript_Parser_Base> m_parser;

        mutable std::atomic_uint_fast32_t m_method_missing_loc = {0};

        State m_state;
    };

    class Dispatch_State
    {
      public:
        explicit Dispatch_State(Dispatch_Engine &t_engine)
          : m_engine(t_engine),
            m_stack_holder(t_engine.get_stack_holder()),
            m_conversions(t_engine.conversions(), t_engine.conversions().conversion_saves())
        {
        }

        Dispatch_Engine *operator->() const {
          return &m_engine.get();
        }

        Dispatch_Engine &operator*() const {
          return m_engine.get();
        }

        Stack_Holder &stack_holder() const {
          return m_stack_holder.get();
        }

        const Type_Conversions_State &conversions() const {
          return m_conversions;
        }

        Type_Conversions::Conversion_Saves &conversion_saves() const {
          return m_conversions.saves();
        }

        Boxed_Value &add_get_object(const std::string &t_name, Boxed_Value obj) const {
          return m_engine.get().add_get_object(t_name, std::move(obj), m_stack_holder.get());
        }

        void add_object(const std::string &t_name, Boxed_Value obj) const {
          return m_engine.get().add_object(t_name, std::move(obj), m_stack_holder.get());
        }

        Boxed_Value get_object(const std::string &t_name, std::atomic_uint_fast32_t &t_loc) const {
          return m_engine.get().get_object(t_name, t_loc, m_stack_holder.get());
        }

      private:
        std::reference_wrapper<Dispatch_Engine> m_engine;
        std::reference_wrapper<Stack_Holder> m_stack_holder;
        Type_Conversions_State m_conversions;
    };
  }
}

#endif


