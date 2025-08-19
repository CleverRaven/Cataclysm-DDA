#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "debug.h"
#include "output.h"

static void test_remove_color_tags( const std::string_view original, const std::string &expected )
{
    CHECK( remove_color_tags( original ) == expected );
}

TEST_CASE( "string_test" )
{
    SECTION( "Case 1 - test remove_color_tags" ) {
        test_remove_color_tags( "<color_red>TestString</color>",
                                "TestString" );
        test_remove_color_tags( "TestStringWithoutOpeningColorTag</color>",
                                "TestStringWithoutOpeningColorTag" );
        test_remove_color_tags( "<color_yellow>TestStringWithoutClosingColorTag",
                                "TestStringWithoutClosingColorTag" );
        test_remove_color_tags( "<color_green>Test</color>StringWithMultiple<color_light_gray>ColorTags",
                                "TestStringWithMultipleColorTags" );
    }
}

static void output_string_trimming_results( const std::string &inputString )
{
    std::cout << "Segments: " << std::endl;
    std::vector<std::string> testSegments = split_by_color( inputString );
    for( const std::string &seg : testSegments ) {
        std::cout << seg << std::endl;
    }
    std::cout << "Fit test: " << std::endl;
    for( int i = 1; i <= utf8_width( inputString, true ); ++i ) {
        std::cout << i << ": |" << std::string( i, ' ' ) << "|" << std::endl;
        std::cout << i << ": |" << remove_color_tags( trim_by_length( inputString,
                  i ) ) << "|" << std::endl;
        std::cout << i << ": " << trim_by_length( inputString, i ) << std::endl;
    }
}

TEST_CASE( "trim_by_length_detailed_string_test", "[.string_trimming]" )
{
    output_string_trimming_results( "У меню <color_light_cyan>Збирання</color> натисніть:" );
}

TEST_CASE( "trim_by_length", "[string_trimming]" )
{
    CHECK( trim_by_length( "ABC", 2 ) == "A…" );
    CHECK( trim_by_length( "ABC", 3 ) == "ABC" );
    CHECK( trim_by_length( "ABCDEF", 4 ) == "ABC…" );
    CHECK( trim_by_length( "AB文字", 6 ) == "AB文字" );
    CHECK( trim_by_length( "AB文字", 5 ) == "AB文…" );
    CHECK( trim_by_length( "AB文字", 4 ) == "AB…" );
    CHECK( trim_by_length( "MRE 主菜（鸡肉意大利香蒜沙司通心粉）（新鲜）",
                           5 ) == "MRE …" );
    CHECK( trim_by_length( "MRE 主菜（鸡肉意大利香蒜沙司通心粉）（新鲜）",
                           6 ) == "MRE …" );
    CHECK( trim_by_length( "MRE 主菜（鸡肉意大利香蒜沙司通心粉）（新鲜）",
                           36 ) == "MRE 主菜（鸡肉意大利香蒜沙司通心粉…" );

    // Check handling of empty strings
    CHECK( trim_by_length( "", 5 ).empty() );

    // Check handling of invalid widths ( <= 0).
    // A debugmsg should be output to tell the player (and programmer) that something has gone wrong
    CHECK( capture_debugmsg_during( [&]() {
        trim_by_length( "", 0 );
    } ) == "Unable to trim string '' to width 0.  Returning empty string." );
    CHECK( capture_debugmsg_during( [&]() {
        trim_by_length( "test string", 0 );
    } ) == "Unable to trim string 'test string' to width 0.  Returning empty string." );
    CHECK( capture_debugmsg_during( [&]() {
        trim_by_length( "test string", -2 );
    } ) == "Unable to trim string 'test string' to width -2.  Returning empty string." );

    // Check trimming at color tag breaks
    /* Note: Due to trim_by_length() doing things one color tag segment at a time, color tags
     * will only appear on the ellipsis if at least one character from the tagged segment is
     * printed. In the keybinding case below, this makes perfect sense, since we don't want
     * to say that the ellipsis is the keybinding.  In other cases, this behaviour is debatable */
    const std::string keybinding_hint = "Press [<color_yellow>?</color>] to change keybindings.";
    CHECK( trim_by_length( keybinding_hint, 6 ) == "Press…" );
    CHECK( trim_by_length( keybinding_hint, 7 ) == "Press …" );
    CHECK( trim_by_length( keybinding_hint, 8 ) == "Press […" );
    CHECK( trim_by_length( keybinding_hint, 9 ) == "Press [<color_yellow>?</color>…" );

    // Test a very long string with multiple sets of color tags
    const std::string jelly_string =
        "Gather <color_light_blue>80 cattail stalks</color> from the swamp and bring "
        "them back to <color_light_red>learn how to craft cattail jelly</color>.  ";
    CHECK( trim_by_length( jelly_string, 10 ) == "Gather <color_light_blue>80…</color>" );
    CHECK( trim_by_length( jelly_string,
                           63 ) == "Gather <color_light_blue>80 cattail stalks</color> from the swamp and bring them back to…" );
    CHECK( trim_by_length( jelly_string, 98 ) == jelly_string );

    // Check trimming at color tag breaks with wide characters
    const std::string tagged_MRE_name =
        "MRE <color_green>主菜</color>（鸡肉意大利香蒜沙司通心粉）（新鲜）";
    CHECK( trim_by_length( tagged_MRE_name, 5 ) == "MRE …" );
    CHECK( trim_by_length( tagged_MRE_name, 6 ) == "MRE …" );
    CHECK( trim_by_length( tagged_MRE_name, 7 ) == "MRE <color_green>主…</color>" );
    CHECK( trim_by_length( tagged_MRE_name, 8 ) == "MRE <color_green>主…</color>" );
    CHECK( trim_by_length( tagged_MRE_name, 9 ) == "MRE <color_green>主菜</color>…" );

    // Check trimming of wide-character strings that are fully color-tagged
    CHECK( trim_by_length( "<color_white_green>休閒區</color>",
                           1 ) == "<color_white_green>…</color>" );
    CHECK( trim_by_length( "<color_white_green>休閒區</color>",
                           3 ) == "<color_white_green>休…</color>" );

    // Check handling of cyrillic text
    const std::string cyrillic_text =
        "У меню <color_light_cyan>Збирання</color> натисніть:";
    CHECK( trim_by_length( cyrillic_text, 2 ) == "У…" );
    CHECK( trim_by_length( cyrillic_text, 9 ) == "У меню <color_light_cyan>З…</color>" );
    CHECK( trim_by_length( cyrillic_text,
                           16 ) == "У меню <color_light_cyan>Збирання</color>…" );

}

TEST_CASE( "str_cat" )
{
    CHECK( str_cat( " " ) == " " );
    CHECK( str_cat( "a", "b", "c" ) == "abc" );
    CHECK( str_cat( "a", std::string( "b" ), "c" ) == "abc" );
    std::string a( "a" );
    std::string b( "b" );
    std::string result = str_cat( a, std::move( b ), a );
    CHECK( result == "aba" );
}

TEST_CASE( "str_append" )
{
    std::string root( "a" );
    CHECK( str_append( root, " " ) == "a " );
    CHECK( str_append( root, "b", "c" ) == "a bc" );
    CHECK( str_append( root, std::string( "b" ), "c" ) == "a bcbc" );
    std::string a( "a" );
    std::string b( "b" );
    str_append( root, a, std::move( b ), a );
    CHECK( root == "a bcbcaba" );
}
