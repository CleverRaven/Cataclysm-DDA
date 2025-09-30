// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_TRACER_HPP_
#define CHAISCRIPT_TRACER_HPP_

namespace chaiscript {
  namespace eval {


    struct Noop_Tracer_Detail
    {
      template<typename T>
        void trace(const chaiscript::detail::Dispatch_State &, const AST_Node_Impl<T> *)
        {
        }
    };

    template<typename ... T>
      struct Tracer : T...
    {
      Tracer() = default;
      explicit Tracer(T ... t)
        : T(std::move(t))...
      {
      }

      void do_trace(const chaiscript::detail::Dispatch_State &ds, const AST_Node_Impl<Tracer<T...>> *node) {
        (void)std::initializer_list<int>{ (static_cast<T&>(*this).trace(ds, node), 0)... };
      }

      static void trace(const chaiscript::detail::Dispatch_State &ds, const AST_Node_Impl<Tracer<T...>> *node) {
        ds->get_parser().get_tracer<Tracer<T...>>().do_trace(ds, node);
      }
    };

    typedef Tracer<Noop_Tracer_Detail> Noop_Tracer;

  }
}

#endif

