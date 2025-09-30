// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_BASIC_HPP_
#define CHAISCRIPT_BASIC_HPP_

#include "chaiscript_defines.hpp"

#include "dispatchkit/dispatchkit.hpp"
#include "dispatchkit/function_call.hpp"
#include "dispatchkit/dynamic_object.hpp"
#include "dispatchkit/boxed_number.hpp"

#include "language/chaiscript_eval.hpp"
#include "language/chaiscript_engine.hpp"

// This file includes all of the basic requirements for ChaiScript,
// to use, you might do something like:
//

/*

#include "chaiscript_stdlib.hpp"
#include "language/chaiscript_parser.hpp"

ChaiScript_Basic chai(
          chaiscript::Std_Lib::library(),
          std::make_unique<parser::ChaiScript_Parser<eval::Noop_Tracer, optimizer::Optimizer_Default>>());

*/

// If you want a fully packaged ready to go ChaiScript, use chaiscript.hpp



#endif /* CHAISCRIPT_BASIC_HPP_ */
