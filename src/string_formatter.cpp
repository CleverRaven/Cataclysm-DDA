#include "string_formatter.h"

#include <exception>

#include <fmt/format.h>
#include <fmt/printf.h>

#include "cata_path.h"
#include "debug.h"
#include "translation.h"

std::string string_vfmt( fmt::string_view format, fmt::format_args args )
{
    try {
        return fmt::vformat( format, args );
    } catch( const std::exception &err ) {
        debugmsg( std::string{ err.what() } + ": " + std::string{ format.data(), format.size() } );
        return err.what();
    }
}

const char *printf_converter<cata_path>::convert( const cata_path &p )
{
    buffer = p.generic_u8string();
    return buffer.c_str();
}

const char *printf_converter<translation>::convert( const translation &t )
{
    return t.translated().c_str();
}

std::string string_vprintf( fmt::string_view format, fmt::printf_args args )
{
    try {
        return fmt::vsprintf( format, args );
    } catch( const std::exception &err ) {
        debugmsg( std::string{ err.what() } + ": " + std::string{ format.data(), format.size() } );
        return err.what();
    }
}
