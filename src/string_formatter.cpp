#include "string_formatter.h"

#include <exception>

#include <fmt/format.h>
#include <fmt/printf.h>

#include "filesystem.h"
#include "cata_path.h"
#include "debug.h"
#include "translation.h"
#include "unicode.h"

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

const char *printf_converter<std::filesystem::path>::convert( const std::filesystem::path &p )
{
    auto u8 = p.generic_u8string();
    buffer = from_u8string( u8 );
    return buffer.c_str();
}

const char *printf_converter<translation>::convert( const translation &t )
{
    return t.translated().c_str();
}

#ifdef __cpp_char8_t
const char *printf_converter<std::u8string>::convert( const std::u8string &u8 )
{
    buffer = from_u8string( u8 );
    return buffer.c_str();
}
#endif

std::string string_vprintf( fmt::string_view format, fmt::printf_args args )
{
    try {
        return fmt::vsprintf( format, args );
    } catch( const std::exception &err ) {
        debugmsg( std::string{ err.what() } + ": " + std::string{ format.data(), format.size() } );
        return err.what();
    }
}

auto fmt::formatter<cata_path>::format( const cata_path &path,
                                        fmt::format_context &ctx ) const -> decltype( ctx.out() )
{
    return formatter<std::string_view>::format( from_u8string( path.generic_u8string() ), ctx );
}

auto fmt::formatter<std::filesystem::path>::format( const std::filesystem::path &path,
        fmt::format_context &ctx ) const -> decltype( ctx.out() )
{
    return formatter<std::string_view>::format( from_u8string( path.generic_u8string() ), ctx );
}
