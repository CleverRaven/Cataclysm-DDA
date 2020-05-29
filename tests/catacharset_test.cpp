#include <clocale>
#include <stdexcept>
#include <string>
#include <vector>

#include "catacharset.h"
#include "catch/catch.hpp"
#include "translations.h"

TEST_CASE( "utf8_width", "[catacharset]" )
{
    CHECK( utf8_width( "Hello, world!", false ) == 13 );
    CHECK( utf8_width( "你好，世界！", false ) == 12 );
    CHECK( utf8_width( "Hello, 世界!", false ) == 12 );
    CHECK( utf8_width( "<color_green>激活</color>", true ) == 4 );
    CHECK( utf8_width( "<color_green>激活</color>", false ) == 25 );
    CHECK( utf8_width( "à", false ) == 1 );
    CHECK( utf8_width( "y\u0300", false ) == 1 );
    CHECK( utf8_width( "à̸̠你⃫", false ) == 3 );
}

TEST_CASE( "utf8_display_split", "[catacharset]" )
{
    CHECK( utf8_display_split( "你好" ) == std::vector<std::string> { "你", "好" } );
    CHECK( utf8_display_split( "à" ) == std::vector<std::string> { "à" } );
    CHECK( utf8_display_split( "y\u0300" ) == std::vector<std::string> { "y\u0300" } );
    CHECK( utf8_display_split( "à̸̠你⃫" ) == std::vector<std::string> { "à̸̠", "你⃫" } );
    CHECK( utf8_display_split( "    " ) == std::vector<std::string> { " ", " ", " ", " " } );
}

TEST_CASE( "base64", "[catacharset]" )
{
    CHECK( base64_encode( "hello" ) == "#aGVsbG8=" );
    CHECK( base64_decode( "#aGVsbG8=" ) == "hello" );
}

TEST_CASE( "utf8_to_wstr", "[catacharset]" )
{
    // std::mbstowcs' returning -1 workaround
    setlocale( LC_ALL, "" );
    std::string src( u8"Hello, 世界!" );
    std::wstring dest( L"Hello, 世界!" );
    CHECK( utf8_to_wstr( src ) == dest );
    setlocale( LC_ALL, "C" );
}

TEST_CASE( "wstr_to_utf8", "[catacharset]" )
{
    // std::wcstombs' returning -1 workaround
    setlocale( LC_ALL, "" );
    std::wstring src( L"Hello, 世界!" );
    std::string dest( u8"Hello, 世界!" );
    CHECK( wstr_to_utf8( src ) == dest );
    setlocale( LC_ALL, "C" );
}

TEST_CASE( "localized_compare", "[catacharset]" )
{
    try {
        std::locale::global( std::locale( "en_US.UTF-8" ) );
    } catch( std::runtime_error & ) {
        // On platforms where we can't set the locale, ignore this test
        return;
    }
    CAPTURE( setlocale( LC_ALL, nullptr ) );
    CAPTURE( std::locale().name() );
    std::string a = "a";
    std::string b = "b";
    std::string c = "c";
    std::string A = "A";
    std::string B = "B";
    CHECK( !localized_compare( a, a ) );
    CHECK( localized_compare( a, B ) );
    CHECK( localized_compare( A, b ) );
    CHECK( !localized_compare( B, a ) );
    CHECK( !localized_compare( b, A ) );
    CHECK( localized_compare( std::make_pair( a, a ), std::make_pair( a, B ) ) );
    CHECK( localized_compare( std::make_tuple( a ), std::make_tuple( B ) ) );
    CHECK( localized_compare( std::make_tuple( a, c, c ), std::make_tuple( B, a, a ) ) );
    CHECK( localized_compare( std::make_tuple( a, a, c ), std::make_tuple( a, B, a ) ) );
    CHECK( localized_compare( std::make_tuple( a, a, a ), std::make_tuple( a, a, B ) ) );
    std::locale::global( std::locale::classic() );
}
