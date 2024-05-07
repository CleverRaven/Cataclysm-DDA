#pragma once
#ifndef CATA_SRC_MATH_PARSER_DIAG_VALUE_H
#define CATA_SRC_MATH_PARSER_DIAG_VALUE_H

#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "dialogue_helpers.h"
#include "math_parser.h"

class math_exp;
struct dialogue;
struct diag_value;
using diag_array = std::vector<diag_value>;
struct diag_value {
    diag_value() = default;
    template <class U>
    explicit diag_value( U u ) : data( std::in_place_type<U>, std::forward<U>( u ) ) {}
    template <class T, class... Args>
    explicit diag_value( std::in_place_type_t<T> /*t*/, Args &&...args )
        : data( std::in_place_type<T>, std::forward<Args>( args )... ) {}

    // these functions can be used at parse time if the parameter needs to be of exactly this type
    // with no conversion. These throw so they should *NOT* be used at runtime.
    bool is_dbl() const;
    double dbl() const;
    bool is_str() const;
    std::string_view str() const;
    bool is_var() const;
    var_info var() const;
    bool is_array() const;
    diag_array const &array() const;

    // evaluate and possibly convert the parameter to this type
    double dbl( dialogue const &d ) const;
    std::string str( dialogue const &d ) const;
    diag_array const &array( dialogue const &/* d */ ) const;

    using impl_t = std::variant<double, std::string, var_info, math_exp, diag_array>;
    impl_t data;
};

constexpr bool operator==( diag_value const &lhs, std::string_view rhs )
{
    return std::holds_alternative<std::string>( lhs.data ) && std::get<std::string>( lhs.data ) == rhs;
}

// helper struct that makes it easy to determine whether a kwarg's value has been used
struct deref_diag_value {
    public:
        deref_diag_value() = default;
        explicit deref_diag_value( diag_value &&dv ) : _val( dv ) {}
        diag_value const *operator->() const {
            _used = true;
            return &_val;
        }
        diag_value const &operator*() const {
            _used = true;
            return _val;
        }
        bool was_used() const {
            return _used;
        }
    private:
        bool mutable _used = false;
        diag_value _val;
};

#endif // CATA_SRC_MATH_PARSER_DIAG_VALUE_H
