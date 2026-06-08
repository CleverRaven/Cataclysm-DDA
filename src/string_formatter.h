#pragma once
#ifndef CATA_SRC_STRING_FORMATTER_H
#define CATA_SRC_STRING_FORMATTER_H

#include <string>
#include <string_view>
#include <type_traits>

#include "printf_converter.h"

// #include <fmt/format.h>
#include <fmt/printf.h>

extern std::string string_vfmt( fmt::string_view format, fmt::format_args args );

extern std::string string_vprintf( fmt::string_view format, fmt::printf_args args );

template<typename ...Args>
inline std::string string_format_inner( std::string_view format, Args &&...args )
{
    return string_vprintf( format, fmt::make_printf_args( args... ) );
}

template<typename ...Args>
inline std::string string_format( std::string_view format, Args &&...args )
{
    return string_format_inner( format, printf_converter<std::decay_t<Args>> {}.convert( args )... );
}

#endif // CATA_SRC_STRING_FORMATTER_H
