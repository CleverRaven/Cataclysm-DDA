#pragma once
#ifndef CATA_SRC_MATH_PARSER_TYPE_H
#define CATA_SRC_MATH_PARSER_TYPE_H

#include <stdexcept>
#include <string>

#include "string_formatter.h"

enum class math_type_t : std::uint8_t {
    ret = 0,
    compare,
    assign,
};

namespace math
{

template<int severity>
constexpr std::string_view _severity_str();

class exception: public std::runtime_error
{
    public:
        explicit exception( std::string const &str ): std::runtime_error( str ) {}
};

template<int severity_>
class math_exception_impl : public exception
{
    public:
        static constexpr int severity = severity_;

        template <typename... Args>
        constexpr explicit math_exception_impl( Args &&...args )
            : exception( string_format( "%s: %s", _severity_str<severity_>(),
                                        string_format( std::forward<Args>( args )... ) ) ) {}
};

// syntax error in parsed expression
using syntax_error = math_exception_impl<0>;
// runtime error that can't be found at parse time, like bad variable types or values
using runtime_error = math_exception_impl<5>;
// math parser entered an unexpected state
using internal_error = math_exception_impl<10>;

template<int severity>
constexpr std::string_view _severity_str()
{
    if constexpr( severity == syntax_error::severity ) {
        return "Math syntax error";
    } else if constexpr( severity == runtime_error::severity ) {
        return "Math runtime error";
    } else if constexpr( severity == internal_error::severity ) {
        return "Fatal math error";
    }
    return "Math error";
}
} // namespace math

#endif // CATA_SRC_MATH_PARSER_TYPE_H
