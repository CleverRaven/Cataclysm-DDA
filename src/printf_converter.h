#pragma once
#ifndef CATA_SRC_PRINTF_CONVERTER_H
#define CATA_SRC_PRINTF_CONVERTER_H

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

class cata_path;
class translation;
namespace std
{
namespace filesystem
{
class path;
}
};

template<typename T, typename Enable = void>
struct printf_converter {
    template<typename U = T>
    decltype( auto ) convert( U && u ) {
        return std::forward<U>( u );
    }
};

template<typename E>
struct printf_converter<E, std::enable_if_t<std::is_enum_v<E>>> {
    auto convert( E e ) {
        return static_cast<std::underlying_type_t<E>>( e );
    }
};

template<>
struct printf_converter<cata_path> {
    std::string buffer;
    const char *convert( const cata_path &p );
};

template<>
struct printf_converter<std::filesystem::path> {
    std::string buffer;
    const char *convert( const std::filesystem::path &p );
};


template<>
struct printf_converter<translation> {
    const char *convert( const translation &t );
};

template<>
struct printf_converter<std::string> {
    inline const char *convert( const std::string &s ) {
        return s.c_str();
    }
};

#ifdef __cpp_char8_t
template<>
struct printf_converter<std::u8string> {
    std::string buffer;
    const char *convert( const std::u8string & );
};
#endif

#endif // CATA_SRC_PRINTF_CONVERTER_H
