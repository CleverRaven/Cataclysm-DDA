#pragma once
#ifndef CATA_SRC_MATH_PARSER_TYPE_H
#define CATA_SRC_MATH_PARSER_TYPE_H

#include <exception>
#include <string>

#include "string_formatter.h"

enum class math_type_t : int {
    ret = 0,
    compare,
    assign,
};

namespace math
{
template<int severity>
class math_exception_base : public std::exception
{
    private:
        std::string msg;
    public:
        template <typename... Args>
        explicit math_exception_base( Args &&...args )
            : msg( string_format( std::forward<Args>( args )... ) ) {}
        const char *what() const noexcept override {
            return msg.c_str();
        }
};

// syntax error in parsed expression
using syntax_error = math_exception_base<0>;
// runtime error that can't be found at parse time, like bad variable types or values
using runtime_error = math_exception_base<5>;
// math parser entered an unexpected state
using internal_error = math_exception_base<10>;
} // namespace math

#endif // CATA_SRC_MATH_PARSER_TYPE_H
