#include <initializer_list>

#include "catch/catch.hpp"
#include "output.h"

template<class IterResult, class IterExpect>
static void check_equal( IterResult beg_res, IterResult end_res,
                         IterExpect beg_exp, IterExpect end_exp )
{
    CHECK( std::distance( beg_res, end_res ) == std::distance( beg_exp, end_exp ) );
    IterResult result = beg_res;
    IterExpect expect = beg_exp;
    for( ; result != end_res && expect != end_exp; ++result, ++expect ) {
        CHECK( *result == *expect );
    }
}

TEST_CASE( "fold-string" )
{
    SECTION( "Case 1 - test wrapping of lorem ipsum" ) {
        const auto folded = foldstring(
                                "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque a.",
                                17
                            );
        const auto expected = {
            /*
             0123456789abcdefg
            */
            "Lorem ipsum dolor",
            "sit amet, ",
            "consectetur ",
            "adipiscing elit. ",
            "Pellentesque a.",
        };
        check_equal( folded.begin(), folded.end(), expected.begin(), expected.end() );
    }

    SECTION( "Case 2 - test wrapping of Chinese" ) {
        const auto folded = foldstring(
                                "春哥纯爷们，铁血真汉子。人民好兄弟，父亲好儿子。",
                                19
                            );
        const auto expected = {
            /*
             0123456789abcdefghi
            */
            "春哥纯爷们，铁血真",
            "汉子。人民好兄弟，",
            "父亲好儿子。",
        };
        check_equal( folded.begin(), folded.end(), expected.begin(), expected.end() );
    }

    SECTION( "Case 3 - test wrapping of mixed language" ) {
        const auto folded = foldstring(
                                "Cataclysm-DDA是Github上的一个开源游戏项目，目前已有超过16000个PR.",
                                13
                            );
        const auto expected = {
            /*
             0123456789abc
            */
            "Cataclysm-DDA",
            "是Github上的",
            "一个开源游戏",
            "项目，目前已",
            "有超过16000个",
            "PR."
        };
        check_equal( folded.begin(), folded.end(), expected.begin(), expected.end() );
    }

    SECTION( "Case 4 - test color tags" ) {
        const auto folded = foldstring(
                                "<color_red>Lorem ipsum dolor sit amet, <color_green>consectetur adipiscing elit</color>. Pellentesque a.</color>",
                                18
                            );
        const auto expected = {
            "<color_red>Lorem ipsum dolor </color>",
            "<color_red>sit amet, </color>",
            "<color_red><color_green>consectetur </color></color>",
            "<color_red><color_green>adipiscing elit</color>. </color>",
            "<color_red>Pellentesque a.</color>",
        };
        check_equal( folded.begin(), folded.end(), expected.begin(), expected.end() );
    }

    SECTION( "Case 5 - test long word" ) {
        const auto folded = foldstring(
                                "You gain a mutation called Hippopotomonstrosesquippedaliophobia!",
                                20
                            );
        const auto expected = {
            /*
             0123456789abcdefghij
            */
            "You gain a mutation ",
            "called ",
            "Hippopotomonstrosesq",
            "uippedaliophobia!"
        };
        check_equal( folded.begin(), folded.end(), expected.begin(), expected.end() );
    }
}
