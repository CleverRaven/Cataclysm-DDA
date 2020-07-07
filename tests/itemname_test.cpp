#include <memory>
#include <string>

#include "avatar.h"
#include "catch/catch.hpp"
#include "player_helpers.h"
#include "flat_set.h"
#include "game.h"
#include "item.h"
#include "type_id.h"

TEST_CASE( "item sizing display", "[item][iteminfo][display_name][sizing]" )
{
    GIVEN( "player is a normal size" ) {
        g->u.clear_mutations();

        WHEN( "the item is a normal size" ) {
            std::string name = item( "bookplate" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>bookplate" );
            }
        }

        WHEN( "the item is oversized" ) {
            std::string name = item( "bootsheath" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>ankle sheath" );
            }
        }

        WHEN( "the item is undersized" ) {
            item i = item( "tunic" );
            i.item_tags.insert( "UNDERSIZE" );
            i.item_tags.insert( "FIT" );
            std::string name = i.display_name();

            THEN( "we have the correct sizing" ) {
                const item::sizing sizing_level = i.get_sizing( g->u );
                CHECK( sizing_level == item::sizing::small_sized_human_char );
            }

            THEN( "the item name says its too small" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>tunic (too small)" );
            }
        }

    }

    GIVEN( "player is a huge size" ) {
        g->u.clear_mutations();
        g->u.toggle_trait( trait_id( "HUGE_OK" ) );

        WHEN( "the item is a normal size" ) {
            std::string name = item( "bookplate" ).display_name();
            THEN( "the item name says its too small" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>bookplate (too small)" );
            }
        }

        WHEN( "the item is oversized" ) {
            std::string name = item( "bootsheath" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>ankle sheath" );
            }
        }

        WHEN( "the item is undersized" ) {
            item i = item( "tunic" );
            i.item_tags.insert( "UNDERSIZE" );
            i.item_tags.insert( "FIT" );
            std::string name = i.display_name();

            THEN( "we have the correct sizing" ) {
                const item::sizing sizing_level = i.get_sizing( g->u );
                CHECK( sizing_level == item::sizing::small_sized_big_char );
            }

            THEN( "the item name says its tiny" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>tunic (tiny!)" );
            }
        }

    }

    GIVEN( "player is a small size" ) {
        g->u.clear_mutations();
        g->u.toggle_trait( trait_id( "SMALL_OK" ) );

        WHEN( "the item is a normal size" ) {
            std::string name = item( "bookplate" ).display_name();
            THEN( "the item name says its too big" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>bookplate (too big)" );
            }
        }

        WHEN( "the item is oversized" ) {
            std::string name = item( "bootsheath" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>ankle sheath (huge!)" );
            }
        }

        WHEN( "the item is undersized" ) {
            item i = item( "tunic" );
            i.item_tags.insert( "UNDERSIZE" );
            i.item_tags.insert( "FIT" );
            std::string name = i.display_name();

            THEN( "we have the correct sizing" ) {
                const item::sizing sizing_level = i.get_sizing( g->u );
                CHECK( sizing_level == item::sizing::small_sized_small_char );
            }

            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>tunic" );
            }
        }
    }
}

TEST_CASE( "display name includes item contents", "[item][display_name][contents]" )
{
    clear_avatar();

    item arrow( "test_arrow_wood", 0, item::default_charges_tag{} );
    // Arrows are ammo with a default count of 10
    REQUIRE( arrow.is_ammo() );
    REQUIRE( arrow.count() == 10 );

    item quiver( "test_quiver" );
    // Quivers are not magazines, nor do they have magazines
    REQUIRE_FALSE( quiver.is_magazine() );
    REQUIRE_FALSE( quiver.magazine_current() );
    // But they do have ammo types and can contain ammo
    REQUIRE_FALSE( quiver.ammo_types().empty() );
    REQUIRE( quiver.can_contain( arrow ) );

    // Check empty quiver display
    CHECK( quiver.display_name() ==
           "<color_c_light_green>||\u00A0</color>"
           "test quiver (0)" );

    // Insert one arrow
    quiver.put_in( arrow, item_pocket::pocket_type::CONTAINER );
    // Expect 1 arrow remaining and displayed
    CHECK( quiver.ammo_remaining() == 10 );
    CHECK( quiver.display_name() ==
           "<color_c_light_green>||\u00A0</color>"
           "test quiver with test wooden broadhead arrow (10)" );
}

