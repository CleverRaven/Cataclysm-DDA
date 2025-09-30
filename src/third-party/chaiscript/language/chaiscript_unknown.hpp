// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_UNKNOWN_HPP_
#define CHAISCRIPT_UNKNOWN_HPP_


namespace chaiscript
{
  namespace detail
  {
    struct Loadable_Module
    {
      Loadable_Module(const std::string &, const std::string &)
      {
#ifdef CHAISCRIPT_NO_DYNLOAD
        throw chaiscript::exception::load_module_error("Loadable module support was disabled (CHAISCRIPT_NO_DYNLOAD)");
#else
        throw chaiscript::exception::load_module_error("Loadable module support not available for your platform");
#endif
      }

      ModulePtr m_moduleptr;
    };
  }
}
#endif

