// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com


#ifndef CHAISCRIPT_ALGEBRAIC_HPP_
#define CHAISCRIPT_ALGEBRAIC_HPP_

#include "../utility/fnv1a.hpp"

#include <string>

namespace chaiscript
{

  struct Operators {
    enum class Opers
    {
      boolean_flag,
      equals, less_than, greater_than, less_than_equal, greater_than_equal, not_equal, 
      non_const_flag, 
      assign, pre_increment, pre_decrement, assign_product, assign_sum,
      assign_quotient, assign_difference,
      non_const_int_flag,
      assign_bitwise_and, assign_bitwise_or, assign_shift_left, assign_shift_right,
      assign_remainder, assign_bitwise_xor,
      const_int_flag,
      shift_left, shift_right, remainder, bitwise_and, bitwise_or, bitwise_xor, bitwise_complement,
      const_flag,
      sum, quotient, product, difference, unary_plus, unary_minus, 
      invalid
    };

    static const char *to_string(Opers t_oper) {
      static const char *opers[] = { 
        "",
        "==", "<", ">", "<=", ">=", "!=",
        "",
        "=", "++", "--", "*=", "+=",
        "/=", "-=",
        "",
        "&=", "|=", "<<=", ">>=",
        "%=", "^=",
        "", 
        "<<", ">>", "%", "&", "|", "^", "~",
        "",
        "+", "/", "*", "-", "+", "-",
        ""
      };
      return opers[static_cast<int>(t_oper)];
    }

    static Opers to_operator(const std::string &t_str, bool t_is_unary = false)
    {
#ifdef CHAISCRIPT_MSVC
#pragma warning(push)
#pragma warning(disable : 4307)
#endif

      const auto op_hash = utility::fnv1a_32(t_str.c_str());
      switch (op_hash) {
        case utility::fnv1a_32("=="): { return Opers::equals; }
        case utility::fnv1a_32("<"): { return Opers::less_than; }
        case utility::fnv1a_32(">"): { return Opers::greater_than; }
        case utility::fnv1a_32("<="): { return Opers::less_than_equal; }
        case utility::fnv1a_32(">="): { return Opers::greater_than_equal; }
        case utility::fnv1a_32("!="): { return Opers::not_equal; }
        case utility::fnv1a_32("="): { return Opers::assign; }
        case utility::fnv1a_32("++"): { return Opers::pre_increment; }
        case utility::fnv1a_32("--"): { return Opers::pre_decrement; }
        case utility::fnv1a_32("*="): { return Opers::assign_product; }
        case utility::fnv1a_32("+="): { return Opers::assign_sum; }
        case utility::fnv1a_32("-="): { return Opers::assign_difference; }
        case utility::fnv1a_32("&="): { return Opers::assign_bitwise_and; }
        case utility::fnv1a_32("|="): { return Opers::assign_bitwise_or; }
        case utility::fnv1a_32("<<="): { return Opers::assign_shift_left; }
        case utility::fnv1a_32(">>="): { return Opers::assign_shift_right; }
        case utility::fnv1a_32("%="): { return Opers::assign_remainder; }
        case utility::fnv1a_32("^="): { return Opers::assign_bitwise_xor; }
        case utility::fnv1a_32("<<"): { return Opers::shift_left; }
        case utility::fnv1a_32(">>"): { return Opers::shift_right; }
        case utility::fnv1a_32("%"): { return Opers::remainder; }
        case utility::fnv1a_32("&"): { return Opers::bitwise_and; }
        case utility::fnv1a_32("|"): { return Opers::bitwise_or; }
        case utility::fnv1a_32("^"): { return Opers::bitwise_xor; }
        case utility::fnv1a_32("~"): { return Opers::bitwise_complement; }
        case utility::fnv1a_32("+"): { return t_is_unary ? Opers::unary_plus : Opers::sum; }
        case utility::fnv1a_32("-"): { return t_is_unary ? Opers::unary_minus : Opers::difference; }
        case utility::fnv1a_32("/"): { return Opers::quotient; }
        case utility::fnv1a_32("*"): { return Opers::product; }
        default: { return Opers::invalid; }
      }
#ifdef CHAISCRIPT_MSVC
#pragma warning(pop)
#endif

    }

  };
}

#endif /* _CHAISCRIPT_ALGEBRAIC_HPP */

