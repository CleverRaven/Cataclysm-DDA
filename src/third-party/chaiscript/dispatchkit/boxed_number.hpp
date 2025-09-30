// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#ifndef CHAISCRIPT_BOXED_NUMERIC_HPP_
#define CHAISCRIPT_BOXED_NUMERIC_HPP_

#include <cstdint>
#include <sstream>
#include <string>

#include "../language/chaiscript_algebraic.hpp"
#include "any.hpp"
#include "boxed_cast.hpp"
#include "boxed_cast_helper.hpp"
#include "boxed_value.hpp"
#include "type_info.hpp"

namespace chaiscript {
class Type_Conversions;
}  // namespace chaiscript

namespace chaiscript
{
  namespace exception
  {
    struct arithmetic_error : std::runtime_error
    {
      explicit arithmetic_error(const std::string& reason) : std::runtime_error("Arithmetic error: " + reason) {}
      arithmetic_error(const arithmetic_error &) = default;
      ~arithmetic_error() noexcept override = default;
    };
  }
}

namespace chaiscript 
{

// Due to the nature of generating every possible arithmetic operation, there
// are going to be warnings generated on every platform regarding size and sign,
// this is OK, so we're disabling size/and sign type warnings
#ifdef CHAISCRIPT_MSVC
#pragma warning(push)
#pragma warning(disable : 4244 4018 4389 4146 4365 4267 4242)
#endif


#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

  /// \brief Represents any numeric type, generically. Used internally for generic operations between POD values
  class Boxed_Number
  {
    private:
      enum class Common_Types {
        t_int32,
        t_double,
        t_uint8,
        t_int8,
        t_uint16,
        t_int16,
        t_uint32,
        t_uint64,
        t_int64,
        t_float,
        t_long_double
      };

      template<typename T>
      static inline void check_divide_by_zero(T t, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr)
      {
#ifndef CHAISCRIPT_NO_PROTECT_DIVIDEBYZERO
        if (t == 0) {
          throw chaiscript::exception::arithmetic_error("divide by zero");
        }
#endif
      }

      template<typename T>
      static inline void check_divide_by_zero(T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr)
      {
      }

      static constexpr Common_Types get_common_type(size_t t_size, bool t_signed)
      {
        return   (t_size == 1 && t_signed)?(Common_Types::t_int8)
                :(t_size == 1)?(Common_Types::t_uint8)
                :(t_size == 2 && t_signed)?(Common_Types::t_int16)
                :(t_size == 2)?(Common_Types::t_uint16)
                :(t_size == 4 && t_signed)?(Common_Types::t_int32)
                :(t_size == 4)?(Common_Types::t_uint32)
                :(t_size == 8 && t_signed)?(Common_Types::t_int64)
                :(Common_Types::t_uint64);
      }


      static Common_Types get_common_type(const Boxed_Value &t_bv)
      {
        const Type_Info &inp_ = t_bv.get_type_info();

        if (inp_ == typeid(int)) {
          return get_common_type(sizeof(int), true);
        } else if (inp_ == typeid(double)) {
          return Common_Types::t_double;
        } else if (inp_ == typeid(long double)) {
          return Common_Types::t_long_double;
        } else if (inp_ == typeid(float)) {
          return Common_Types::t_float;
        } else if (inp_ == typeid(char)) {
          return get_common_type(sizeof(char), std::is_signed<char>::value);
        } else if (inp_ == typeid(unsigned char)) {
          return get_common_type(sizeof(unsigned char), false);
        } else if (inp_ == typeid(unsigned int)) {
          return get_common_type(sizeof(unsigned int), false);
        } else if (inp_ == typeid(long)) {
          return get_common_type(sizeof(long), true);
        } else if (inp_ == typeid(long long)) {
          return get_common_type(sizeof(long long), true);
        } else if (inp_ == typeid(unsigned long)) {
          return get_common_type(sizeof(unsigned long), false);
        } else if (inp_ == typeid(unsigned long long)) {
          return get_common_type(sizeof(unsigned long long), false);
        } else if (inp_ == typeid(std::int8_t)) {
          return Common_Types::t_int8;
        } else if (inp_ == typeid(std::int16_t)) {
          return Common_Types::t_int16;
        } else if (inp_ == typeid(std::int32_t)) {
          return Common_Types::t_int32;
        } else if (inp_ == typeid(std::int64_t)) {
          return Common_Types::t_int64;
        } else if (inp_ == typeid(std::uint8_t)) {
          return Common_Types::t_uint8;
        } else if (inp_ == typeid(std::uint16_t)) {
          return Common_Types::t_uint16;
        } else if (inp_ == typeid(std::uint32_t)) {
          return Common_Types::t_uint32;
        } else if (inp_ == typeid(std::uint64_t)) {
          return Common_Types::t_uint64;
        } else if (inp_ == typeid(wchar_t)) {
          return get_common_type(sizeof(wchar_t), std::is_signed<wchar_t>::value);
        } else if (inp_ == typeid(char16_t)) {
          return get_common_type(sizeof(char16_t), std::is_signed<char16_t>::value);
        } else if (inp_ == typeid(char32_t)) {
          return get_common_type(sizeof(char32_t), std::is_signed<char32_t>::value);
        } else  {
          throw chaiscript::detail::exception::bad_any_cast();
        }
      }

      template<typename T>
      static Boxed_Value boolean_go(Operators::Opers t_oper, const T &t, const T &u)
      {
        switch (t_oper)
        {
          case Operators::Opers::equals:
            return const_var(t == u);
          case Operators::Opers::less_than:
            return const_var(t < u);
          case Operators::Opers::greater_than:
            return const_var(t > u);
          case Operators::Opers::less_than_equal:
            return const_var(t <= u);
          case Operators::Opers::greater_than_equal:
            return const_var(t >= u);
          case Operators::Opers::not_equal:
            return const_var(t != u);
          default:
            throw chaiscript::detail::exception::bad_any_cast();
        }
      }

      template<typename T>
      static Boxed_Value unary_go(Operators::Opers t_oper, T &t, const Boxed_Value &t_lhs) 
      {
        switch (t_oper)
        {
          case Operators::Opers::pre_increment:
            ++t;
            break;
          case Operators::Opers::pre_decrement:
            --t;
            break;
          default:
            throw chaiscript::detail::exception::bad_any_cast();
        }

        return t_lhs;
      }

      template<typename T, typename U>
      static Boxed_Value binary_go(Operators::Opers t_oper, T &t, const U &u, const Boxed_Value &t_lhs) 
      {
        switch (t_oper)
        {
          case Operators::Opers::assign:
            t = u;
            break;
          case Operators::Opers::assign_product:
            t *= u;
            break;
          case Operators::Opers::assign_sum:
            t += u;
            break;
          case Operators::Opers::assign_quotient:
            check_divide_by_zero(u);
            t /= u;
            break;
          case Operators::Opers::assign_difference:
            t -= u;
            break;
          default:
            throw chaiscript::detail::exception::bad_any_cast();
        }

        return t_lhs;
      }

      template<typename T, typename U>
      static Boxed_Value binary_int_go(Operators::Opers t_oper, T &t, const U &u, const Boxed_Value &t_lhs) 
      {
        switch (t_oper)
        {
          case Operators::Opers::assign_bitwise_and:
            t &= u;
            break;
          case Operators::Opers::assign_bitwise_or:
            t |= u;
            break;
          case Operators::Opers::assign_shift_left:
            t <<= u;
            break;
          case Operators::Opers::assign_shift_right:
            t >>= u;
            break;
          case Operators::Opers::assign_remainder:
            check_divide_by_zero(u);
            t %= u;
            break;
          case Operators::Opers::assign_bitwise_xor:
            t ^= u;
            break;
          default:
            throw chaiscript::detail::exception::bad_any_cast();
        }
        return t_lhs;
      }

      template<typename T>
      static Boxed_Value const_unary_int_go(Operators::Opers t_oper, const T &t) 
      {
        switch (t_oper)
        {
          case Operators::Opers::bitwise_complement:
            return const_var(~t);
          default:
            throw chaiscript::detail::exception::bad_any_cast();
        }
      }

      template<typename T>
      static Boxed_Value const_binary_int_go(Operators::Opers t_oper, const T &t, const T &u) 
      {
        switch (t_oper)
        {
          case Operators::Opers::shift_left:
            return const_var(t << u);
          case Operators::Opers::shift_right:
            return const_var(t >> u);
          case Operators::Opers::remainder:
            check_divide_by_zero(u);
            return const_var(t % u);
          case Operators::Opers::bitwise_and:
            return const_var(t & u);
          case Operators::Opers::bitwise_or:
            return const_var(t | u);
          case Operators::Opers::bitwise_xor:
            return const_var(t ^ u);
          default:
            throw chaiscript::detail::exception::bad_any_cast();
        }
      }

      template<typename T>
      static Boxed_Value const_unary_go(Operators::Opers t_oper, const T &t)
      {
        switch (t_oper)
        {
          case Operators::Opers::unary_minus:
            return const_var(-t);
          case Operators::Opers::unary_plus:
            return const_var(+t);
          default:
            throw chaiscript::detail::exception::bad_any_cast();
        }
      }

      template<typename T>
      static Boxed_Value const_binary_go(Operators::Opers t_oper, const T &t, const T &u) 
      {
        switch (t_oper)
        {
          case Operators::Opers::sum:
            return const_var(t + u);
          case Operators::Opers::quotient:
            check_divide_by_zero(u);
            return const_var(t / u);
          case Operators::Opers::product:
            return const_var(t * u);
          case Operators::Opers::difference:
            return const_var(t - u);
          default:
            throw chaiscript::detail::exception::bad_any_cast();
        }
      }

      template<typename LHS, typename RHS>
      static auto go(Operators::Opers t_oper, const Boxed_Value &t_lhs, const Boxed_Value &t_rhs)
          -> typename std::enable_if<!std::is_floating_point<LHS>::value && !std::is_floating_point<RHS>::value, Boxed_Value>::type
      {
        typedef typename std::common_type<LHS, RHS>::type common_type;
        if (t_oper > Operators::Opers::boolean_flag && t_oper < Operators::Opers::non_const_flag)
        {
          return boolean_go(t_oper, get_as_aux<common_type, LHS>(t_lhs), get_as_aux<common_type, RHS>(t_rhs));
        } else if (t_oper > Operators::Opers::non_const_flag && t_oper < Operators::Opers::non_const_int_flag && !t_lhs.is_const() && !t_lhs.is_return_value()) {
          return binary_go(t_oper, *static_cast<LHS *>(t_lhs.get_ptr()), get_as_aux<common_type, RHS>(t_rhs), t_lhs);
        } else if (t_oper > Operators::Opers::non_const_int_flag && t_oper < Operators::Opers::const_int_flag && !t_lhs.is_const() && !t_lhs.is_return_value()) {
          return binary_int_go(t_oper, *static_cast<LHS *>(t_lhs.get_ptr()), get_as_aux<common_type, RHS>(t_rhs), t_lhs);
        } else if (t_oper > Operators::Opers::const_int_flag && t_oper < Operators::Opers::const_flag) {
          return const_binary_int_go(t_oper, get_as_aux<common_type, LHS>(t_lhs), get_as_aux<common_type, RHS>(t_rhs));
        } else if (t_oper > Operators::Opers::const_flag) {
          return const_binary_go(t_oper, get_as_aux<common_type, LHS>(t_lhs), get_as_aux<common_type, RHS>(t_rhs));
        } else {
          throw chaiscript::detail::exception::bad_any_cast();
        }
      }

      template<typename LHS, typename RHS>
      static auto go(Operators::Opers t_oper, const Boxed_Value &t_lhs, const Boxed_Value &t_rhs) 
          -> typename std::enable_if<std::is_floating_point<LHS>::value || std::is_floating_point<RHS>::value, Boxed_Value>::type
      {
        typedef typename std::common_type<LHS, RHS>::type common_type;
        if (t_oper > Operators::Opers::boolean_flag && t_oper < Operators::Opers::non_const_flag)
        {
          return boolean_go(t_oper, get_as_aux<common_type, LHS>(t_lhs), get_as_aux<common_type, RHS>(t_rhs));
        } else if (t_oper > Operators::Opers::non_const_flag && t_oper < Operators::Opers::non_const_int_flag && !t_lhs.is_const() && !t_lhs.is_return_value()) {
          return binary_go(t_oper, *static_cast<LHS *>(t_lhs.get_ptr()), get_as_aux<common_type, RHS>(t_rhs), t_lhs);
        } else if (t_oper > Operators::Opers::const_flag) {
          return const_binary_go(t_oper, get_as_aux<common_type, LHS>(t_lhs), get_as_aux<common_type, RHS>(t_rhs));
        } else {
          throw chaiscript::detail::exception::bad_any_cast();
        }
      }

      // Unary 
      template<typename LHS>
      static auto go(Operators::Opers t_oper, const Boxed_Value &t_lhs)
          -> typename std::enable_if<!std::is_floating_point<LHS>::value, Boxed_Value>::type
      {
        if (t_oper > Operators::Opers::non_const_flag && t_oper < Operators::Opers::non_const_int_flag && !t_lhs.is_const() && !t_lhs.is_return_value()) {
          return unary_go(t_oper, *static_cast<LHS *>(t_lhs.get_ptr()), t_lhs);
        } else if (t_oper > Operators::Opers::const_int_flag && t_oper < Operators::Opers::const_flag) {
          return const_unary_int_go(t_oper, *static_cast<const LHS *>(t_lhs.get_const_ptr()));
        } else if (t_oper > Operators::Opers::const_flag) {
          return const_unary_go(t_oper, *static_cast<const LHS *>(t_lhs.get_const_ptr()));
        } else {
          throw chaiscript::detail::exception::bad_any_cast();
        }
      }

      template<typename LHS>
      static auto go(Operators::Opers t_oper, const Boxed_Value &t_lhs) 
          -> typename std::enable_if<std::is_floating_point<LHS>::value, Boxed_Value>::type
      {
        if (t_oper > Operators::Opers::non_const_flag && t_oper < Operators::Opers::non_const_int_flag && !t_lhs.is_const() && !t_lhs.is_return_value()) {
          return unary_go(t_oper, *static_cast<LHS *>(t_lhs.get_ptr()), t_lhs);
        } else if (t_oper > Operators::Opers::const_flag) {
          return const_unary_go(t_oper, *static_cast<const LHS *>(t_lhs.get_const_ptr()));
        } else {
          throw chaiscript::detail::exception::bad_any_cast();
        }
      }

      template<typename LHS>
        inline static Boxed_Value oper_rhs(Operators::Opers t_oper, const Boxed_Value &t_lhs, const Boxed_Value &t_rhs)
        {
          switch (get_common_type(t_rhs)) {
            case Common_Types::t_int32:
              return go<LHS, int32_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_uint8:
              return go<LHS, uint8_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_int8:
              return go<LHS, int8_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_uint16:
              return go<LHS, uint16_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_int16:
              return go<LHS, int16_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_uint32:
              return go<LHS, uint32_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_uint64:
              return go<LHS, uint64_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_int64:
              return go<LHS, int64_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_double:
              return go<LHS, double>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_float:
              return go<LHS, float>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_long_double:
              return go<LHS, long double>(t_oper, t_lhs, t_rhs);
          }

          throw chaiscript::detail::exception::bad_any_cast();
        }

        inline static Boxed_Value oper(Operators::Opers t_oper, const Boxed_Value &t_lhs)
        {
          switch (get_common_type(t_lhs)) {
            case Common_Types::t_int32:
              return go<int32_t>(t_oper, t_lhs);
            case Common_Types::t_uint8:
              return go<uint8_t>(t_oper, t_lhs);
            case Common_Types::t_int8:
              return go<int8_t>(t_oper, t_lhs);
            case Common_Types::t_uint16:
              return go<uint16_t>(t_oper, t_lhs);
            case Common_Types::t_int16:
              return go<int16_t>(t_oper, t_lhs);
            case Common_Types::t_uint32:
              return go<uint32_t>(t_oper, t_lhs);
            case Common_Types::t_uint64:
              return go<uint64_t>(t_oper, t_lhs);
            case Common_Types::t_int64:
              return go<int64_t>(t_oper, t_lhs);
            case Common_Types::t_double:
              return go<double>(t_oper, t_lhs);
            case Common_Types::t_float:
              return go<float>(t_oper, t_lhs);
            case Common_Types::t_long_double:
              return go<long double>(t_oper, t_lhs);
          }

          throw chaiscript::detail::exception::bad_any_cast();
        }


        inline static Boxed_Value oper(Operators::Opers t_oper, const Boxed_Value &t_lhs, const Boxed_Value &t_rhs)
        {
          switch (get_common_type(t_lhs)) {
            case Common_Types::t_int32:
              return oper_rhs<int32_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_uint8:
              return oper_rhs<uint8_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_int8:
              return oper_rhs<int8_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_uint16:
              return oper_rhs<uint16_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_int16:
              return oper_rhs<int16_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_uint32:
              return oper_rhs<uint32_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_uint64:
              return oper_rhs<uint64_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_int64:
              return oper_rhs<int64_t>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_double:
              return oper_rhs<double>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_float:
              return oper_rhs<float>(t_oper, t_lhs, t_rhs);
            case Common_Types::t_long_double:
              return oper_rhs<long double>(t_oper, t_lhs, t_rhs);
          }

          throw chaiscript::detail::exception::bad_any_cast();
        }

        template<typename Target, typename Source>
          static inline Target get_as_aux(const Boxed_Value &t_bv)
          {
            return static_cast<Target>(*static_cast<const Source *>(t_bv.get_const_ptr()));
          }

        template<typename Source>
         static std::string to_string_aux(const Boxed_Value &v)
        {
          std::ostringstream oss;
          oss << *static_cast<const Source *>(v.get_const_ptr());
          return oss.str();
        }

    public:
      Boxed_Number()
        : bv(Boxed_Value(0))
      {
      }

      explicit Boxed_Number(Boxed_Value v)
        : bv(std::move(v))
      {
        validate_boxed_number(bv);
      }

      Boxed_Number(const Boxed_Number &) = default;
      Boxed_Number(Boxed_Number &&) = default;
      Boxed_Number& operator=(Boxed_Number &&) = default;

      template<typename T> explicit Boxed_Number(T t)
        : bv(Boxed_Value(t))
      {
        validate_boxed_number(bv);
      }

      static Boxed_Value clone(const Boxed_Value &t_bv) {
        return Boxed_Number(t_bv).get_as(t_bv.get_type_info()).bv;
      }

      static bool is_floating_point(const Boxed_Value &t_bv)
      {
        const Type_Info &inp_ = t_bv.get_type_info();

        if (inp_ == typeid(double)) {
          return true;
        } else if (inp_ == typeid(long double)) {
          return true;
        } else if (inp_ == typeid(float)) {
          return true;
        } else {
          return false;
        }
      }

      Boxed_Number get_as(const Type_Info &inp_) const
      {
        if (inp_.bare_equal_type_info(typeid(int))) {
          return Boxed_Number(get_as<int>());
        } else if (inp_.bare_equal_type_info(typeid(double))) {
          return Boxed_Number(get_as<double>());
        } else if (inp_.bare_equal_type_info(typeid(float))) {
          return Boxed_Number(get_as<float>());
        } else if (inp_.bare_equal_type_info(typeid(long double))) {
          return Boxed_Number(get_as<long double>());
        } else if (inp_.bare_equal_type_info(typeid(char))) {
          return Boxed_Number(get_as<char>());
        } else if (inp_.bare_equal_type_info(typeid(unsigned char))) {
          return Boxed_Number(get_as<unsigned char>());
        } else if (inp_.bare_equal_type_info(typeid(wchar_t))) {
          return Boxed_Number(get_as<wchar_t>());
        } else if (inp_.bare_equal_type_info(typeid(char16_t))) {
          return Boxed_Number(get_as<char16_t>());
        } else if (inp_.bare_equal_type_info(typeid(char32_t))) {
          return Boxed_Number(get_as<char32_t>());
        } else if (inp_.bare_equal_type_info(typeid(unsigned int))) {
          return Boxed_Number(get_as<unsigned int>());
        } else if (inp_.bare_equal_type_info(typeid(long))) {
          return Boxed_Number(get_as<long>());
        } else if (inp_.bare_equal_type_info(typeid(long long))) {
          return Boxed_Number(get_as<long long>());
        } else if (inp_.bare_equal_type_info(typeid(unsigned long))) {
          return Boxed_Number(get_as<unsigned long>());
        } else if (inp_.bare_equal_type_info(typeid(unsigned long long))) {
          return Boxed_Number(get_as<unsigned long long>());
        } else if (inp_.bare_equal_type_info(typeid(int8_t))) {
          return Boxed_Number(get_as<int8_t>());
        } else if (inp_.bare_equal_type_info(typeid(int16_t))) {
          return Boxed_Number(get_as<int16_t>());
        } else if (inp_.bare_equal_type_info(typeid(int32_t))) {
          return Boxed_Number(get_as<int32_t>());
        } else if (inp_.bare_equal_type_info(typeid(int64_t))) {
          return Boxed_Number(get_as<int64_t>());
        } else if (inp_.bare_equal_type_info(typeid(uint8_t))) {
          return Boxed_Number(get_as<uint8_t>());
        } else if (inp_.bare_equal_type_info(typeid(uint16_t))) {
          return Boxed_Number(get_as<uint16_t>());
        } else if (inp_.bare_equal_type_info(typeid(uint32_t))) {
          return Boxed_Number(get_as<uint32_t>());
        } else if (inp_.bare_equal_type_info(typeid(uint64_t))) {
          return Boxed_Number(get_as<uint64_t>());
        } else {
          throw chaiscript::detail::exception::bad_any_cast();
        }

      }

      template<typename Source, typename Target> 
      static void check_type()
      {
#ifdef CHAISCRIPT_MSVC
// MSVC complains about this being redundant / tautologica l
#pragma warning(push)
#pragma warning(disable : 4127 6287)
#endif
        if (sizeof(Source) != sizeof(Target)
            || std::is_signed<Source>() != std::is_signed<Target>()
            || std::is_floating_point<Source>() != std::is_floating_point<Target>())
        {
          throw chaiscript::detail::exception::bad_any_cast();
        }
#ifdef CHAISCRIPT_MSVC
#pragma warning(pop)
#endif
      }

      template<typename Target> Target get_as_checked() const
      {
        switch (get_common_type(bv)) {
          case Common_Types::t_int32:
            check_type<int32_t, Target>();
            return get_as_aux<Target, int32_t>(bv);
          case Common_Types::t_uint8:
            check_type<uint8_t, Target>();
            return get_as_aux<Target, uint8_t>(bv);
          case Common_Types::t_int8:
            check_type<int8_t, Target>();
            return get_as_aux<Target, int8_t>(bv);
          case Common_Types::t_uint16:
            check_type<uint16_t, Target>();
            return get_as_aux<Target, uint16_t>(bv);
          case Common_Types::t_int16:
            check_type<int16_t, Target>();
            return get_as_aux<Target, int16_t>(bv);
          case Common_Types::t_uint32:
            check_type<uint32_t, Target>();
            return get_as_aux<Target, uint32_t>(bv);
          case Common_Types::t_uint64:
            check_type<uint64_t, Target>();
            return get_as_aux<Target, uint64_t>(bv);
          case Common_Types::t_int64:
            check_type<int64_t, Target>();
            return get_as_aux<Target, int64_t>(bv);
          case Common_Types::t_double:
            check_type<double, Target>();
            return get_as_aux<Target, double>(bv);
          case Common_Types::t_float:
            check_type<float, Target>();
            return get_as_aux<Target, float>(bv);
          case Common_Types::t_long_double:
            check_type<long double, Target>();
            return get_as_aux<Target, long double>(bv);
        }

        throw chaiscript::detail::exception::bad_any_cast();
      }


      template<typename Target> Target get_as() const
      {
        switch (get_common_type(bv)) {
          case Common_Types::t_int32:
            return get_as_aux<Target, int32_t>(bv);
          case Common_Types::t_uint8:
            return get_as_aux<Target, uint8_t>(bv);
          case Common_Types::t_int8:
            return get_as_aux<Target, int8_t>(bv);
          case Common_Types::t_uint16:
            return get_as_aux<Target, uint16_t>(bv);
          case Common_Types::t_int16:
            return get_as_aux<Target, int16_t>(bv);
          case Common_Types::t_uint32:
            return get_as_aux<Target, uint32_t>(bv);
          case Common_Types::t_uint64:
            return get_as_aux<Target, uint64_t>(bv);
          case Common_Types::t_int64:
            return get_as_aux<Target, int64_t>(bv);
          case Common_Types::t_double:
            return get_as_aux<Target, double>(bv);
          case Common_Types::t_float:
            return get_as_aux<Target, float>(bv);
          case Common_Types::t_long_double:
            return get_as_aux<Target, long double>(bv);
        }

        throw chaiscript::detail::exception::bad_any_cast();
      }

      std::string to_string() const
      {
        switch (get_common_type(bv)) {
          case Common_Types::t_int32:
            return std::to_string(get_as<int32_t>());
          case Common_Types::t_uint8:
            return std::to_string(get_as<uint32_t>());
          case Common_Types::t_int8:
            return std::to_string(get_as<int32_t>());
          case Common_Types::t_uint16:
            return std::to_string(get_as<uint16_t>());
          case Common_Types::t_int16:
            return std::to_string(get_as<int16_t>());
          case Common_Types::t_uint32:
            return std::to_string(get_as<uint32_t>());
          case Common_Types::t_uint64:
            return std::to_string(get_as<uint64_t>());
          case Common_Types::t_int64:
            return std::to_string(get_as<int64_t>());
          case Common_Types::t_double:
            return to_string_aux<double>(bv);
          case Common_Types::t_float:
            return to_string_aux<float>(bv);
          case Common_Types::t_long_double:
            return to_string_aux<long double>(bv);
        }

        throw chaiscript::detail::exception::bad_any_cast();
      }

      static void validate_boxed_number(const Boxed_Value &v)
      {
        const Type_Info &inp_ = v.get_type_info();
        if (inp_ == typeid(bool))
        {
          throw chaiscript::detail::exception::bad_any_cast();
        }

        if (!inp_.is_arithmetic())
        {
          throw chaiscript::detail::exception::bad_any_cast();
        }
      }



      static bool equals(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs)
      {
        return boxed_cast<bool>(oper(Operators::Opers::equals, t_lhs.bv, t_rhs.bv));
      }

      static bool less_than(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs)
      {
        return boxed_cast<bool>(oper(Operators::Opers::less_than, t_lhs.bv, t_rhs.bv));
      }

      static bool greater_than(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs) 
      {
        return boxed_cast<bool>(oper(Operators::Opers::greater_than, t_lhs.bv, t_rhs.bv));
      }

      static bool greater_than_equal(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs)
      {
        return boxed_cast<bool>(oper(Operators::Opers::greater_than_equal, t_lhs.bv, t_rhs.bv));
      }

      static bool less_than_equal(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs)
      {
        return boxed_cast<bool>(oper(Operators::Opers::less_than_equal, t_lhs.bv, t_rhs.bv));
      }

      static bool not_equal(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs) 
      {
        return boxed_cast<bool>(oper(Operators::Opers::not_equal, t_lhs.bv, t_rhs.bv));
      }

      static Boxed_Number pre_decrement(Boxed_Number t_lhs)
      {
        return Boxed_Number(oper(Operators::Opers::pre_decrement, t_lhs.bv));
      }

      static Boxed_Number pre_increment(Boxed_Number t_lhs) 
      {
        return Boxed_Number(oper(Operators::Opers::pre_increment, t_lhs.bv));
      }

      static const Boxed_Number sum(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs) 
      {
        return Boxed_Number(oper(Operators::Opers::sum, t_lhs.bv, t_rhs.bv));
      }

      static const Boxed_Number unary_plus(const Boxed_Number &t_lhs) 
      {
        return Boxed_Number(oper(Operators::Opers::unary_plus, t_lhs.bv));
      }

      static const Boxed_Number unary_minus(const Boxed_Number &t_lhs)
      {
        return Boxed_Number(oper(Operators::Opers::unary_minus, t_lhs.bv));
      }

      static const Boxed_Number difference(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs) 
      {
        return Boxed_Number(oper(Operators::Opers::difference, t_lhs.bv, t_rhs.bv));
      }

      static Boxed_Number assign_bitwise_and(Boxed_Number t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::assign_bitwise_and, t_lhs.bv, t_rhs.bv));
      }

      static Boxed_Number assign(Boxed_Number t_lhs, const Boxed_Number &t_rhs) 
      {
        return Boxed_Number(oper(Operators::Opers::assign, t_lhs.bv, t_rhs.bv));
      }

      static Boxed_Number assign_bitwise_or(Boxed_Number t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::assign_bitwise_or, t_lhs.bv, t_rhs.bv));
      }

      static Boxed_Number assign_bitwise_xor(Boxed_Number t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::assign_bitwise_xor, t_lhs.bv, t_rhs.bv));
      }

      static Boxed_Number assign_remainder(Boxed_Number t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::assign_remainder, t_lhs.bv, t_rhs.bv));
      }

      static Boxed_Number assign_shift_left(Boxed_Number t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::assign_shift_left, t_lhs.bv, t_rhs.bv));
      }

      static Boxed_Number assign_shift_right(Boxed_Number t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::assign_shift_right, t_lhs.bv, t_rhs.bv));
      }

      static const Boxed_Number bitwise_and(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::bitwise_and, t_lhs.bv, t_rhs.bv));
      }

      static const Boxed_Number bitwise_complement(const Boxed_Number &t_lhs)
      {
        return Boxed_Number(oper(Operators::Opers::bitwise_complement, t_lhs.bv, Boxed_Value(0)));
      }

      static const Boxed_Number bitwise_xor(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::bitwise_xor, t_lhs.bv, t_rhs.bv));
      }

      static const Boxed_Number bitwise_or(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::bitwise_or, t_lhs.bv, t_rhs.bv));
      }

      static Boxed_Number assign_product(Boxed_Number t_lhs, const Boxed_Number &t_rhs) 
      {
        return Boxed_Number(oper(Operators::Opers::assign_product, t_lhs.bv, t_rhs.bv));
      }

      static Boxed_Number assign_quotient(Boxed_Number t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::assign_quotient, t_lhs.bv, t_rhs.bv));
      }

      static Boxed_Number assign_sum(Boxed_Number t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::assign_sum, t_lhs.bv, t_rhs.bv));
      }
      static Boxed_Number assign_difference(Boxed_Number t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::assign_difference, t_lhs.bv, t_rhs.bv));
      }

      static const Boxed_Number quotient(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs) 
      {
        return Boxed_Number(oper(Operators::Opers::quotient, t_lhs.bv, t_rhs.bv));
      }

      static const Boxed_Number shift_left(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::shift_left, t_lhs.bv, t_rhs.bv));
      }

      static const Boxed_Number product(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::product, t_lhs.bv, t_rhs.bv));
      }

      static const Boxed_Number remainder(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::remainder, t_lhs.bv, t_rhs.bv));
      }

      static const Boxed_Number shift_right(const Boxed_Number &t_lhs, const Boxed_Number &t_rhs)
      {
        return Boxed_Number(oper(Operators::Opers::shift_right, t_lhs.bv, t_rhs.bv));
      }



      static Boxed_Value do_oper(Operators::Opers t_oper, const Boxed_Value &t_lhs, const Boxed_Value &t_rhs)
      {
        return oper(t_oper, t_lhs, t_rhs);
      }

      static Boxed_Value do_oper(Operators::Opers t_oper, const Boxed_Value &t_lhs)
      {
        return oper(t_oper, t_lhs);
      }



      Boxed_Value bv;
  };

  namespace detail
  {
    /// Cast_Helper for converting from Boxed_Value to Boxed_Number
    template<>
      struct Cast_Helper<Boxed_Number>
      {
        static Boxed_Number cast(const Boxed_Value &ob, const Type_Conversions_State *)
        {
          return Boxed_Number(ob);
        }
      };

    /// Cast_Helper for converting from Boxed_Value to Boxed_Number
    template<>
      struct Cast_Helper<const Boxed_Number &> : Cast_Helper<Boxed_Number>
      {
      };

    /// Cast_Helper for converting from Boxed_Value to Boxed_Number
    template<>
      struct Cast_Helper<const Boxed_Number> : Cast_Helper<Boxed_Number>
      {
      };
  }

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef CHAISCRIPT_MSVC
#pragma warning(pop)
#endif

}



#endif

