#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "iteminfo_query.h"
#include "player.h"

void iteminfo_test( const item &i, const iteminfo_query &q, const std::string &reference )
{
    g->u.empty_traits();
    std::vector<iteminfo> info_v;
    std::string info = i.info( info_v, &q, 1 );
    CHECK( info == reference );
}

TEST_CASE( "armor_info", "[item][iteminfo]" )
{
    // Just a generic typical piece of clothing
    iteminfo_query q( { iteminfo_parts::ARMOR_BODYPARTS, iteminfo_parts::ARMOR_LAYER,
                        iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH,
                        iteminfo_parts::ARMOR_ENCUMBRANCE, iteminfo_parts::ARMOR_PROTECTION
                      } );
    iteminfo_test(
        item( "longshirt" ), q,
        "--\n"
        "Covers: The <color_c_cyan>torso</color>. The <color_c_cyan>arms</color>. \n"
        "Layer: <color_c_light_blue>Normal</color>. \n"
        "Coverage: <color_c_yellow>90</color>%  Warmth: <color_c_yellow>5</color>\n"
        "--\n"
        "<color_c_white>Encumbrance</color>: <color_c_yellow>3</color> <color_c_red>(poor fit)</color>\n"
        "<color_c_white>Protection</color>: Bash: <color_c_yellow>1</color>  Cut: <color_c_yellow>1</color>\n"
        "  Acid: <color_c_yellow>0</color>  Fire: <color_c_yellow>0</color>  Environmental: <color_c_yellow>0</color>\n" );
}

TEST_CASE( "if_covers_nothing_omit_irreelevant_info", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::ARMOR_BODYPARTS, iteminfo_parts::ARMOR_LAYER,
                        iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH,
                        iteminfo_parts::ARMOR_ENCUMBRANCE, iteminfo_parts::ARMOR_PROTECTION
                      } );
    iteminfo_test(
        item( "ear_plugs" ), q,
        "--\n"
        "Covers: <color_c_cyan>Nothing</color>.\n" );
}
