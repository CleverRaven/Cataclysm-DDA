#pragma once
#ifndef CATA_SRC_MATH_PARSER_DIAG_VALUE_H
#define CATA_SRC_MATH_PARSER_DIAG_VALUE_H

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

struct const_dialogue;
struct diag_value;

using diag_array = std::vector<diag_value>;

// https://stackoverflow.com/a/45896101
// *INDENT-OFF*
template <class T, class U>
struct is_one_of;

template <class T, class... Ts>
// NOLINTNEXTLINE(cata-avoid-alternative-tokens) broken check
struct is_one_of<T, std::variant<Ts...>> : std::bool_constant<( std::is_same_v<T, Ts> || ... )> {};
template <class T, class V>
using is_variant_type = is_one_of<T, V>;
// *INDENT-ON*

struct diag_value {
    using impl_t = std::variant<std::monostate, double, std::string, diag_array>;

    diag_value() = default;

    // *INDENT-OFF* astyle formatting isn't stable here
    template <typename D,
              std::enable_if_t<!is_variant_type<D, impl_t>{} && std::is_arithmetic_v<D>, int> = 0>
    explicit diag_value( D d ) : data( std::in_place_type<double>, std::forward<D>( d ) ) {}

    template <typename S,
              std::enable_if_t<
                  !is_variant_type<S, impl_t>{} && std::is_convertible_v<S, std::string>, int> = 0>
    explicit diag_value( S s ) : data( std::in_place_type<std::string>, std::forward<S>( s ) ) {}

    template <class U, std::enable_if_t<is_variant_type<U, impl_t>{}, int> = 0>
    explicit diag_value( U u ) : data( std::in_place_type<U>, std::forward<U>( u ) ) {}
    // *INDENT-ON*

    template <class T, class... Args>
    explicit diag_value( std::in_place_type_t<T> /*t*/, Args &&...args )
        : data( std::in_place_type<T>, std::forward<Args>( args )... ) {}

    bool is_dbl() const;
    bool is_str() const;
    bool is_array() const;
    bool is_empty() const;

    // These functions can be used at parse time if the parameter needs
    // to be of exactly this type with no conversion.
    // These throw a math::syntax_error for type mismatches
    double dbl() const;
    std::string_view str() const;
    diag_array const &array() const;

    // Evaluate and possibly convert the parameter to this type.
    // These throw a math::runtime_error for failed conversions
    double dbl( const_dialogue const &d ) const;
    std::string str( const_dialogue const &d ) const;
    diag_array const &array( const_dialogue const &/* d */ ) const;

    impl_t data;
};

constexpr bool operator==( diag_value const &lhs, std::string_view rhs )
{
    return std::holds_alternative<std::string>( lhs.data ) && std::get<std::string>( lhs.data ) == rhs;
}

#endif // CATA_SRC_MATH_PARSER_DIAG_VALUE_H
