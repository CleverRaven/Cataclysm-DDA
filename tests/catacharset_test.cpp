#include <clocale>
#include <string>

#include "catch/catch.hpp"
#include "catacharset.h"

TEST_CASE( "utf8_width" )
{
    CHECK( utf8_width( "Hello, world!", false ) == 13 );
    CHECK( utf8_width( "你好，世界！", false ) == 12 );
    CHECK( utf8_width( "Hello, 世界!", false ) == 12 );
    CHECK( utf8_width( "<color_green>激活</color>", true ) == 4 );
    CHECK( utf8_width( "<color_green>激活</color>", false ) == 25 );
}

TEST_CASE( "base64" )
{
    CHECK( base64_encode( "hello" ) == "#aGVsbG8=" );
    CHECK( base64_decode( "#aGVsbG8=" ) == "hello" );
}

TEST_CASE( "utf8_to_wstr" )
{
    // std::mbstowcs' returning -1 workaround
    setlocale( LC_ALL, "" );
    std::string src( u8"Hello, 世界!" );
    std::wstring dest( L"Hello, 世界!" );
    CHECK( utf8_to_wstr( src ) == dest );
}

TEST_CASE( "wstr_to_utf8" )
{
    // std::wcstombs' returning -1 workaround
    setlocale( LC_ALL, "" );
    std::wstring src( L"Hello, 世界!" );
    std::string dest( u8"Hello, 世界!" );
    CHECK( wstr_to_utf8( src ) == dest );
}
