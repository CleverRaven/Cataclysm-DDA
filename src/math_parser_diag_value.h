#pragma once
#ifndef CATA_SRC_MATH_PARSER_DIAG_VALUE_H
#define CATA_SRC_MATH_PARSER_DIAG_VALUE_H

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "cata_variant.h"
#include "coordinates.h"

class JsonOut;
class JsonValue;
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
    struct legacy_value;
    using impl_t = std::variant <
                   std::monostate, double, std::string,
                   diag_array, tripoint_abs_ms, legacy_value
                   >;

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

    explicit diag_value( cata_variant const &cv );

    bool is_dbl() const;
    bool is_str() const;
    bool is_array() const;
    bool is_tripoint() const;
    bool is_empty() const;

    // These functions show a debugmsg on type mismatches
    double dbl() const;
    std::string const &str() const;
    diag_array const &array() const;
    tripoint_abs_ms const &tripoint() const;

    // These functions throw a math::runtime_error on type mismatches
    // and are meant to be used only inside EOC and math code
    double dbl( const_dialogue const &d ) const;
    std::string const &str( const_dialogue const &d ) const;
    diag_array const &array( const_dialogue const &d ) const;
    tripoint_abs_ms const &tripoint( const_dialogue const &d ) const;

    std::string to_string( bool i18n = false ) const;

    void serialize( JsonOut & ) const;
    void deserialize( const JsonValue &jsin );

    struct legacy_value {
        std::string val;
        std::shared_ptr<diag_value::impl_t> mutable converted;

        explicit legacy_value( std::string in_ ) : val( std::move( in_ ) ) {}
    };

    impl_t data;
};

bool operator==( diag_value::legacy_value const &lhs, diag_value::legacy_value const &rhs );
bool operator!=( diag_value::legacy_value const &lhs, diag_value::legacy_value const &rhs );
bool operator==( diag_value const &lhs, diag_value const &rhs );
bool operator!=( diag_value const &lhs, diag_value const &rhs );

// these ignore legacy_value so only use them in new code
bool operator==( diag_value const &lhs, double rhs );
bool operator!=( diag_value const &lhs, double rhs );
bool operator==( diag_value const &lhs, std::string_view rhs );
bool operator!=( diag_value const &lhs, std::string_view rhs );
bool operator==( diag_value const &lhs, tripoint_abs_ms const &rhs );
bool operator!=( diag_value const &lhs, tripoint_abs_ms const &rhs );

#endif // CATA_SRC_MATH_PARSER_DIAG_VALUE_H
