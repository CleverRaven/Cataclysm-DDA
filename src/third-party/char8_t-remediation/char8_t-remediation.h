// Copyright (c) 2019, Tom Honermann
//
// This file is distributed under the MIT License. See the accompanying file
// LICENSE.txt or http://www.opensource.org/licenses/mit-license.php for terms
// and conditions.

#ifndef CHAR8_T_REMEDIATION_HPP // {
#define CHAR8_T_REMEDIATION_HPP


#include <algorithm>
#include <string>
#include <utility>


#if defined(__cpp_char8_t) // {

#if defined(__cpp_nontype_template_parameter_class) \
 && defined(__cpp_deduction_guides) // {

// Non-type template parameters with class type are supported.  This enables
// defining a constexpr user-defined literal that returns a reference to a
// static buffer of type char that contains the contents of the UTF-8 literal.

namespace char8_t_remediation {

template<std::size_t N>
struct char8_t_string_literal {
    static constexpr inline std::size_t size = N;
    template<std::size_t... I>
    constexpr char8_t_string_literal(
        const char8_t (&r)[N],
        std::index_sequence<I...>)
    :
        s{r[I]...}
    {}
    constexpr char8_t_string_literal(
        const char8_t (&r)[N])
    :
        char8_t_string_literal(r, std::make_index_sequence<N>())
    {}
// P0732R2 requires that operator <=> be defined for class types used as
// non-type template arguments, but gcc, as of version 9.1.0, does not yet
// implement support for operator <=>, but does provide an implementation
// of P0732R2 (that presumably relies on implicit memberwise comparison).
#if defined(__cpp_impl_three_way_comparison) // {
    auto operator <=>(const char8_t_string_literal&) = default;
#endif // }
    char8_t s[N];
};
template<std::size_t N>
char8_t_string_literal(const char8_t(&)[N]) -> char8_t_string_literal<N>;

template<char8_t_string_literal L, std::size_t... I>
constexpr inline const char as_char_buffer[sizeof...(I)] =
    { static_cast<char>(L.s[I])... };

template<char8_t_string_literal L, std::size_t... I>
constexpr auto& make_as_char_buffer(std::index_sequence<I...>) {
    return as_char_buffer<L, I...>;
}

} // namespace char8_t_remediation

inline constexpr char operator ""_as_char(char8_t c) {
    return c;
}

// gcc requires a fix for bug 88095 in order to deduce the template arguments
// for the non-type template parameter.  A patch is available in
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88095
template<char8_t_remediation::char8_t_string_literal L>
constexpr auto& operator""_as_char() {
    return char8_t_remediation::make_as_char_buffer<L>(
               std::make_index_sequence<decltype(L)::size>());
}

#define U8(x) u8##x##_as_char


#else // } !defined(__cpp_nontype_template_parameter_class) ||
      //   !defined(__cpp_deduction_guides) {


// Non-type template parameters with class type are not supported.  The
// best we can do is reinterpret_cast the UTF-8 literal as a pointer to
// const char.

inline constexpr char operator ""_as_char(char8_t c) {
    return c;
}

inline const char* operator""_as_char(const char8_t *p, std::size_t) {
    return reinterpret_cast<const char*>(p);
}

#define U8(x) u8##x##_as_char


#endif // }


#else // } !defined(__cpp_char8_t) {


#define U8(x) u8##x


#endif // }


// char_array provides an alternative declaration for C-style arrays that
// preserves the ability to initialize a char array with a u8 string literal.
// C++14 or later is required for index_sequence and make_index_sequence.

#if __cplusplus >= 201402L // {

template<std::size_t N>
struct char_array {
    template<std::size_t P, std::size_t... I>
    constexpr char_array(
        const char (&r)[P],
        std::index_sequence<I...>)
    :
        data{(I<P?r[I]:'\0')...}
    {}
    template<std::size_t P, typename = std::enable_if_t<(P<=N)>>
    constexpr char_array(const char(&r)[P])
        : char_array(r, std::make_index_sequence<N>())
    {}

#if defined(__cpp_char8_t) // {
    template<std::size_t P, std::size_t... I>
    constexpr char_array(
        const char8_t (&r)[P],
        std::index_sequence<I...>)
    :
        data{(I<P?static_cast<char>(r[I]):'\0')...}
    {}
    template<std::size_t P, typename = std::enable_if_t<(P<=N)>>
    constexpr char_array(const char8_t(&r)[P])
        : char_array(r, std::make_index_sequence<N>())
    {}
#endif // }

    using const_conversion_type = const char(&)[N];
    constexpr operator const_conversion_type() const {
        return data;
    }
    using conversion_type = const char(&)[N];
    constexpr operator conversion_type() {
        return data;
    }

    char data[N];
};

#if defined(__cpp_deduction_guides) // {
template<std::size_t N>
char_array(const char(&)[N]) -> char_array<N>;
#if defined(__cpp_char8_t) // {
template<std::size_t N>
char_array(const char8_t(&)[N]) -> char_array<N>;
#endif // }
#endif // }

#endif // }


// from_u8string enables explicit conversion from std::u8string to
// std::string.

inline std::string from_u8string(const std::string &s) {
  return s;
}
inline std::string from_u8string(std::string &&s) {
  return std::move(s);
}
#if defined(__cpp_lib_char8_t)
inline std::string from_u8string(const std::u8string &s) {
  return std::string(s.begin(), s.end());
}
#endif


#endif // } CHAR8_T_REMEDIATION_HPP
