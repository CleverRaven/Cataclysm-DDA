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
