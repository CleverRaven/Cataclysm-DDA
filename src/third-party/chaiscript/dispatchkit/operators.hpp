// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com


#ifndef CHAISCRIPT_OPERATORS_HPP_
#define CHAISCRIPT_OPERATORS_HPP_

#include "../chaiscript_defines.hpp"
#include "register_function.hpp"

namespace chaiscript 
{
  namespace bootstrap
  {
    namespace operators
    {
      template<typename T>
        void assign(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs, const T&rhs)->T&{return lhs = rhs;}), "=");
        }

      template<typename T>
        void assign_bitwise_and(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs, const T&rhs)->T&{return lhs &= rhs;}), "&=");
        }

      template<typename T>
        void assign_xor(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs, const T&rhs)->T&{return lhs ^= rhs;}), "^=");
        }

      template<typename T>
        void assign_bitwise_or(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs, const T&rhs)->T&{return lhs |= rhs;}), "|=");
        }

      template<typename T>
        void assign_difference(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs, const T&rhs)->T&{return lhs -= rhs;}), "-=");
        }

      template<typename T>
        void assign_left_shift(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs, const T&rhs)->T&{return lhs <<= rhs;}), "<<=");
        }

      template<typename T>
        void assign_product(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs, const T&rhs)->T&{return lhs <<= rhs;}), "*=");
        }

      template<typename T>
        void assign_quotient(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs, const T&rhs)->T&{return lhs /= rhs;}), "/=");
        }

      template<typename T>
        void assign_remainder(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs, const T&rhs)->T&{return lhs %= rhs;}), "%=");
        }

      template<typename T>
        void assign_right_shift(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs, const T&rhs)->T&{return lhs >>= rhs;}), ">>=");
        }

      template<typename T>
        void assign_sum(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs, const T&rhs)->T&{return lhs += rhs;}), "+=");
        }

      template<typename T>
        void prefix_decrement(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs)->T&{return --lhs;}), "--");
        }

      template<typename T>
        void prefix_increment(Module& m)
        {
          m.add(chaiscript::fun([](T &lhs)->T&{return ++lhs;}), "++");
        }

      template<typename T>
        void equal(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs==rhs;}), "==");
        }

      template<typename T>
        void greater_than(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs>rhs;}), ">");
        }

      template<typename T>
        void greater_than_equal(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs>=rhs;}), ">=");
        }

      template<typename T>
        void less_than(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs<rhs;}), "<");
        }

      template<typename T>
        void less_than_equal(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs<=rhs;}), "<=");
        }

      template<typename T>
        void logical_compliment(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs){return !lhs;}), "!");
        }

      template<typename T>
        void not_equal(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs!=rhs;}), "!=");
        }

      template<typename T>
        void addition(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs+rhs;}), "+");
        }

      template<typename T>
        void unary_plus(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs){return +lhs;}), "+");
        }

      template<typename T>
        void subtraction(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs-rhs;}), "-");
        }

      template<typename T>
        void unary_minus(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs){return -lhs;}), "-");
        }

      template<typename T>
        void bitwise_and(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs&rhs;}), "&");
        }

      template<typename T>
        void bitwise_compliment(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs){return ~lhs;}), "~");
        }

      template<typename T>
        void bitwise_xor(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs^rhs;}), "^");
        }

      template<typename T>
        void bitwise_or(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs|rhs;}), "|");
        }

      template<typename T>
        void division(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs/rhs;}), "/");
        }

      template<typename T>
        void left_shift(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs<<rhs;}), "<<");
        }

      template<typename T>
        void multiplication(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs*rhs;}), "*");
        }

      template<typename T>
        void remainder(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs%rhs;}), "%");
        }

      template<typename T>
        void right_shift(Module& m)
        {
          m.add(chaiscript::fun([](const T &lhs, const T &rhs){return lhs>>rhs;}), ">>");
        }
    }
  }
}

#endif
