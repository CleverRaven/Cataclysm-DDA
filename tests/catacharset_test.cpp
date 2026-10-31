#include <clocale>
#include <functional>
#include <locale>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <io.h>
#else
#include <stdio.h>
#include <unistd.h>
#endif
#include <char8_t-remediation/char8_t-remediation.h>
#include "cata_catch.h"
#include "catacharset.h"
#include "localized_comparator.h"
#include "output.h"
#include "unicode.h"

TEST_CASE( "utf8_width", "[catacharset][nogame]" )
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

TEST_CASE( "utf8_display_split", "[catacharset][nogame]" )
{
    CHECK( utf8_display_split( "你好" ) == std::vector<std::string> { "你", "好" } );
    CHECK( utf8_display_split( "à" ) == std::vector<std::string> { "à" } );
    CHECK( utf8_display_split( "y\u0300" ) == std::vector<std::string> { "y\u0300" } );
    CHECK( utf8_display_split( "à̸̠你⃫" ) == std::vector<std::string> { "à̸̠", "你⃫" } );
    CHECK( utf8_display_split( "    " ) == std::vector<std::string> { " ", " ", " ", " " } );
}

TEST_CASE( "base64", "[catacharset][nogame]" )
{
    CHECK( base64_encode( "hello" ) == "#aGVsbG8=" );
    CHECK( base64_decode( "#aGVsbG8=" ) == "hello" );
}

TEST_CASE( "utf8_to_utf32_roundtrip", "[catacharset][nogame]" )
{
    const std::string original = U8( "Hello, 世界! à y\u0300 à̸̠你⃫" );
    const std::u32string converted = utf8_to_utf32( original );
    const std::string roundtrip = utf32_to_utf8( converted );
    CHECK( roundtrip == original );
}

TEST_CASE( "localized_compare", "[catacharset][nogame]" )
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

static void check_in_place_func( const std::function<void( char32_t & )> &func,
                                 char32_t ch, char32_t expected )
{
    func( ch );
    CHECK( ch == expected );
}

template <typename Func>
static void check_pure_func( Func &&func, const char32_t ch, const char32_t expected )
{
    CHECK( func( ch ) == expected );
}

TEST_CASE( "u32_to_lowercase", "[catacharset][nogame]" )
{
    // Punctuations
    check_pure_func( u32_to_lowercase, U'!', U'!' );

    // Latin
    check_pure_func( u32_to_lowercase, U'a', U'a' );
    check_pure_func( u32_to_lowercase, U'A', U'a' );
    check_pure_func( u32_to_lowercase, U'é', U'é' );
    check_pure_func( u32_to_lowercase, U'É', U'é' );
    check_pure_func( u32_to_lowercase, U'ō', U'ō' );
    check_pure_func( u32_to_lowercase, U'Ō', U'ō' );

    // Greek
    check_pure_func( u32_to_lowercase, U'Α', U'α' );
    check_pure_func( u32_to_lowercase, U'α', U'α' );
    check_pure_func( u32_to_lowercase, U'Σ', U'σ' );
    check_pure_func( u32_to_lowercase, U'σ', U'σ' );

    // Cyrillic
    check_pure_func( u32_to_lowercase, U'а', U'а' );
    check_pure_func( u32_to_lowercase, U'А', U'а' );
    check_pure_func( u32_to_lowercase, U'б', U'б' );
    check_pure_func( u32_to_lowercase, U'Б', U'б' );

    // CJK
    check_pure_func( u32_to_lowercase, U'中', U'中' );
    check_pure_func( u32_to_lowercase, U'の', U'の' );

    // Emoji
    check_pure_func( u32_to_lowercase, U'😅', U'😅' );
}

TEST_CASE( "u32_to_uppercase", "[catacharset][nogame]" )
{
    // Punctuations
    check_pure_func( u32_to_uppercase, U'!', U'!' );

    // Latin
    check_pure_func( u32_to_uppercase, U'a', U'A' );
    check_pure_func( u32_to_uppercase, U'A', U'A' );
    check_pure_func( u32_to_uppercase, U'é', U'É' );
    check_pure_func( u32_to_uppercase, U'É', U'É' );
    check_pure_func( u32_to_uppercase, U'ō', U'Ō' );
    check_pure_func( u32_to_uppercase, U'Ō', U'Ō' );

    // Greek
    check_pure_func( u32_to_uppercase, U'α', U'Α' );
    check_pure_func( u32_to_uppercase, U'Α', U'Α' );
    check_pure_func( u32_to_uppercase, U'σ', U'Σ' );
    check_pure_func( u32_to_uppercase, U'Σ', U'Σ' );

    // Cyrillic
    check_pure_func( u32_to_uppercase, U'а', U'А' );
    check_pure_func( u32_to_uppercase, U'А', U'А' );
    check_pure_func( u32_to_uppercase, U'б', U'Б' );
    check_pure_func( u32_to_uppercase, U'Б', U'Б' );

    // CJK
    check_pure_func( u32_to_uppercase, U'中', U'中' );
    check_pure_func( u32_to_uppercase, U'の', U'の' );

    // Emoji
    check_pure_func( u32_to_uppercase, U'😅', U'😅' );
}

TEST_CASE( "remove_accent", "[catacharset][nogame]" )
{
    // Latin
    check_in_place_func( remove_accent, U'o', U'o' );
    check_in_place_func( remove_accent, U'ô', U'o' );
    check_in_place_func( remove_accent, U'ö', U'o' );
    check_in_place_func( remove_accent, U'ō', U'o' );

    // Cyrillic
    check_in_place_func( remove_accent, U'б', U'б' );

    // CJK
    check_in_place_func( remove_accent, U'中', U'中' );
    check_in_place_func( remove_accent, U'の', U'の' );

    // Emoji
    check_in_place_func( remove_accent, U'😅', U'😅' );
}

TEST_CASE( "uppercase_string", "[catacharset][nogame]" )
{
    CHECK( to_upper_case( "Héllσ 中😅" ) == "HÉLLΣ 中😅" );
    CHECK( uppercase_first_letter( "éllσ" ) == "Éllσ" );
    CHECK( uppercase_first_letter( "中eé" ) == "中eé" );
}

TEST_CASE( "lowercase_string", "[catacharset][nogame]" )
{
    CHECK( to_lower_case( "HéLLΣ 中😅" ) == "héllσ 中😅" );
    CHECK( lowercase_first_letter( "Éllσ" ) == "éllσ" );
    CHECK( lowercase_first_letter( "中Eé" ) == "中Eé" );
}

TEST_CASE( "remove_punctuations", "[catacharset][nogame]" )
{
    CHECK( remove_punctuations( "Hello,_/ world!" ) == "Hello_ world" );
    CHECK( remove_punctuations( "你好，世界！" ) == "你好世界" );
    CHECK( remove_punctuations( "Hello, 世界!" ) == "Hello 世界" );
    CHECK( remove_punctuations( "C'est la vie." ) == "Cest la vie" );
    CHECK( remove_punctuations( "¡Hola, señor!" ) == "Hola señor" );
}

TEST_CASE( "utf8_view", "[catacharset][nogame]" )
{
    static const std::string str{"Français中文русский"};
    static const std::vector<char32_t> expected_code_points{
        0x46, 0x72, 0x61, 0x6e, 0xe7, 0x61, 0x69, 0x73, // Latin
        0x4e2d, 0x6587, // CJK
        0x440, 0x443, 0x441, 0x441, 0x43a, 0x438, 0x439 // Cyrillic
    };
    std::vector<char32_t> actual_code_points;
    for( char32_t c : utf8_view( str ) ) {
        actual_code_points.emplace_back( c );
    }
    CHECK( actual_code_points == expected_code_points );
}

TEST_CASE( "tty", "[catacharset][nogame][!mayfail]" )
{
#ifdef _WIN32
    REQUIRE( _isatty( _fileno( stdout ) ) );
#else
    REQUIRE( isatty( fileno( stdout ) ) );
#endif
    std::cout << U8( "Hello, 世界! à y\u0300 à̸̠你⃫" ) << std::endl;
}