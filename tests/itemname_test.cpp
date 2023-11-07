#include <iosfwd>
#include <set>
#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "flag.h"
#include "item.h"
#include "item_pocket.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "type_id.h"

static const trait_id trait_HUGE_OK( "HUGE_OK" );
static const trait_id trait_SMALL_OK( "SMALL_OK" );

TEST_CASE( "item_sizing_display", "[item][iteminfo][display_name][sizing]" )
{
    Character &player_character = get_player_character();
    GIVEN( "player is a normal size" ) {
        player_character.clear_mutations();

        WHEN( "the item is a normal size" ) {
            std::string name = item( "bookplate" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_green>++</color>\u00A0bookplate" );
            }
        }

        WHEN( "the item is oversized" ) {
            std::string name = item( "bootsheath" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_green>++</color>\u00A0ankle sheath" );
            }
        }

        WHEN( "the item is undersized" ) {
            item i = item( "tunic" );
            i.set_flag( flag_UNDERSIZE );
            i.set_flag( flag_FIT );
            std::string name = i.display_name();

            THEN( "we have the correct sizing" ) {
                const item::sizing sizing_level = i.get_sizing( player_character );
                CHECK( sizing_level == item::sizing::small_sized_human_char );
            }

            THEN( "the item name says its too small" ) {
                CHECK( name == "<color_c_green>++</color>\u00A0tunic (too small)" );
            }
        }

    }

    GIVEN( "player is a huge size" ) {
        player_character.clear_mutations();
        player_character.toggle_trait( trait_HUGE_OK );

        WHEN( "the item is a normal size" ) {
            std::string name = item( "bookplate" ).display_name();
            THEN( "the item name says its too small" ) {
                CHECK( name == "<color_c_green>++</color>\u00A0bookplate (too small)" );
            }
        }

        WHEN( "the item is oversized" ) {
            std::string name = item( "bootsheath" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_green>++</color>\u00A0ankle sheath" );
            }
        }

        WHEN( "the item is undersized" ) {
            item i = item( "tunic" );
            i.set_flag( flag_UNDERSIZE );
            i.set_flag( flag_FIT );
            std::string name = i.display_name();

            THEN( "we have the correct sizing" ) {
                const item::sizing sizing_level = i.get_sizing( player_character );
                CHECK( sizing_level == item::sizing::small_sized_big_char );
            }

            THEN( "the item name says its tiny" ) {
                CHECK( name == "<color_c_green>++</color>\u00A0tunic (tiny!)" );
            }
        }

    }

    GIVEN( "player is a small size" ) {
        player_character.clear_mutations();
        player_character.toggle_trait( trait_SMALL_OK );

        WHEN( "the item is a normal size" ) {
            std::string name = item( "bookplate" ).display_name();
            THEN( "the item name says its too big" ) {
                CHECK( name == "<color_c_green>++</color>\u00A0bookplate (too big)" );
            }
        }

        WHEN( "the item is oversized" ) {
            std::string name = item( "bootsheath" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_green>++</color>\u00A0ankle sheath (huge!)" );
            }
        }

        WHEN( "the item is undersized" ) {
            item i = item( "tunic" );
            i.set_flag( flag_UNDERSIZE );
            i.set_flag( flag_FIT );
            std::string name = i.display_name();

            THEN( "we have the correct sizing" ) {
                const item::sizing sizing_level = i.get_sizing( player_character );
                CHECK( sizing_level == item::sizing::small_sized_small_char );
            }

            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_green>++</color>\u00A0tunic" );
            }
        }
    }
}

TEST_CASE( "display_name_includes_item_contents", "[item][display_name][contents]" )
{
    clear_avatar();

    item arrow( "test_arrow_wood", calendar::turn_zero, item::default_charges_tag{} );
    // Arrows are ammo with a default count of 10
    REQUIRE( arrow.is_ammo() );
    REQUIRE( arrow.count() == 10 );

    item quiver( "test_quiver" );
    // Quivers are not magazines, nor do they have magazines
    REQUIRE_FALSE( quiver.is_magazine() );
    REQUIRE_FALSE( quiver.magazine_current() );
    // But they do have ammo types and can contain ammo
    REQUIRE_FALSE( quiver.ammo_types().empty() );
    REQUIRE( quiver.can_contain( arrow ).success() );

    // Check empty quiver display
    CHECK( quiver.display_name() ==
           "<color_c_green>++</color>\u00A0"
           "test quiver (0)" );

    // Insert one arrow
    quiver.put_in( arrow, item_pocket::pocket_type::CONTAINER );
    // Expect 1 arrow remaining and displayed
    CHECK( quiver.ammo_remaining() == 10 );
    std::string const arrow_color = get_tag_from_color( arrow.color_in_inventory() );
    std::string const color_end_tag = "</color>";
    CHECK( quiver.display_name() ==
           "<color_c_green>++</color>\u00A0"
           "test quiver > " + arrow_color + "test wooden broadhead arrows" + color_end_tag + " (10)" );
}

TEST_CASE( "display_name_rotten_food", "[item][display_name][contents]" )
{
    clear_avatar();

    item wrapper( "wrapper" );
    item butter_std( "butter" );
    item butter_rot1( "butter" );
    item butter_rot2( "butter" );
    butter_std.set_relative_rot( 0.5 );
    butter_rot1.set_relative_rot( 1.01 );
    butter_rot2.set_relative_rot( 1.02 );

    const std::string butter_std_tname =
        colorize( butter_std.tname(), butter_std.color_in_inventory() );
    const std::string butter_rot_tname =
        colorize( butter_rot1.tname(), butter_rot1.color_in_inventory() );
    REQUIRE( butter_std_tname == "<color_c_light_cyan>butter</color>" );
    REQUIRE( butter_rot_tname == "<color_c_brown>butter (rotten)</color>" );
    REQUIRE_FALSE( butter_std.stacks_with( butter_rot1 ) );
    REQUIRE( butter_rot1.stacks_with( butter_rot2 ) );

    REQUIRE( wrapper.put_in( butter_rot1, item_pocket::pocket_type::CONTAINER ).success() );
    CHECK( wrapper.display_name() ==
           "paper wrapper > " + butter_rot_tname + " hidden" );

    REQUIRE( wrapper.put_in( butter_rot2, item_pocket::pocket_type::CONTAINER ).success() );
    CHECK( wrapper.display_name() ==
           "paper wrapper > " + butter_rot_tname + " (2) hidden" );

    REQUIRE( wrapper.put_in( butter_std, item_pocket::pocket_type::CONTAINER ).success() );
    CHECK( wrapper.display_name() ==
           "paper wrapper > 3 hidden items" );
}
