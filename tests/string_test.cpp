#include "catch/catch.hpp"

#include <string>

#include "output.h"

static void test_remove_color_tags( const std::string &original, const std::string &expected )
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

TEST_CASE( "trim_by_length" )
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
}
